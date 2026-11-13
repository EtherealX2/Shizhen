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

#ifndef KSNIP_UI_H
#define KSNIP_UI_H

#include <QSize>
#include <QString>
#include <QVariant>

class QFrame;
class QHBoxLayout;
class QLabel;
class QPushButton;
class QVBoxLayout;
class QWidget;

// Small factory layer that keeps every screen on the same shadcn/ui primitives:
// card surfaces, 36px controls, segmented tabs, variant buttons and hairline
// separators. Layouts are returned so callers can keep building in place.
namespace Ui
{
enum class ButtonVariant
{
	Primary,
	Outline,
	Ghost,
	Secondary,
	Destructive
};

// shadcn Card with an optional header (title + description) and a body layout.
QFrame *card(QWidget *parent, const QString &title, const QString &description, QVBoxLayout *&body);
QFrame *hline(QWidget *parent = nullptr);
QFrame *vline(QWidget *parent = nullptr);
QLabel *cardTitle(const QString &text, QWidget *parent);
QLabel *mutedLabel(const QString &text, QWidget *parent);
QPushButton *button(const QString &text, ButtonVariant variant, QWidget *parent);
QHBoxLayout *buttonRow(QWidget *parent = nullptr);

// QSS attribute selectors are resolved on first polish, so anything applied
// after construction has to be re-polished to pick the new rule up.
void repolish(QWidget *widget);
void setControlHeight(QWidget *widget, int height = 36);
void setIcon(QWidget *widget, const QString &name, const QSize &size = QSize(16, 16));
}

#endif // KSNIP_UI_H
