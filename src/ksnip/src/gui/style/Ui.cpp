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

#include "Ui.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSize>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

#include "Theme.h"

namespace Ui
{
QFrame *card(QWidget *parent, const QString &title, const QString &description, QVBoxLayout *&body)
{
	auto frame = new QFrame(parent);
	frame->setObjectName(QLatin1String("liteCard"));
	frame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	// Without this the stylesheet paints the border but not the surface, so a
	// card is invisible against the canvas.
	frame->setAttribute(Qt::WA_StyledBackground, true);

	auto outer = new QVBoxLayout(frame);
	outer->setContentsMargins(20, 18, 20, 18);
	outer->setSpacing(5);

	auto titleLabel = cardTitle(title, frame);
	outer->addWidget(titleLabel);

	if(!description.isEmpty()) {
		auto desc = mutedLabel(description, frame);
		desc->setObjectName(QLatin1String("liteCardDescription"));
		desc->setWordWrap(true);
		desc->setContentsMargins(0, 2, 0, 0);
		outer->addWidget(desc);
	}

	body = new QVBoxLayout;
	body->setContentsMargins(0, 12, 0, 0);
	body->setSpacing(12);
	outer->addLayout(body);

	return frame;
}

QFrame *hline(QWidget *parent)
{
	auto line = new QFrame(parent);
	line->setObjectName(QLatin1String("liteHLine"));
	line->setFrameShape(QFrame::HLine);
	line->setFixedHeight(1);
	line->setAttribute(Qt::WA_StyledBackground, true);
	return line;
}

QFrame *vline(QWidget *parent)
{
	auto line = new QFrame(parent);
	line->setObjectName(QLatin1String("liteVLine"));
	line->setFrameShape(QFrame::VLine);
	line->setFixedWidth(1);
	line->setAttribute(Qt::WA_StyledBackground, true);
	return line;
}

QLabel *cardTitle(const QString &text, QWidget *parent)
{
	auto label = new QLabel(text, parent);
	label->setObjectName(QLatin1String("liteCardTitle"));
	return label;
}

QLabel *mutedLabel(const QString &text, QWidget *parent)
{
	auto label = new QLabel(text, parent);
	label->setProperty("role", QLatin1String("muted"));
	return label;
}

QPushButton *button(const QString &text, ButtonVariant variant, QWidget *parent)
{
	auto pushButton = new QPushButton(text, parent);
	switch(variant) {
		case ButtonVariant::Primary:
			pushButton->setProperty("accent", true);
			break;
		case ButtonVariant::Ghost:
			pushButton->setProperty("variant", QLatin1String("ghost"));
			break;
		case ButtonVariant::Secondary:
			pushButton->setProperty("variant", QLatin1String("secondary"));
			break;
		case ButtonVariant::Destructive:
			pushButton->setProperty("variant", QLatin1String("destructive"));
			break;
		case ButtonVariant::Outline:
			break;
	}
	repolish(pushButton);
	setControlHeight(pushButton);
	return pushButton;
}

QHBoxLayout *buttonRow(QWidget *parent)
{
	auto layout = parent != nullptr ? new QHBoxLayout(parent) : new QHBoxLayout;
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(8);
	return layout;
}
void repolish(QWidget *widget)
{
	if(widget == nullptr) {
		return;
	}
	widget->style()->unpolish(widget);
	widget->style()->polish(widget);
	widget->update();
}

void setControlHeight(QWidget *widget, int height)
{
	if(widget == nullptr) {
		return;
	}
	widget->setFixedHeight(height);
	widget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
}

void setIcon(QWidget *widget, const QString &name, const QSize &size)
{
	auto icon = Theme::instance()->icon(name);
	if(auto label = qobject_cast<QLabel*>(widget)) {
		label->setPixmap(icon.pixmap(size));
	} else if(auto toolButton = qobject_cast<QToolButton*>(widget)) {
		toolButton->setIcon(icon);
		toolButton->setIconSize(size);
	} else if(auto pushButton = qobject_cast<QPushButton*>(widget)) {
		pushButton->setIcon(icon);
		pushButton->setIconSize(size);
	}
}
}
