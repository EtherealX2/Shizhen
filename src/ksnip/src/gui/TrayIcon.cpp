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

#include "TrayIcon.h"
#include "src/common/AppInfo.h"

#include "src/gui/style/Theme.h"

TrayIcon::TrayIcon(const QSharedPointer<IConfig> &config, const QSharedPointer<IIconLoader> &iconLoader, QObject *parent) :
	QSystemTrayIcon(parent),
	mConfig(config),
	mSettingsAction(nullptr),
	mQuitAction(nullptr)
{
	auto icon = iconLoader->loadForTheme(QLatin1String("ksnip.svg"));
	setIcon(icon);
	setToolTip(AppInfo::displayName());

	connect(Theme::instance(), &Theme::changed, this, [this, iconLoader](){
		setIcon(iconLoader->loadForTheme(QLatin1String("ksnip.svg")));
	});

	connect(this, &QSystemTrayIcon::activated, this, &TrayIcon::activatedDefaultAction);
	connect(this, &QSystemTrayIcon::messageClicked, this, &TrayIcon::openContentUrl);
}

void TrayIcon::setupMenu()
{
	// Native popups are rectangular; a translucent window lets the rounded card
	// styling from Theme paint with real corners.
	if(!mMenu.testAttribute(Qt::WA_TranslucentBackground)) {
		mMenu.setAttribute(Qt::WA_TranslucentBackground, true);
		mMenu.setWindowFlag(Qt::FramelessWindowHint, true);
	}
	mMenu.clear();
	for(auto captureAction : mCaptureActions) {
		mMenu.addAction(captureAction);
	}
	mMenu.addSeparator();
	if(mSettingsAction != nullptr) {
		mMenu.addAction(mSettingsAction);
	}
	mMenu.addSeparator();
	if(mQuitAction != nullptr) {
		mMenu.addAction(mQuitAction);
	}
	setContextMenu(&mMenu);
}

void TrayIcon::setCaptureActions(const QList<QAction*> &captureActions)
{
	mCaptureActions = captureActions;
}

void TrayIcon::setSettingsAction(QAction *action)
{
	mSettingsAction = action;
}

void TrayIcon::setQuitAction(QAction *action)
{
	mQuitAction = action;
}

void TrayIcon::setEnabled(bool enabled)
{
	if(enabled) {
		setupMenu();
		show();
	} else {
		hide();
	}
}

void TrayIcon::showInfo(const QString &title, const QString &message, const QString &contentUrl)
{
	showMessage(title, message, contentUrl, QSystemTrayIcon::Information);
}

void TrayIcon::showWarning(const QString &title, const QString &message, const QString &contentUrl)
{
	showMessage(title, message, contentUrl, QSystemTrayIcon::Warning);
}

void TrayIcon::showCritical(const QString &title, const QString &message, const QString &contentUrl)
{
	showMessage(title, message, contentUrl, QSystemTrayIcon::Critical);
}

void TrayIcon::showMessage(const QString &title, const QString &message, const QString &contentUrl, QSystemTrayIcon::MessageIcon messageIcon)
{
	mToastContentUrl = PathHelper::extractParentDirectory(contentUrl);
	QSystemTrayIcon::showMessage(title, message, messageIcon);
}

void TrayIcon::activatedDefaultAction(ActivationReason reason) const
{
	if(reason != ActivationReason::Context) {
		triggerDefaultCaptureMode();
	}
}

void TrayIcon::triggerDefaultCaptureMode() const
{
	auto captureMode = mConfig->defaultTrayIconCaptureMode();
	for(auto action : mCaptureActions) {
		if (action->data().value<CaptureModes>() == captureMode) {
			action->trigger();
			return;
		}
	}
	if(!mCaptureActions.isEmpty()) {
		mCaptureActions.first()->trigger();
	}
}

void TrayIcon::openContentUrl()
{
	if(!mToastContentUrl.isEmpty() && !mToastContentUrl.isNull()) {
		QDesktopServices::openUrl(mToastContentUrl);
	}
}
