/*
 * Copyright (C) 2017 Damir Porobic <https://github.com/damirporobic>
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

#if  defined(_WIN32) && defined(QT_NO_DEBUG)
// Prevent starting console in background under windows
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#endif

#include "BuildConfig.h"
#include "src/bootstrapper/BootstrapperFactory.h"
#include "src/logging/LogOutputHandler.h"
#include "src/gui/style/Theme.h"
#include "src/common/AppInfo.h"
#include <QSettings>

#ifdef Q_OS_WIN
#include <windows.h>
#include <shlobj.h>
#endif

int main(int argc, char** argv)
{
	qInstallMessageHandler(LogOutputHandler::handleOutput);

	QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QApplication app(argc, argv);
    app.setAttribute(Qt::AA_UseHighDpiPixmaps);

    // 内部标识保持 ASCII，界面显示名由 AppInfo::displayName() 提供
    app.setOrganizationName(AppInfo::organizationName());
    app.setOrganizationDomain(QLatin1String("shizhen.app"));
    app.setApplicationName(QLatin1String("Shizhen"));
    app.setApplicationDisplayName(AppInfo::displayName());
    app.setApplicationVersion(QLatin1String(KSNIP_VERSION));
    app.setDesktopFileName(QLatin1String("app.shizhen.Shizhen.desktop"));

    // 便携模式：所有设置（含标注工具的设置）统一写到程序目录的 config.ini，
    // 不写注册表，整包拷走配置跟着走。
    QSettings::setDefaultFormat(QSettings::IniFormat);
    qputenv("SHIZHEN_CONFIG", AppInfo::configFilePath().toLocal8Bit());

    auto dependencyInjector = new DependencyInjector;
	DependencyInjectorBootstrapper::BootstrapCore(dependencyInjector);

	app.setStyle(dependencyInjector->get<IConfig>()->applicationStyle());
	Theme::instance()->apply(&app, dependencyInjector->get<IConfig>()->appearanceTheme());

#ifdef Q_OS_WIN
	// Shell caches exe icons per path; notify so replaced icons refresh in taskbar/jump lists.
	SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATHW, reinterpret_cast<LPCWSTR>(QCoreApplication::applicationFilePath().utf16()), nullptr);
#endif

	BootstrapperFactory bootstrapperFactory;
	return bootstrapperFactory.create(dependencyInjector)->start(app);
}


