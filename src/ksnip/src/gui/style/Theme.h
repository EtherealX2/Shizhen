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

#ifndef KSNIP_THEME_H
#define KSNIP_THEME_H

#include <QObject>
#include <QColor>
#include <QFont>
#include <QIcon>
#include <QString>

class QApplication;
class QWidget;

class Theme : public QObject
{
	Q_OBJECT

public:
	static Theme *instance();

	void apply(QApplication *app, const QString &mode);
	bool isDark() const;

	QColor background() const;
	QColor surface() const;
	// One step above the card: menus, popups, the selected segment and the
	// floating capture toolbar. Light themes go to pure white, dark themes lift
	// instead of sink (a darker thumb inside a lighter track reads as pressed).
	QColor surfaceRaised() const;
	// Editable fields sit one step above or below the card depending on theme.
	QColor fieldSurface() const;
	QColor surfaceAlt() const;
	QColor border() const;
	QColor borderStrong() const;
	QColor inputBorder() const;
	QColor hover() const;
	QColor pressed() const;
	QColor textPrimary() const;
	QColor textSecondary() const;
	QColor textTertiary() const;
	QColor accent() const;
	QColor accentHover() const;
	// Bottom stop of the vertical primary-button gradient (BoardUI paints its
	// primary surface as a two-stop ramp instead of a flat fill).
	QColor accentDeep() const;
	QColor accentSoft() const;
	QColor accentBorder() const;
	QColor onAccent() const;
	QColor overlaySurface() const;
	QColor overlayText() const;
	QColor snippingAccent() const;
	QColor destructive() const;
	QColor ring() const;
	QColor success() const;

	QFont baseFont() const;
	QFont monoFont() const;

	// Lucide-style glyph from the bundled icon set, tinted for the active theme.
	QIcon icon(const QString &name) const;
	// Same glyph, inverted for use on a filled primary surface.
	QIcon onAccentIcon(const QString &name) const;

	QString globalStyleSheet() const;
	QString toolBarStyleSheet() const;
	QString stripStyleSheet() const;

signals:
	void changed();

protected:
	bool eventFilter(QObject *watched, QEvent *event) override;

private:
	explicit Theme(QObject *parent = nullptr);
	static bool systemIsDark();
	void applyTitleBar(QWidget *widget);

	bool mDark;
};

#endif // KSNIP_THEME_H
