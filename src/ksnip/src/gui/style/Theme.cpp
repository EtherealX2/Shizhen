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

#include "Theme.h"

#include <QApplication>
#include <QPalette>
#include <QSettings>
#include <QStyleFactory>
#include <QIcon>
#include <QWidget>
#include <QEvent>
#include <QVector>
#include <algorithm>

#ifdef Q_OS_WIN
#include <windows.h>
#include <dwmapi.h>
#endif

// Design tokens follow BoardUI (boardui.com): a tinted canvas with white cards,
// hairline #ebebeb borders, 10/12px radii, a blue accent whose primary surface
// is a two-stop vertical gradient, and the 20/16/14/13/12 type ramp.
Theme *Theme::instance()
{
	static Theme theme;
	return &theme;
}

Theme::Theme(QObject *parent) :
	QObject(parent),
	mDark(false)
{
}

bool Theme::systemIsDark()
{
	QSettings personalize(QLatin1String("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize"), QSettings::NativeFormat);
	return personalize.value(QLatin1String("AppsUseLightTheme"), 1).toInt() == 0;
}

void Theme::apply(QApplication *app, const QString &mode)
{
	mDark = mode == QLatin1String("dark") || (mode == QLatin1String("system") && systemIsDark());

	app->setStyle(QStyleFactory::create(QLatin1String("Fusion")));

	QPalette palette;
	palette.setColor(QPalette::Window, background());
	palette.setColor(QPalette::WindowText, textPrimary());
	palette.setColor(QPalette::Base, surface());
	palette.setColor(QPalette::AlternateBase, surfaceAlt());
	palette.setColor(QPalette::Text, textPrimary());
	palette.setColor(QPalette::PlaceholderText, textTertiary());
	palette.setColor(QPalette::Button, surface());
	palette.setColor(QPalette::ButtonText, textPrimary());
	palette.setColor(QPalette::Highlight, accent());
	palette.setColor(QPalette::HighlightedText, onAccent());
	palette.setColor(QPalette::ToolTipBase, mDark ? QColor(250, 250, 250) : QColor(10, 10, 10));
	palette.setColor(QPalette::ToolTipText, mDark ? QColor(10, 10, 10) : QColor(250, 250, 250));
	palette.setColor(QPalette::Link, accent());
	palette.setColor(QPalette::Disabled, QPalette::WindowText, textTertiary());
	palette.setColor(QPalette::Disabled, QPalette::Text, textTertiary());
	palette.setColor(QPalette::Disabled, QPalette::ButtonText, textTertiary());
	app->setPalette(palette);

	app->setFont(baseFont());
	app->setStyleSheet(globalStyleSheet());
	app->setWindowIcon(QIcon(QLatin1String(":/icons/app.png")));

	app->installEventFilter(this);
	for(auto widget : app->topLevelWidgets()) {
		applyTitleBar(widget);
	}

	emit changed();
}

bool Theme::eventFilter(QObject *watched, QEvent *event)
{
	if(event->type() == QEvent::Show || event->type() == QEvent::WinIdChange) {
		auto widget = qobject_cast<QWidget*>(watched);
		if(widget != nullptr && widget->isWindow()) {
			applyTitleBar(widget);
		}
	}
	return QObject::eventFilter(watched, event);
}

void Theme::applyTitleBar(QWidget *widget)
{
#ifdef Q_OS_WIN
	if(widget->windowFlags() & Qt::FramelessWindowHint) {
		return;
	}
	// DWMWA_USE_IMMERSIVE_DARK_MODE; older SDK headers do not define it.
	constexpr DWORD darkModeAttribute = 20;
	BOOL dark = mDark ? TRUE : FALSE;
	DwmSetWindowAttribute(reinterpret_cast<HWND>(widget->winId()), darkModeAttribute, &dark, sizeof(dark));
	// Windows 10 releases before 20H1 only honour the preview attribute.
	constexpr DWORD darkModePreviewAttribute = 19;
	DwmSetWindowAttribute(reinterpret_cast<HWND>(widget->winId()), darkModePreviewAttribute, &dark, sizeof(dark));
#else
	Q_UNUSED(widget)
#endif
}

bool Theme::isDark() const
{
	return mDark;
}

QColor Theme::background() const
{
	// The canvas is tinted so cards read as elevated surfaces.
	return mDark ? QColor(18, 18, 18) : QColor(250, 250, 250);
}

QColor Theme::surface() const
{
	return mDark ? QColor(28, 28, 28) : QColor(255, 255, 255);
}

QColor Theme::surfaceRaised() const
{
	return mDark ? QColor(45, 45, 45) : QColor(255, 255, 255);
}

QColor Theme::fieldSurface() const
{
	return mDark ? QColor(35, 35, 35) : QColor(255, 255, 255);
}

QColor Theme::surfaceAlt() const
{
	return mDark ? QColor(36, 36, 36) : QColor(245, 245, 245);
}

QColor Theme::border() const
{
	return mDark ? QColor(255, 255, 255, 26) : QColor(235, 235, 235);
}

QColor Theme::borderStrong() const
{
	return mDark ? QColor(255, 255, 255, 45) : QColor(212, 212, 212);
}

QColor Theme::inputBorder() const
{
	return mDark ? QColor(255, 255, 255, 38) : QColor(226, 226, 226);
}

QColor Theme::hover() const
{
	return mDark ? QColor(255, 255, 255, 16) : QColor(245, 245, 245);
}

QColor Theme::pressed() const
{
	return mDark ? QColor(255, 255, 255, 30) : QColor(235, 235, 235);
}

QColor Theme::textPrimary() const
{
	return mDark ? QColor(245, 245, 245) : QColor(10, 10, 10);
}

QColor Theme::textSecondary() const
{
	return mDark ? QColor(163, 163, 163) : QColor(111, 111, 111);
}

QColor Theme::textTertiary() const
{
	return mDark ? QColor(115, 115, 115) : QColor(161, 161, 161);
}

QColor Theme::accent() const
{
	// BoardUI maps its accent ramp to blue; blue-500 in light, slightly lifted
	// in dark so it keeps contrast on a #1a1a1a card.
	return mDark ? QColor(75, 147, 255) : QColor(43, 127, 255);
}

QColor Theme::accentHover() const
{
	return mDark ? QColor(96, 161, 255) : QColor(33, 111, 235);
}

QColor Theme::accentDeep() const
{
	// accent-600: the bottom stop of the primary gradient.
	return mDark ? QColor(43, 127, 255) : QColor(25, 102, 229);
}

QColor Theme::accentSoft() const
{
	auto soft = accent();
	soft.setAlpha(mDark ? 48 : 24);
	return soft;
}

QColor Theme::accentBorder() const
{
	auto borderColor = accent();
	borderColor.setAlpha(mDark ? 96 : 70);
	return borderColor;
}

QColor Theme::onAccent() const
{
	return QColor(255, 255, 255);
}

QColor Theme::overlaySurface() const
{
	return mDark ? QColor(26, 26, 26, 250) : QColor(255, 255, 255, 250);
}

QColor Theme::overlayText() const
{
	return textPrimary();
}

QColor Theme::snippingAccent() const
{
	// neutral-500 keeps the crosshair legible on both light and dark zoomed pixels.
	return QColor(115, 115, 115);
}

QColor Theme::destructive() const
{
	return mDark ? QColor(248, 113, 113) : QColor(220, 38, 38);
}

QColor Theme::ring() const
{
	auto focusRing = accent();
	focusRing.setAlpha(mDark ? 130 : 105);
	return focusRing;
}

QColor Theme::success() const
{
	return mDark ? QColor(74, 222, 128) : QColor(22, 163, 74);
}

QFont Theme::baseFont() const
{
	QFont font;
	font.setFamilies({QLatin1String("Inter"), QLatin1String("Segoe UI Variable Text"),
		QLatin1String("Segoe UI"), QLatin1String("Microsoft YaHei UI"),
		QLatin1String("Microsoft YaHei"), QLatin1String("Noto Sans SC")});
	font.setPixelSize(14);
	font.setHintingPreference(QFont::PreferFullHinting);
	return font;
}

QFont Theme::monoFont() const
{
	// Qt 5.15 has no OpenType feature API; a monospace family guarantees tabular figures.
	QFont font;
	font.setFamilies({QLatin1String("Cascadia Mono"), QLatin1String("Consolas"),
		QLatin1String("Courier New")});
	font.setPixelSize(13);
	return font;
}

QIcon Theme::icon(const QString &name) const
{
	// Each glyph ships twice: light ink for the dark theme, dark ink for the light one.
	return QIcon(QStringLiteral(":/icons/ui/%1-%2.svg").arg(name, mDark ? QLatin1String("light") : QLatin1String("dark")));
}

QIcon Theme::onAccentIcon(const QString &name) const
{
	// Glyphs drawn on top of a filled primary surface invert with the theme.
	return QIcon(QStringLiteral(":/icons/ui/%1%2.svg").arg(name, mDark ? QLatin1String("-dark") : QString()));
}

static QString css(const QColor &color)
{
	return QStringLiteral("rgba(%1,%2,%3,%4)")
		.arg(color.red()).arg(color.green()).arg(color.blue()).arg(color.alpha());
}

// Gradient stops have to be hex: Qt's stop parser does not accept rgba().
static QString hex(const QColor &color)
{
	return color.name(QColor::HexRgb);
}

static QString gradient(const QColor &top, const QColor &bottom)
{
	return QStringLiteral("qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 %1, stop:1 %2)")
		.arg(hex(top), hex(bottom));
}

static QColor withAlpha(QColor color, int alpha)
{
	color.setAlpha(alpha);
	return color;
}

static QString expandTokens(QString sheet, const QVector<QPair<QString, QString>> &tokens)
{
	// Longest name first: "@primary" is a prefix of "@primary-fg" and replacing it
	// first would leave "#2b7fff-fg" behind and silently drop the declaration.
	auto ordered = tokens;
	std::sort(ordered.begin(), ordered.end(), [](const QPair<QString, QString> &a, const QPair<QString, QString> &b){
		return a.first.size() > b.first.size();
	});
	for(const auto &token : ordered) {
		sheet.replace(token.first, token.second);
	}
	return sheet;
}

QString Theme::globalStyleSheet() const
{
	const auto ink = mDark ? QStringLiteral("light") : QStringLiteral("dark");
	const auto checkIcon = mDark ? QLatin1String(":/icons/ui/check-on-accent-dark.svg")
								 : QLatin1String(":/icons/ui/check-on-accent.svg");
	const auto radioIcon = mDark ? QLatin1String(":/icons/ui/radio-on-accent-dark.svg")
								 : QLatin1String(":/icons/ui/radio-on-accent.svg");

	const QVector<QPair<QString, QString>> tokens = {
		{ QStringLiteral("@fg"), css(textPrimary()) },
		{ QStringLiteral("@muted-fg"), css(textSecondary()) },
		{ QStringLiteral("@tertiary-fg"), css(textTertiary()) },
		{ QStringLiteral("@bg"), css(background()) },
		{ QStringLiteral("@card"), css(surface()) },
		{ QStringLiteral("@raised"), css(surfaceRaised()) },
		{ QStringLiteral("@field"), css(fieldSurface()) },
		{ QStringLiteral("@muted"), css(surfaceAlt()) },
		{ QStringLiteral("@hover"), css(hover()) },
		{ QStringLiteral("@pressed"), css(pressed()) },
		{ QStringLiteral("@border"), css(border()) },
		{ QStringLiteral("@border-strong"), css(borderStrong()) },
		{ QStringLiteral("@input"), css(inputBorder()) },
		{ QStringLiteral("@primary"), css(accent()) },
		{ QStringLiteral("@primary-fg"), css(onAccent()) },
		{ QStringLiteral("@primary-hover"), css(accentHover()) },
		{ QStringLiteral("@primary-soft"), css(accentSoft()) },
		{ QStringLiteral("@primary-border"), css(accentBorder()) },
		{ QStringLiteral("@primary-grad"), gradient(accent(), accentDeep()) },
		{ QStringLiteral("@primary-grad-hover"), gradient(accentHover(), accent()) },
		{ QStringLiteral("@primary-grad-pressed"), gradient(accentDeep(), accentDeep()) },
		{ QStringLiteral("@ring"), css(ring()) },
		{ QStringLiteral("@destructive"), css(destructive()) },
		{ QStringLiteral("@destructive-soft"), css(withAlpha(destructive(), mDark ? 46 : 22)) },
		{ QStringLiteral("@success"), css(success()) },
		{ QStringLiteral("@success-soft"), css(withAlpha(success(), mDark ? 40 : 18)) },
		{ QStringLiteral("@tooltip-bg"), mDark ? css(QColor(250, 250, 250)) : css(QColor(10, 10, 10)) },
		{ QStringLiteral("@tooltip-fg"), mDark ? css(QColor(10, 10, 10)) : css(QColor(250, 250, 250)) },
		{ QStringLiteral("@ink"), ink },
		{ QStringLiteral("@check-icon"), checkIcon },
		{ QStringLiteral("@radio-icon"), radioIcon }
	};

	const QString sheet = QStringLiteral(
		// ---- base ------------------------------------------------------------
		"* { outline: none; }"
		"QWidget { color: @fg; font-family: \"Inter\",\"Segoe UI Variable Text\",\"Segoe UI\","
		"\"Microsoft YaHei UI\",\"Microsoft YaHei\",sans-serif; font-size: 14px; }"
		"QDialog, QMainWindow { background: @bg; }"
		"QLabel { background: transparent; }"

		// ---- type ramp -------------------------------------------------------
		// BoardUI: Title 20/600, Headline 16/600, Body 14, Body2 13, Caption 12.
		"QLabel[role=\"title\"] { font-size: 20px; font-weight: 600; }"
		"QLabel[role=\"muted\"] { color: @muted-fg; }"
		"QLabel[role=\"hint\"] { color: @muted-fg; font-size: 13px; }"
		"QLabel[role=\"caption\"] { color: @tertiary-fg; font-size: 12px; }"
		"QLabel[role=\"mono\"] { font-family: \"Cascadia Mono\",Consolas,monospace; }"
		"QLabel#liteCardTitle { color: @fg; font-size: 16px; font-weight: 600; }"
		"QLabel#liteCardDescription { color: @muted-fg; font-size: 13px; }"
		"QLabel#liteFieldLabel { color: @fg; font-size: 14px; font-weight: 500; }"

		// ---- surfaces --------------------------------------------------------
		"QFrame#liteCard { background: @card; border: 1px solid @border; border-radius: 12px; }"
		"QFrame#liteHLine { border: none; background: @border; max-height: 1px; min-height: 1px; }"
		"QFrame#liteVLine { border: none; background: @border; max-width: 1px; min-width: 1px; }"

		// ---- segmented control (settings tabs) -------------------------------
		"QWidget#liteSegmented { background: @muted; border: 1px solid @border; border-radius: 12px; }"
		"QWidget#liteSegmented QPushButton { background: transparent; border: 1px solid transparent;"
		"  border-radius: 8px; color: @muted-fg; font-size: 13px; font-weight: 500;"
		"  padding: 0 12px; min-width: 0; min-height: 0; }"
		"QWidget#liteSegmented QPushButton:hover { color: @fg; }"
		"QWidget#liteSegmented QPushButton:checked { background: @raised; color: @fg;"
		"  border: 1px solid @border; }"
		"QWidget#liteSegmented QPushButton:disabled { color: @tertiary-fg; }"

		// ---- buttons ---------------------------------------------------------
		"QPushButton { background: @card; color: @fg; border: 1px solid @input; border-radius: 10px;"
		"  padding: 6px 14px; min-height: 22px; min-width: 64px; font-size: 14px; font-weight: 500; }"
		"QPushButton:hover { background: @hover; }"
		"QPushButton:pressed { background: @pressed; }"
		"QPushButton:focus { border-color: @ring; }"
		"QPushButton:disabled { color: @tertiary-fg; background: @muted; border-color: @border; }"
		"QPushButton[accent=\"true\"] { background: @primary-grad; color: @primary-fg;"
		"  border: 1px solid @primary-border; }"
		"QPushButton[accent=\"true\"]:hover { background: @primary-grad-hover; }"
		"QPushButton[accent=\"true\"]:pressed { background: @primary-grad-pressed; }"
		"QPushButton[accent=\"true\"]:disabled { background: @muted; color: @tertiary-fg;"
		"  border-color: @border; }"
		"QPushButton[variant=\"secondary\"] { background: @muted; border-color: transparent; }"
		"QPushButton[variant=\"secondary\"]:hover { background: @pressed; }"
		"QPushButton[variant=\"ghost\"] { background: transparent; border-color: transparent;"
		"  color: @muted-fg; }"
		"QPushButton[variant=\"ghost\"]:hover { background: @hover; color: @fg; }"
		"QPushButton[variant=\"destructive\"] { background: transparent; color: @destructive;"
		"  border: 1px solid @border; }"
		"QPushButton[variant=\"destructive\"]:hover { background: @destructive-soft; border-color: @destructive-soft; }"
		"QPushButton[size=\"sm\"] { min-height: 18px; padding: 4px 10px; font-size: 13px; border-radius: 8px; }"
		"QDialogButtonBox QPushButton { min-width: 76px; }"

		// ---- text inputs -----------------------------------------------------
		"QLineEdit, QPlainTextEdit, QTextEdit, QComboBox {"
		"  background: @field; color: @fg; border: 1px solid @input; border-radius: 10px;"
		"  padding: 6px 10px; min-height: 22px; font-size: 14px;"
		"  selection-background-color: @primary; selection-color: @primary-fg; }"
		"QLineEdit:hover, QPlainTextEdit:hover, QTextEdit:hover, QComboBox:hover { border-color: @border-strong; }"
		"QLineEdit:focus, QPlainTextEdit:focus, QTextEdit:focus, QComboBox:focus { border-color: @ring; }"
		"QLineEdit:disabled, QPlainTextEdit:disabled, QTextEdit:disabled, QComboBox:disabled {"
		"  color: @tertiary-fg; background: @muted; }"
		"QComboBox { padding-right: 26px; }"
		"QComboBox::drop-down { border: none; width: 26px; subcontrol-origin: padding;"
		"  subcontrol-position: center right; }"
		"QComboBox::down-arrow { image: url(:/icons/ui/chevron-down-@ink.svg); width: 14px; height: 14px; }"
		"QComboBox QAbstractItemView { background: @raised; color: @fg; border: 1px solid @border;"
		"  border-radius: 12px; padding: 5px; outline: none;"
		"  selection-background-color: @hover; selection-color: @fg; }"
		"QComboBox QAbstractItemView::item { min-height: 28px; padding: 2px 8px; border-radius: 8px; }"
		// Spin boxes keep the stock Fusion text layout: a stylesheet padding here
		// makes Qt paint the value twice, offset by the padding.
		"QSpinBox, QDoubleSpinBox { background: @field; color: @fg; border: 1px solid @input;"
		"  border-radius: 10px; min-height: 22px; font-size: 14px;"
		"  font-family: \"Cascadia Mono\",Consolas,monospace;"
		"  selection-background-color: @primary; selection-color: @primary-fg; }"
		"QSpinBox:hover, QDoubleSpinBox:hover { border-color: @border-strong; }"
		"QSpinBox:focus, QDoubleSpinBox:focus { border-color: @ring; }"
		"QSpinBox:disabled, QDoubleSpinBox:disabled { color: @tertiary-fg; background: @muted; }"
		"QSpinBox::up-button, QDoubleSpinBox::up-button { subcontrol-origin: border;"
		"  subcontrol-position: top right; width: 20px; border: none; background: transparent;"
		"  margin: 3px 3px 0 0; }"
		"QSpinBox::down-button, QDoubleSpinBox::down-button { subcontrol-origin: border;"
		"  subcontrol-position: bottom right; width: 20px; border: none; background: transparent;"
		"  margin: 0 3px 3px 0; }"
		"QSpinBox::up-arrow, QDoubleSpinBox::up-arrow { image: url(:/icons/ui/chevron-up-@ink.svg);"
		"  width: 11px; height: 11px; }"
		"QSpinBox::down-arrow, QDoubleSpinBox::down-arrow { image: url(:/icons/ui/chevron-down-@ink.svg);"
		"  width: 11px; height: 11px; }"
		"QTextEdit#liteOcrText, QPlainTextEdit#liteOcrText { background: @muted;"
		"  border: 1px solid @border; border-radius: 10px; padding: 12px; }"

		// ---- selection controls ---------------------------------------------
		"QCheckBox, QRadioButton { background: transparent; spacing: 9px; color: @fg; font-size: 14px; }"
		"QCheckBox::indicator { width: 16px; height: 16px; border: 1px solid @border-strong;"
		"  border-radius: 5px; background: @card; }"
		"QCheckBox::indicator:hover { border-color: @primary-border; }"
		"QCheckBox::indicator:checked { background: @primary; border-color: @primary; image: url(@check-icon); }"
		"QCheckBox::indicator:disabled { background: @muted; border-color: @border; }"
		"QRadioButton::indicator { width: 16px; height: 16px; border: 1px solid @border-strong;"
		"  border-radius: 8px; background: @card; }"
		"QRadioButton::indicator:hover { border-color: @primary-border; }"
		"QRadioButton::indicator:checked { background: @primary; border-color: @primary;"
		"  image: url(@radio-icon); }"

		// ---- tabs ------------------------------------------------------------
		"QTabWidget::pane { background: transparent; border: none; margin-top: 12px; }"
		"QTabBar { background: @muted; border: 1px solid @border; border-radius: 12px; }"
		"QTabBar::tab { background: transparent; color: @muted-fg; padding: 7px 16px;"
		"  margin: 3px 2px; border: 1px solid transparent; border-radius: 8px;"
		"  font-weight: 500; font-size: 13px; }"
		"QTabBar::tab:hover { color: @fg; }"
		"QTabBar::tab:selected { background: @raised; color: @fg; border: 1px solid @border; }"
		"QTabBar::tab:disabled { color: @tertiary-fg; }"

		// ---- scroll ----------------------------------------------------------
		"QScrollArea, QScrollArea > QWidget > QWidget { background: transparent; border: none; }"
		"QScrollBar:vertical { background: transparent; width: 10px; margin: 0; }"
		"QScrollBar::handle:vertical { background: @border-strong; border-radius: 3px; min-height: 36px; margin: 2px; }"
		"QScrollBar::handle:vertical:hover { background: @tertiary-fg; }"
		"QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
		"QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }"
		"QScrollBar:horizontal { background: transparent; height: 10px; margin: 0; }"
		"QScrollBar::handle:horizontal { background: @border-strong; border-radius: 3px; min-width: 36px; margin: 2px; }"
		"QScrollBar::handle:horizontal:hover { background: @tertiary-fg; }"
		"QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }"
		"QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal { background: transparent; }"

		// ---- menus & tooltips ------------------------------------------------
		"QMenu { background: @raised; color: @fg; border: 1px solid @border; border-radius: 12px; padding: 5px; }"
		"QMenu::item { padding: 7px 26px 7px 10px; border-radius: 8px; background: transparent;"
		"  min-height: 18px; font-size: 14px; }"
		"QMenu::item:selected { background: @hover; }"
		"QMenu::item:disabled { color: @tertiary-fg; }"
		"QMenu::separator { height: 1px; background: @border; margin: 5px 2px; }"
		"QMenu::icon { left: 8px; }"
		"QToolTip { background: @tooltip-bg; color: @tooltip-fg; border: none; padding: 6px 9px;"
		"  border-radius: 8px; font-size: 12px; }"

		// ---- data views ------------------------------------------------------
		"QTableWidget, QTableView, QListWidget, QListView, QTreeWidget, QTreeView {"
		"  background: @card; color: @fg; border: 1px solid @border; border-radius: 12px;"
		"  alternate-background-color: @muted; selection-background-color: @hover;"
		"  selection-color: @fg; }"
		"QHeaderView::section { background: @muted; color: @muted-fg; border: none;"
		"  border-bottom: 1px solid @border; padding: 8px 12px; font-weight: 500; }"

		// ---- group boxes -----------------------------------------------------
		"QGroupBox { background: @card; border: 1px solid @border; border-radius: 12px;"
		"  margin-top: 18px; padding: 14px 16px 16px 16px; font-weight: 600; }"
		"QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; left: 14px;"
		"  top: 0px; padding: 0 6px; color: @muted-fg; font-weight: 500; font-size: 13px; }"
		// Nested section already rendered by a Card header: drop the extra frame.
		"QGroupBox#liteFlatGroup { border: none; background: transparent; margin: 0px; padding: 0px; }"

		// ---- status labels ---------------------------------------------------
		"QLabel[tone=\"success\"] { color: @success; background: @success-soft; border-radius: 8px;"
		"  padding: 5px 9px; font-size: 13px; }"
		"QLabel[tone=\"destructive\"] { color: @destructive; background: @destructive-soft;"
		"  border-radius: 8px; padding: 5px 9px; font-size: 13px; }"
	);

	return expandTokens(sheet, tokens);
}

QString Theme::toolBarStyleSheet() const
{
	const QVector<QPair<QString, QString>> tokens = {
		{ QStringLiteral("@card"), css(surface()) },
		{ QStringLiteral("@raised"), css(surfaceRaised()) },
		{ QStringLiteral("@border"), css(border()) },
		{ QStringLiteral("@border-strong"), css(borderStrong()) },
		{ QStringLiteral("@hover"), css(hover()) },
		{ QStringLiteral("@pressed"), css(pressed()) },
		{ QStringLiteral("@primary"), css(accent()) },
		{ QStringLiteral("@primary-hover"), css(accentHover()) },
		{ QStringLiteral("@primary-soft"), css(accentSoft()) },
		{ QStringLiteral("@primary-border"), css(accentBorder()) },
		{ QStringLiteral("@primary-grad"), gradient(accent(), accentDeep()) },
		{ QStringLiteral("@primary-grad-hover"), gradient(accentHover(), accent()) },
		{ QStringLiteral("@primary-grad-pressed"), gradient(accentDeep(), accentDeep()) },
		{ QStringLiteral("@destructive-soft"), css(withAlpha(destructive(), mDark ? 46 : 22)) }
	};

	// BoardUI toolbar: 14px container radius, 10px icon-button radius, 34px hit
	// targets and a gradient primary action at the end of the row.
	const QString sheet = QStringLiteral(
		"QWidget#liteToolBar { background: @raised; border: 1px solid @border; border-radius: 14px; }"
		"QToolButton { background: transparent; border: 1px solid transparent; border-radius: 10px;"
		"  padding: 0px; }"
		"QToolButton:hover { background: @hover; }"
		"QToolButton:pressed { background: @pressed; }"
		"QToolButton:checked { background: @primary-soft; border-color: @primary-border; }"
		"QToolButton:disabled { background: transparent; }"
		"QToolButton#liteDiscard { border-radius: 10px; }"
		"QToolButton#liteDiscard:hover { background: @destructive-soft; }"
		"QToolButton#liteConfirm { background: @primary-grad; border: 1px solid @primary-border;"
		"  border-radius: 10px; }"
		"QToolButton#liteConfirm:hover { background: @primary-grad-hover; }"
		"QToolButton#liteConfirm:pressed { background: @primary-grad-pressed; }"
		"QFrame#liteSeparator { background: @border; border: none; }"
	);

	return expandTokens(sheet, tokens);
}

QString Theme::stripStyleSheet() const
{
	return QStringLiteral(
		"QWidget#liteSettingsStrip { background: %1; border: 1px solid %2; border-radius: 12px; }"
	).arg(css(surfaceRaised()), css(border()));
}
