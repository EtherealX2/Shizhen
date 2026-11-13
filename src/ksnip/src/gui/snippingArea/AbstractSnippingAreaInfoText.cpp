/*
 * Copyright (C) 2020 Damir Porobic <damir.porobic@gmx.com>
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

#include "AbstractSnippingAreaInfoText.h"

#include "src/gui/style/Theme.h"

AbstractSnippingAreaInfoText::AbstractSnippingAreaInfoText(QObject *parent):
        QObject(parent),
        mBaseInfoTextRectSize(new QSize(500, 100)),
        mRectOffset(40, 40),
        mIsActive(false)
{
}

AbstractSnippingAreaInfoText::~AbstractSnippingAreaInfoText()
{
	delete mBaseInfoTextRectSize;
}

void AbstractSnippingAreaInfoText::paint(QPainter *painter)
{
	if(mIsActive) {
		auto theme = Theme::instance();
		auto fontMetric = painter->fontMetrics();
		auto textRect = fontMetric.boundingRect(QRect(mSnippingAreaGeometry.topLeft().toPoint() + mRectOffset, *mBaseInfoTextRectSize), Qt::TextWordWrap, mInfoText);
		auto boundingRect = textRect.adjusted(-14, -14, 14, 14);

		if(boundingRect.contains(mCurrentMousePos.toPoint())) {
			auto bottomPosition = mSnippingAreaGeometry.bottomRight().toPoint();
			textRect.moveBottomRight(bottomPosition - QPoint(54, 54));
			boundingRect.moveBottomRight(bottomPosition - mRectOffset);
		}

		painter->setRenderHint(QPainter::Antialiasing);
		painter->setBrush(theme->overlaySurface());
		painter->setPen(QPen(theme->borderStrong(), 1));
		painter->drawRoundedRect(boundingRect, 8, 8);

		auto lines = mInfoText.split(QLatin1Char('\n'));
		auto lineHeight = fontMetric.lineSpacing();
		auto y = textRect.top();
		for(int i = 0; i < lines.size(); ++i) {
			auto font = painter->font();
			font.setWeight(i == 0 ? QFont::Medium : QFont::Normal);
			painter->setFont(font);
			painter->setPen(i == 0 ? theme->textPrimary() : theme->textSecondary());
			painter->drawText(QRect(textRect.left(), y, textRect.width(), lineHeight), Qt::TextWordWrap, lines[i]);
			y += lineHeight;
		}
	}
}

void AbstractSnippingAreaInfoText::handleMouseMove(const QPointF &pos)
{
	mCurrentMousePos = pos;
}

void AbstractSnippingAreaInfoText::activate(const QRectF &snippingAreaGeometry)
{
	mIsActive = true;
	mSnippingAreaGeometry = snippingAreaGeometry;
	updateInfoText();
}

void AbstractSnippingAreaInfoText::deactivate()
{
	mIsActive = false;
}

void AbstractSnippingAreaInfoText::setInfoText(const QStringList &infoTextLines)
{
	auto newLine = QLatin1String("\n");
	mInfoText = QString();
	for (int i = 0; i < infoTextLines.length(); ++i) {
		mInfoText += infoTextLines[i];
		if(i < infoTextLines.length() - 1){
			mInfoText += newLine;
		}
	}
}
