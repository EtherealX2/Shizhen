/*
 * Copyright (C) 2022 Damir Porobic <damir.porobic@gmx.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "SettingsAdapter.h"

namespace kImageAnnotator {

/**
 * 默认使用 Qt 的平台默认位置（Windows 下为注册表）。
 * 若宿主程序通过 SHIZHEN_CONFIG 指定了 ini 路径，则写入该文件，
 * 用于便携模式下让标注工具的设置也跟随程序目录。
 */
SettingsAdapter::SettingsAdapter()
{
	const auto configPath = qEnvironmentVariable("SHIZHEN_CONFIG");
	if (configPath.isEmpty()) {
		mSettings.reset(new QSettings());
	} else {
		mSettings.reset(new QSettings(configPath, QSettings::IniFormat));
	}
}

void SettingsAdapter::sync()
{
	mSettings->sync();
}

void SettingsAdapter::setValue(const QString &key, const QVariant &value)
{
	mSettings->setValue(key, value);
}

QVariant SettingsAdapter::value(const QString &key, const QVariant &defaultValue) const
{
	return mSettings->value(key, defaultValue);
}

} // namespace kImageAnnotator
