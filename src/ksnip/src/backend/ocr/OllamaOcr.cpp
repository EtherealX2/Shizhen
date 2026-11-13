/*
 * Copyright (C) 2026 ksnip-lite contributors
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "OllamaOcr.h"

#include <QBuffer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>

namespace {
// A degenerate model can ramble for megabytes; parsing that on the UI thread
// freezes the whole app, so oversized answers are rejected instead.
constexpr qint64 maxResponseBytes = 8 * 1024 * 1024;
}

OllamaOcr::OllamaOcr(const QSharedPointer<IConfig> &config, QObject *parent) :
	QObject(parent),
	mConfig(config),
	mReply(nullptr),
	mIsTesting(false),
	mCancelled(false)
{
	mTimeoutTimer.setSingleShot(true);
	mTimeoutTimer.setInterval(180 * 1000);
	connect(&mTimeoutTimer, &QTimer::timeout, this, [this](){
		if(mReply != nullptr) {
			mReply->abort();
		}
	});
}

bool OllamaOcr::isBusy() const
{
	return mReply != nullptr;
}

QString OllamaOcr::baseUrl() const
{
	auto url = mConfig->ollamaUrl().trimmed();
	while(url.endsWith(QLatin1Char('/'))) {
		url.chop(1);
	}
	return url;
}

void OllamaOcr::recognize(const QImage &image)
{
	if(isBusy() || image.isNull()) {
		return;
	}

	QByteArray imageData;
	QBuffer buffer(&imageData);
	buffer.open(QIODevice::WriteOnly);
	image.save(&buffer, "PNG");
	buffer.close();

	QJsonObject body;
	body.insert(QLatin1String("model"), mConfig->ollamaModel());
	body.insert(QLatin1String("prompt"), mConfig->ollamaPrompt());
	body.insert(QLatin1String("system"), QStringLiteral("你是 OCR 文字提取引擎。只逐字输出图片中实际存在的文字，保持原有排版；按图片中的换行逐行输出，保留原文的换行与段落结构，不要合并行、不要拆分行、不要重新排版；禁止输出任何描述、总结、解释、前言、结语、推测或对话内容。"));
	body.insert(QLatin1String("stream"), false);
	body.insert(QLatin1String("think"), false);
	QJsonObject options;
	options.insert(QLatin1String("temperature"), 0);
	body.insert(QLatin1String("options"), options);
	QJsonArray images;
	images.append(QString::fromLatin1(imageData.toBase64()));
	body.insert(QLatin1String("images"), images);

	QNetworkRequest request{QUrl(baseUrl() + QLatin1String("/api/generate"))};
	request.setHeader(QNetworkRequest::ContentTypeHeader, QLatin1String("application/json"));

	startRequest(request, QJsonDocument(body).toJson(QJsonDocument::Compact), false);
}

void OllamaOcr::testConnection()
{
	if(isBusy()) {
		return;
	}

	QNetworkRequest request{QUrl(baseUrl() + QLatin1String("/api/tags"))};
	startRequest(request, QByteArray(), true);
}

void OllamaOcr::startRequest(const QNetworkRequest &request, const QByteArray &body, bool isTesting)
{
	mIsTesting = isTesting;
	mCancelled = false;
	mTimeoutTimer.start();

	if(body.isEmpty()) {
		mReply = mNetworkManager.get(request);
	} else {
		mReply = mNetworkManager.post(request, body);
	}

	connect(mReply, &QNetworkReply::finished, this, &OllamaOcr::handleReply);
}

void OllamaOcr::cancel()
{
	if(mReply != nullptr) {
		mCancelled = true;
		mReply->abort();
	}
}

void OllamaOcr::handleReply()
{
	mTimeoutTimer.stop();

	auto reply = mReply;
	mReply = nullptr;
	reply->deleteLater();

	auto isTesting = mIsTesting;
	auto wasCancelled = mCancelled;
	mCancelled = false;

	if(wasCancelled) {
		// The owning window is closing; stay silent instead of reporting a failure.
		return;
	}

	if(reply->error() != QNetworkReply::NoError) {
		auto error = reply->errorString();
		if(reply->error() == QNetworkReply::OperationCanceledError) {
			error = tr("OCR 请求超时或已取消。");
		}
		if(isTesting) {
			emit testFinished(false, error);
		} else {
			emit failed(error);
		}
		return;
	}

	if(reply->bytesAvailable() > maxResponseBytes) {
		auto error = tr("OCR 返回内容过大，已中止。请缩小截图范围后重试。");
		if(isTesting) {
			emit testFinished(false, error);
		} else {
			emit failed(error);
		}
		return;
	}

	auto response = QJsonDocument::fromJson(reply->readAll());
	if(!response.isObject()) {
		auto error = tr("Ollama 返回了无效响应。");
		if(isTesting) {
			emit testFinished(false, error);
		} else {
			emit failed(error);
		}
		return;
	}

	if(isTesting) {
		auto models = response.object().value(QLatin1String("models")).toArray();
		QStringList names;
		for(const auto &model : models) {
			names.append(model.toObject().value(QLatin1String("name")).toString());
		}
		emit testFinished(true, tr("连接成功。可用模型：%1").arg(names.join(QLatin1String(", "))));
		return;
	}

	auto text = response.object().value(QLatin1String("response")).toString();
	auto thinkEnd = text.lastIndexOf(QLatin1String("</think>"));
	if(thinkEnd >= 0) {
		text = text.mid(thinkEnd + 8);
	}
	emit finished(text.trimmed());
}