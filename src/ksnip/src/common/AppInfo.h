/*
 * 拾帧 (Shizhen) - 轻量截图与标注工具
 *
 * 品牌与路径信息集中定义，避免在代码各处硬编码应用名。
 * 注意：QCoreApplication::applicationName() 保持 ASCII（"Shizhen"），
 * 用于配置目录、日志、单实例键等内部标识；界面上统一使用 displayName()。
 */

#ifndef SHIZHEN_APPINFO_H
#define SHIZHEN_APPINFO_H

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QString>

inline namespace AppInfo {

inline QString displayName()
{
	return QStringLiteral("拾帧");
}

inline QString tagline()
{
	return QStringLiteral("截图与标注工具");
}

inline QString organizationName()
{
	return QStringLiteral("Shizhen");
}

/**
 * 便携模式：配置写在可执行文件同级的 config.ini，
 * 打包拷走时配置跟着走，不碰注册表。
 *
 * 若程序目录只读（例如装在 Program Files），回退到用户配置目录，
 * 保证程序仍能保存设置而不至于静默失败。
 */
inline QString configFilePath()
{
	static const auto path = [] {
		const auto appDir = QCoreApplication::applicationDirPath();
		const auto candidate = appDir + QStringLiteral("/config.ini");

		if (QFileInfo::exists(candidate)) {
			return candidate;
		}

		QFile probe(appDir + QStringLiteral("/.shizhen-write-test"));
		if (probe.open(QIODevice::WriteOnly)) {
			probe.close();
			probe.remove();
			return candidate;
		}

		auto fallbackDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
		QDir().mkpath(fallbackDir);
		return fallbackDir + QStringLiteral("/config.ini");
	}();

	return path;
}

} // namespace AppInfo

#endif // SHIZHEN_APPINFO_H
