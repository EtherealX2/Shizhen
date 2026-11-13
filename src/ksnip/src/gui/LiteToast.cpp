/*
 * Copyright (C) 2020 Damir Porobic <damir.porobic@gmx.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "LiteToast.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QPointer>
#include <QScreen>
#include <QTimer>
#include <QVBoxLayout>

#include "src/gui/style/Theme.h"

static QPointer<LiteToast> sCurrentToast;

LiteToast::LiteToast(const QString &title, const QString &message) :
	QWidget(nullptr, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint),
	mIconLabel(new QLabel(this)),
	mTitleLabel(new QLabel(title, this)),
	mMessageLabel(new QLabel(message, this))
{
	setAttribute(Qt::WA_TranslucentBackground);
	setAttribute(Qt::WA_ShowWithoutActivating);
	setAttribute(Qt::WA_DeleteOnClose);

	auto theme = Theme::instance();
	setStyleSheet(QStringLiteral("QLabel { background: transparent; border: none; }"));

	mIconLabel->setPixmap(QIcon(QStringLiteral(":/icons/app.png")).pixmap(28, 28));
	mIconLabel->setFixedSize(28, 28);

	auto titleFont = theme->baseFont();
	titleFont.setBold(true);
	mTitleLabel->setFont(titleFont);
	mTitleLabel->setStyleSheet(QStringLiteral("color: %1;").arg(theme->textPrimary().name()));
	mMessageLabel->setStyleSheet(QStringLiteral("color: %1;").arg(theme->textSecondary().name()));
	mMessageLabel->setWordWrap(true);
	mTitleLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
	mMessageLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

	auto textLayout = new QVBoxLayout;
	textLayout->setContentsMargins(0, 0, 0, 0);
	textLayout->setSpacing(2);
	textLayout->addWidget(mTitleLabel);
	textLayout->addWidget(mMessageLabel);

	auto layout = new QHBoxLayout(this);
	layout->setContentsMargins(14, 12, 14, 12);
	layout->setSpacing(10);
	layout->addWidget(mIconLabel, 0, Qt::AlignTop);
	layout->addLayout(textLayout, 1);

	setFixedWidth(360);
	adjustSize();

	auto area = QApplication::primaryScreen()->availableGeometry();
	move(area.right() - width() - 12, area.bottom() - height() - 12);

	QTimer::singleShot(4000, this, &QWidget::close);
}

void LiteToast::show(const QString &title, const QString &message)
{
	dismiss();
	sCurrentToast = new LiteToast(title, message);
	sCurrentToast->QWidget::show();
}

void LiteToast::dismiss()
{
	if(sCurrentToast) {
		sCurrentToast->close();
	}
}

void LiteToast::mousePressEvent(QMouseEvent *event)
{
	Q_UNUSED(event)
	close();
}

void LiteToast::paintEvent(QPaintEvent *event)
{
	Q_UNUSED(event)
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);
	auto theme = Theme::instance();
	auto rect = QRectF(this->rect()).adjusted(0.5, 0.5, -0.5, -0.5);
	painter.setPen(QPen(theme->border(), 1));
	painter.setBrush(theme->surface());
	painter.drawRoundedRect(rect, 10, 10);
}