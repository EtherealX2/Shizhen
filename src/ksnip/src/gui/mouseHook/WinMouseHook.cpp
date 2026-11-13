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

#include "WinMouseHook.h"

#include <QDebug>

#ifdef Q_OS_WIN

WinMouseHook *WinMouseHook::sInstance = nullptr;

WinMouseHook::WinMouseHook(const QSharedPointer<IConfig> &config, QObject *parent) :
	QObject(parent),
	mConfig(config),
	mEnabled(true),
	mHook(nullptr)
{
	sInstance = this;
	mHook = SetWindowsHookExW(WH_MOUSE_LL, mouseProc, GetModuleHandleW(nullptr), 0);
	if(mHook == nullptr) {
		qWarning("WinMouseHook: Failed to install mouse hook, error %lu", GetLastError());
	} else {
		qDebug("WinMouseHook: mouse hook installed");
	}
}

WinMouseHook::~WinMouseHook()
{
	if(mHook != nullptr) {
		UnhookWindowsHookEx(mHook);
		mHook = nullptr;
	}
	if(sInstance == this) {
		sInstance = nullptr;
	}
}

void WinMouseHook::setEnabled(bool enabled)
{
	mEnabled = enabled;
}

LRESULT CALLBACK WinMouseHook::mouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if(nCode >= 0 && sInstance != nullptr && sInstance->mConfig->isDebugEnabled() &&
		(wParam == WM_XBUTTONDOWN || wParam == WM_LBUTTONDOWN)) {
		auto info = reinterpret_cast<MSLLHOOKSTRUCT*>(lParam);
		qDebug("WinMouseHook: msg=0x%llx mouseData=0x%lx enabled=%d", (unsigned long long)wParam, info->mouseData, sInstance->mEnabled);
	}
	if(nCode >= 0 && sInstance != nullptr && sInstance->mEnabled && wParam == WM_XBUTTONDOWN) {
		auto info = reinterpret_cast<MSLLHOOKSTRUCT*>(lParam);
		auto button = HIWORD(info->mouseData);

		auto mode = -1;
		if(button == XBUTTON1) {
			mode = sInstance->mConfig->mouseSideButton1Mode();
		} else if(button == XBUTTON2) {
			mode = sInstance->mConfig->mouseSideButton2Mode();
		}

		if(sInstance->mConfig->isDebugEnabled()) {
			qDebug("WinMouseHook: XBUTTONDOWN button=%u mode=%d hookEnabled=%d", button, mode, sInstance->mConfig->mouseHookEnabled());
		}

		if(mode >= 0 && sInstance->mConfig->mouseHookEnabled()) {
			auto captureMode = static_cast<CaptureModes>(mode);
			QMetaObject::invokeMethod(sInstance, [captureMode](){
				emit sInstance->captureTriggered(captureMode);
			}, Qt::QueuedConnection);
			return 1;
		}
	}
	return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

#else

WinMouseHook::WinMouseHook(const QSharedPointer<IConfig> &config, QObject *parent) :
	QObject(parent),
	mConfig(config),
	mEnabled(true)
{
}

WinMouseHook::~WinMouseHook() = default;

void WinMouseHook::setEnabled(bool enabled)
{
	mEnabled = enabled;
}

#endif