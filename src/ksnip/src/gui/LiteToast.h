/*
 * Copyright (C) 2020 Damir Porobic <damir.porobic@gmx.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef KSNIP_LITETOAST_H
#define KSNIP_LITETOAST_H

#include <QWidget>

class QLabel;

// Shell tray balloons are suppressed by Windows focus assist / do-not-disturb,
// so launch feedback uses a self-drawn topmost window instead.
class LiteToast : public QWidget
{
	Q_OBJECT
public:
	static void show(const QString &title, const QString &message);
	static void dismiss();

protected:
	void mousePressEvent(QMouseEvent *event) override;
	void paintEvent(QPaintEvent *event) override;

private:
	explicit LiteToast(const QString &title, const QString &message);

	QLabel *mIconLabel;
	QLabel *mTitleLabel;
	QLabel *mMessageLabel;
};

#endif //KSNIP_LITETOAST_H