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

#ifndef KSNIP_CAPTUREWINDOW_H
#define KSNIP_CAPTUREWINDOW_H

#include <QWidget>
#include <QToolButton>
#include <QButtonGroup>
#include <QVector>
#include <QPair>
#include <QElapsedTimer>
#include <QPointer>

#include "src/gui/imageAnnotator/KImageAnnotatorAdapter.h"
#include "src/backend/ocr/OllamaOcr.h"
#include "src/backend/ocr/WindowsOcr.h"
#include "src/gui/INotificationService.h"
#include "src/gui/clipboard/IClipboard.h"
#include "src/common/dtos/CaptureDto.h"
#include "src/dependencyInjector/DependencyInjector.h"

class OcrResultDialog;

class CaptureWindow : public QWidget
{
	Q_OBJECT
public:
	CaptureWindow(
			const CaptureDto &capture,
			DependencyInjector *dependencyInjector,
			const QSharedPointer<INotificationService> &notificationService,
			QWidget *parent = nullptr);
	~CaptureWindow() override;

protected:
	void keyPressEvent(QKeyEvent *event) override;
	void showEvent(QShowEvent *event) override;
	void closeEvent(QCloseEvent *event) override;
	bool event(QEvent *event) override;
	bool eventFilter(QObject *watched, QEvent *event) override;

private:
	DependencyInjector *mDependencyInjector;
	QSharedPointer<IConfig> mConfig;
	QSharedPointer<INotificationService> mNotificationService;
	QSharedPointer<IClipboard> mClipboard;
	KImageAnnotatorAdapter *mAnnotator;
	OllamaOcr *mOcr;
	WindowsOcr *mWinOcr;
	QWidget *mToolBar;
	QWidget *mSettingsStrip;
	QWidget *mBorderOverlay;
	QButtonGroup *mToolGroup;
	QToolButton *mOcrButton;
	QToolButton *mMoveButton;
	QToolButton *mDiscardButton;
	QToolButton *mConfirmButton;
	QPointer<OcrResultDialog> mOcrDialog;
	QWidget *mMoveTarget;
	QRect mCaptureRect;
	QSize mImageSize;
	QPoint mDragOffset;
	QPoint mBarPressPos;
	QElapsedTimer mBarPressTimer;
	bool mIsDragging;
	bool mUserMoved;
	bool mIsClosing;
	QVector<QPair<QToolButton*, QString>> mBarButtons;

	void setupUi(const CaptureDto &capture);
	void setupToolBar();
	bool handleDrag(QWidget *source, QEvent *event);
	QToolButton *addBarButton(const QString &iconName, const QString &toolTip);
	void applyTheme();
	void addSeparator();
	void selectTool(kImageAnnotator::AnnotationTool tool);
	void setMoveMode(bool on);
	void updateGeometry();
	void alignToCaptureRect();
	void paintBorder();
	void save();
	void confirm();
	void discard();
	void runOcr();
	void ocrFinished(const QString &text);
	void ocrFailed(const QString &error);
	void setOcrBusy(bool busy);
	void showOcrResult(const QString &text);
	void restoreAfterOcrDialog();
	void closeOcrDialog();
	void releaseMouseGrab();
};

#endif //KSNIP_CAPTUREWINDOW_H