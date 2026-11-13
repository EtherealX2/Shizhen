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

#ifndef KSNIP_WINMOUSEHOOK_H
#define KSNIP_WINMOUSEHOOK_H

#include <QObject>
#include <QSharedPointer>

#include "src/backend/config/IConfig.h"
#include "src/common/enum/CaptureModes.h"

#ifdef Q_OS_WIN
#include <windows.h>
#endif

class WinMouseHook : public QObject
{
	Q_OBJECT
public:
	explicit WinMouseHook(const QSharedPointer<IConfig> &config, QObject *parent = nullptr);
	~WinMouseHook() override;

	void setEnabled(bool enabled);

signals:
	void captureTriggered(CaptureModes captureMode);

private:
	QSharedPointer<IConfig> mConfig;
	bool mEnabled;

#ifdef Q_OS_WIN
	HHOOK mHook;
	static WinMouseHook *sInstance;
	static LRESULT CALLBACK mouseProc(int nCode, WPARAM wParam, LPARAM lParam);
#endif
};

#endif //KSNIP_WINMOUSEHOOK_H