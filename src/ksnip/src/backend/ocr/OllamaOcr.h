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

#ifndef KSNIP_OLLAMAOCR_H
#define KSNIP_OLLAMAOCR_H

#include <QObject>
#include <QImage>
#include <QSharedPointer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>

#include "src/backend/config/IConfig.h"

class OllamaOcr : public QObject
{
	Q_OBJECT
public:
	explicit OllamaOcr(const QSharedPointer<IConfig> &config, QObject *parent = nullptr);

	void recognize(const QImage &image);
	void testConnection();
	void cancel();
	bool isBusy() const;

signals:
	void finished(const QString &text);
	void failed(const QString &error);
	void testFinished(bool successful, const QString &info);

private:
	QSharedPointer<IConfig> mConfig;
	QNetworkAccessManager mNetworkManager;
	QNetworkReply *mReply;
	QTimer mTimeoutTimer;
	bool mIsTesting;
	bool mCancelled;

	void startRequest(const QNetworkRequest &request, const QByteArray &body, bool isTesting);
	void handleReply();
	QString baseUrl() const;
};

#endif //KSNIP_OLLAMAOCR_H