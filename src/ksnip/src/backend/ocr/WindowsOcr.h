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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef KSNIP_WINDOWSOCR_H
#define KSNIP_WINDOWSOCR_H

#include <QObject>
#include <QImage>
#include <QProcess>
#include <QTimer>

class WindowsOcr : public QObject
{
	Q_OBJECT
public:
	explicit WindowsOcr(QObject *parent = nullptr);
	~WindowsOcr() override;

	void recognize(const QImage &image);
	void cancel();
	bool isBusy() const;

signals:
	void finished(const QString &text);
	void failed(const QString &error);

private:
	QProcess mProcess;
	QTimer mTimeoutTimer;
	QString mTempPath;
	bool mCancelled;

	void removeTempFile();
};

#endif //KSNIP_WINDOWSOCR_H