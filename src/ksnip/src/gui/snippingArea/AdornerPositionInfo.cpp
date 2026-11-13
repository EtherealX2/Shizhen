/*
 * Copyright (C) 2019 Damir Porobic <damir.porobic@gmx.com>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "AdornerPositionInfo.h"

#include "src/gui/style/Theme.h"

AdornerPositionInfo::AdornerPositionInfo() :
	mFont(Theme::instance()->monoFont()),
	mFontMetric(new QFontMetrics(mFont))
{
}

AdornerPositionInfo::~AdornerPositionInfo()
{
	delete mFontMetric;
}

void AdornerPositionInfo::update(const QPoint &mousePosition, const QRect &screenRect)
{
	QPoint textOffset(10, 8);
    auto offset = screenRect.topLeft();
	mText = QString::number(mousePosition.x() - offset.x()) + QLatin1String(", ") + QString::number(mousePosition.y() - offset.y());
	mBox = mFontMetric->boundingRect(mText);
	mBox.moveTopLeft(mousePosition + textOffset);
	mTextRect = mBox;
	mBox.adjust(0, 0, 7, 4);
	mTextRect.adjust(-3, 0, 5, 0);
}

void AdornerPositionInfo::paint(QPainter *painter, const QColor &color)
{
	Q_UNUSED(color)
	auto theme = Theme::instance();
	painter->setRenderHint(QPainter::Antialiasing);
	painter->setPen(QPen(theme->borderStrong(), 1));
	painter->setBrush(theme->overlaySurface());
	painter->drawRoundedRect(mTextRect, 6, 6);

	painter->setFont(mFont);
	painter->setPen(theme->textPrimary());
	painter->drawText(mBox, mText);
}
