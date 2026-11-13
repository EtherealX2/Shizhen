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

#include "CaptureWindow.h"
#include "OcrResultDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QEvent>
#include <QCloseEvent>
#include <QMouseEvent>
#include <QShowEvent>
#include <QScreen>
#include <QGuiApplication>
#include <QApplication>
#include <QCursor>
#include <QTimer>
#include <QElapsedTimer>
#include <QFrame>
#include <QPainter>
#include <QGraphicsView>
#include <QGraphicsDropShadowEffect>

#include "src/gui/operations/SaveOperation.h"
#include "src/gui/style/Theme.h"
#include "src/gui/clipboard/IClipboard.h"
#include "src/common/loader/IIconLoader.h"
#include "src/backend/saver/IImageSaver.h"
#include "src/backend/saver/ISavePathProvider.h"
#include "src/backend/recentImages/IRecentImageService.h"
#include "src/common/adapter/fileDialog/IFileDialogService.h"

using kImageAnnotator::AnnotationTool;

CaptureWindow::CaptureWindow(
		const CaptureDto &capture,
		DependencyInjector *dependencyInjector,
		const QSharedPointer<INotificationService> &notificationService,
		QWidget *parent) :
	QWidget(parent, Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool),
	mDependencyInjector(dependencyInjector),
	mConfig(mDependencyInjector->get<IConfig>()),
	mNotificationService(notificationService),
	mClipboard(mDependencyInjector->get<IClipboard>()),
	mAnnotator(new KImageAnnotatorAdapter),
	mOcr(new OllamaOcr(mConfig, this)),
	mWinOcr(new WindowsOcr(this)),
	mToolBar(nullptr),
	mSettingsStrip(nullptr),
	mBorderOverlay(nullptr),
	mToolGroup(new QButtonGroup(this)),
	mOcrButton(nullptr),
	mMoveButton(nullptr),
	mDiscardButton(nullptr),
	mConfirmButton(nullptr),
	mMoveTarget(nullptr),
	mIsDragging(false),
	mUserMoved(false),
	mIsClosing(false)
{
	setAttribute(Qt::WA_DeleteOnClose);
	setAttribute(Qt::WA_TranslucentBackground);
	setWindowTitle(tr("截图"));

	setupUi(capture);

	connect(mOcr, &OllamaOcr::finished, this, &CaptureWindow::ocrFinished);
	connect(mOcr, &OllamaOcr::failed, this, &CaptureWindow::ocrFailed);
	connect(mWinOcr, &WindowsOcr::finished, this, &CaptureWindow::ocrFinished);
	connect(mWinOcr, &WindowsOcr::failed, this, &CaptureWindow::ocrFailed);
	connect(Theme::instance(), &Theme::changed, this, &CaptureWindow::applyTheme);
}

CaptureWindow::~CaptureWindow()
{
	delete mAnnotator;
}

void CaptureWindow::setupUi(const CaptureDto &capture)
{
	mCaptureRect = capture.captureRect;

	auto pixmap = capture.screenshot;
	auto screen = QGuiApplication::screenAt(capture.captureRect.center());
	if(screen == nullptr) {
		screen = QGuiApplication::primaryScreen();
	}
	auto available = screen->availableGeometry();

	auto dpr = pixmap.devicePixelRatio();
	if(dpr <= 0) {
		dpr = 1;
	}
	mImageSize = QSize(qRound(pixmap.width() / dpr), qRound(pixmap.height() / dpr));
	// Reserve the window chrome (margins, spacing, toolbar, settings strip);
	// otherwise a tall capture pushes the close buttons below the screen edge.
	auto maxWidth = available.width() - 40;
	auto maxHeight = available.height() - 150;
	if(mImageSize.width() > maxWidth || mImageSize.height() > maxHeight) {
		pixmap = pixmap.scaled(qRound(maxWidth * dpr), qRound(maxHeight * dpr),
							   Qt::KeepAspectRatio, Qt::SmoothTransformation);
		mImageSize = QSize(qRound(pixmap.width() / dpr), qRound(pixmap.height() / dpr));
	}

	mAnnotator->loadImage(pixmap);
	if(capture.isCursorValid()) {
		mAnnotator->insertImageItem(capture.cursor.position, capture.cursor.image);
	}
	mAnnotator->setTabBarAutoHide(true);
	mAnnotator->setSaveToolSelection(false);
	mAnnotator->setSwitchToSelectToolAfterDrawingItem(false);
	mAnnotator->setSelectItemAfterDrawing(false);
	mAnnotator->setToolPickerVisible(false);
	mAnnotator->setGeneralSettingsVisible(false);
	mAnnotator->setItemSettingsVisible(false);
	mAnnotator->setCompactView(true);
	mAnnotator->setTool(AnnotationTool::Select);
	mAnnotator->widget()->setStyleSheet(QLatin1String(
		"QTabWidget::pane { border: none; }"));
	mSettingsStrip = new QWidget(this);
	mSettingsStrip->setObjectName(QLatin1String("liteSettingsStrip"));
	mSettingsStrip->setStyleSheet(Theme::instance()->stripStyleSheet());
	auto stripLayout = new QHBoxLayout(mSettingsStrip);
	stripLayout->setContentsMargins(9, 3, 9, 3);
	for(auto child : mAnnotator->widget()->findChildren<QWidget*>()) {
		if(QLatin1String(child->metaObject()->className()) == QLatin1String("kImageAnnotator::AnnotationItemSettings")) {
			child->setParent(mSettingsStrip);
			stripLayout->addWidget(child);
			break;
		}
	}
	mSettingsStrip->hide();

	// Global QSS paints container backgrounds; collapse the annotator's layout margins so no background ring shows around the canvas.
	mAnnotator->widget()->layout()->setContentsMargins(0, 0, 0, 0);
	for(auto layout : mAnnotator->widget()->findChildren<QLayout*>()) {
		layout->setContentsMargins(0, 0, 0, 0);
	}

	setupToolBar();

	auto layout = new QVBoxLayout(this);
	layout->setContentsMargins(14, 14, 14, 14);
	layout->setSpacing(11);
	layout->addWidget(mAnnotator->widget(), 0, Qt::AlignHCenter);
	layout->addWidget(mSettingsStrip, 0, Qt::AlignHCenter);
	layout->addWidget(mToolBar, 0, Qt::AlignHCenter);

	// Border must paint above the annotator canvas, which can cover window-level painting.
	mBorderOverlay = new QWidget(this);
	mBorderOverlay->setAttribute(Qt::WA_TransparentForMouseEvents);
	mBorderOverlay->setAttribute(Qt::WA_TranslucentBackground);
	mBorderOverlay->installEventFilter(this);
	mBorderOverlay->raise();

	updateGeometry();
}

void CaptureWindow::setupToolBar()
{
	mToolBar = new QWidget(this);
	mToolBar->setObjectName(QLatin1String("liteToolBar"));
	mToolBar->setStyleSheet(Theme::instance()->toolBarStyleSheet());
	mToolBar->installEventFilter(this);
	mToolBar->setCursor(Qt::SizeAllCursor);

	auto shadow = new QGraphicsDropShadowEffect(mToolBar);
	shadow->setBlurRadius(26);
	shadow->setOffset(0, 5);
	shadow->setColor(QColor(0, 0, 0, 46));
	mToolBar->setGraphicsEffect(shadow);

	auto barLayout = new QHBoxLayout(mToolBar);
	barLayout->setContentsMargins(4, 4, 4, 4);
	barLayout->setSpacing(2);

	mToolGroup->setExclusive(true);

	mMoveButton = addBarButton(QLatin1String("move"), tr("移动"));
	mMoveButton->setCheckable(true);
	connect(mMoveButton, &QToolButton::toggled, this, &CaptureWindow::setMoveMode);
	barLayout->addWidget(mMoveButton);
	addSeparator();

	auto addTool = [this, barLayout](const QString &icon, const QString &tip, AnnotationTool tool){
		auto button = addBarButton(icon, tip);
		button->setCheckable(true);
		mToolGroup->addButton(button);
		barLayout->addWidget(button);
		connect(button, &QToolButton::clicked, this, [this, tool](){ selectTool(tool); });
		return button;
	};

	auto selectButton = addTool(QLatin1String("pointer"), tr("选择"), AnnotationTool::Select);
	addTool(QLatin1String("square"), tr("矩形"), AnnotationTool::Rect);
	addTool(QLatin1String("circle"), tr("椭圆"), AnnotationTool::Ellipse);
	addTool(QLatin1String("arrow"), tr("箭头"), AnnotationTool::Arrow);
	addTool(QLatin1String("pencil"), tr("画笔"), AnnotationTool::Pen);
	addTool(QLatin1String("type"), tr("文字"), AnnotationTool::Text);
	addTool(QLatin1String("grid"), tr("马赛克"), AnnotationTool::Pixelate);
	selectButton->setChecked(true);

	addSeparator();

	auto undoAction = mAnnotator->undoAction();
	auto undoButton = addBarButton(QLatin1String("undo"), tr("撤销"));
	undoButton->setEnabled(undoAction->isEnabled());
	connect(undoAction, &QAction::changed, this, [undoButton, undoAction](){ undoButton->setEnabled(undoAction->isEnabled()); });
	connect(undoButton, &QToolButton::clicked, undoAction, &QAction::trigger);
	barLayout->addWidget(undoButton);

	auto redoAction = mAnnotator->redoAction();
	auto redoButton = addBarButton(QLatin1String("redo"), tr("重做"));
	redoButton->setEnabled(redoAction->isEnabled());
	connect(redoAction, &QAction::changed, this, [redoButton, redoAction](){ redoButton->setEnabled(redoAction->isEnabled()); });
	connect(redoButton, &QToolButton::clicked, redoAction, &QAction::trigger);
	barLayout->addWidget(redoButton);

	addSeparator();

	mOcrButton = addBarButton(QLatin1String("ocr"), tr("OCR 文字识别"));
	connect(mOcrButton, &QToolButton::clicked, this, &CaptureWindow::runOcr);
	barLayout->addWidget(mOcrButton);

	auto saveButton = addBarButton(QLatin1String("save"), tr("保存"));
	connect(saveButton, &QToolButton::clicked, this, &CaptureWindow::save);
	barLayout->addWidget(saveButton);

	addSeparator();

	auto discardButton = new QToolButton(mToolBar);
	mDiscardButton = discardButton;
	discardButton->setObjectName(QLatin1String("liteDiscard"));
	discardButton->setIcon(Theme::instance()->icon(QLatin1String("x")));
	discardButton->setIconSize(QSize(18, 18));
	discardButton->setFixedSize(32, 32);
	discardButton->setCursor(Qt::PointingHandCursor);
	discardButton->setToolTip(tr("放弃 (Esc)"));
	discardButton->setAccessibleName(tr("放弃 (Esc)"));
	connect(discardButton, &QToolButton::clicked, this, &CaptureWindow::discard);
	barLayout->addWidget(discardButton);

	auto confirmButton = new QToolButton(mToolBar);
	mConfirmButton = confirmButton;
	confirmButton->setObjectName(QLatin1String("liteConfirm"));
	confirmButton->setIcon(Theme::instance()->onAccentIcon(QLatin1String("check-inverse")));
	confirmButton->setIconSize(QSize(18, 18));
	confirmButton->setFixedSize(32, 32);
	confirmButton->setCursor(Qt::PointingHandCursor);
	confirmButton->setToolTip(tr("完成（复制到剪贴板）"));
	confirmButton->setAccessibleName(tr("完成（复制到剪贴板）"));
	connect(confirmButton, &QToolButton::clicked, this, &CaptureWindow::confirm);
	barLayout->addWidget(confirmButton);
}

QToolButton *CaptureWindow::addBarButton(const QString &iconName, const QString &toolTip)
{
	auto button = new QToolButton(mToolBar);
	if(!iconName.isEmpty()) {
		button->setIcon(Theme::instance()->icon(iconName));
		mBarButtons.append(qMakePair(button, iconName));
	}
	button->setIconSize(QSize(18, 18));
	button->setFixedSize(32, 32);
	button->setToolTip(toolTip);
	button->setAccessibleName(toolTip);
	button->setCursor(Qt::PointingHandCursor);
	return button;
}

void CaptureWindow::applyTheme()
{
	mToolBar->setStyleSheet(Theme::instance()->toolBarStyleSheet());
	mSettingsStrip->setStyleSheet(Theme::instance()->stripStyleSheet());
	for(const auto &entry : mBarButtons) {
		entry.first->setIcon(Theme::instance()->icon(entry.second));
	}
	if(mDiscardButton != nullptr) {
		mDiscardButton->setIcon(Theme::instance()->icon(QLatin1String("x")));
	}
	if(mConfirmButton != nullptr) {
		mConfirmButton->setIcon(Theme::instance()->onAccentIcon(QLatin1String("check-inverse")));
	}
	if(mBorderOverlay != nullptr) {
		mBorderOverlay->update();
	}
}

void CaptureWindow::addSeparator()
{
	auto line = new QFrame(mToolBar);
	line->setObjectName(QLatin1String("liteSeparator"));
	line->setFrameShape(QFrame::VLine);
	line->setFixedSize(1, 20);
	line->setContentsMargins(0, 0, 0, 0);
	auto layout = static_cast<QHBoxLayout*>(mToolBar->layout());
	layout->addSpacing(2);
	layout->addWidget(line, 0, Qt::AlignVCenter);
	layout->addSpacing(2);
}

static QGraphicsView *annotatorCanvasView(QWidget *annotatorWidget)
{
	// The annotator embeds several QGraphicsViews (crop page, rotate/scale dialogs);
	// only the one on the visible page is the image canvas.
	for(auto view : annotatorWidget->findChildren<QGraphicsView*>()) {
		if(view->isVisibleTo(annotatorWidget)) {
			return view;
		}
	}
	return nullptr;
}

void CaptureWindow::selectTool(AnnotationTool tool)
{
	mMoveButton->setChecked(false);
	mAnnotator->setTool(tool);
	mSettingsStrip->setVisible(tool != AnnotationTool::Select);
	updateGeometry();
	QTimer::singleShot(0, this, [this](){ alignToCaptureRect(); });
}

void CaptureWindow::setMoveMode(bool on)
{
	if(mMoveTarget != nullptr) {
		if(mIsDragging) {
			mIsDragging = false;
			mMoveTarget->releaseMouse();
		}
		mMoveTarget->removeEventFilter(this);
		mMoveTarget->unsetCursor();
		mMoveTarget = nullptr;
	}
	if(on) {
		auto annotatorWidget = mAnnotator->widget();
		auto view = annotatorCanvasView(annotatorWidget);
		mMoveTarget = view != nullptr ? view->viewport() : annotatorWidget;
		mMoveTarget->installEventFilter(this);
		mMoveTarget->setCursor(Qt::SizeAllCursor);
		if(mToolGroup->checkedButton() != nullptr) {
			mToolGroup->checkedButton()->setChecked(false);
		}
	}
}

void CaptureWindow::updateGeometry()
{
	auto annotatorWidget = mAnnotator->widget();
	// The annotator's QGraphicsView centers the scene when its viewport is larger,
	// which would inset the image and push the painted border outside the window.
	auto view = annotatorCanvasView(annotatorWidget);
	if(view != nullptr) {
		view->setAlignment(Qt::AlignLeft | Qt::AlignTop);
		view->setFrameShape(QFrame::NoFrame);
		view->viewport()->setAutoFillBackground(false);
	}
	for(int i = 0; i < 2; ++i) {
		annotatorWidget->layout()->activate();
		auto chrome = QPoint(0, 0);
		auto imageRect = mAnnotator->imageGlobalRect();
		if(imageRect.isValid()) {
			auto offset = imageRect.topLeft() - annotatorWidget->mapToGlobal(QPoint(0, 0));
			chrome = QPoint(qMax(0, offset.x()), qMax(0, offset.y()));
		}
		annotatorWidget->setFixedSize(mImageSize.width() + chrome.x(), mImageSize.height() + chrome.y());
	}
	adjustSize();
	if(mBorderOverlay != nullptr) {
		mBorderOverlay->setGeometry(rect());
		mBorderOverlay->raise();
		mBorderOverlay->update();
	}
	update();
}

void CaptureWindow::alignToCaptureRect()
{
	if(mUserMoved) {
		return;
	}
	auto screen = QGuiApplication::screenAt(mCaptureRect.center());
	if(screen == nullptr) {
		screen = QGuiApplication::screenAt(QCursor::pos());
	}
	if(screen == nullptr) {
		screen = QGuiApplication::primaryScreen();
	}
	auto available = screen->availableGeometry();

	QPoint position;
	auto imageRect = mAnnotator->imageGlobalRect();
	if(mCaptureRect.isValid() && !mCaptureRect.isNull() && imageRect.isValid()) {
		position = pos() + (mCaptureRect.topLeft() - imageRect.topLeft());
	} else {
		position = available.center() - QPoint(width() / 2, height() / 2);
	}

	position.setX(qBound(available.left(), position.x(), qMax(available.left(), available.right() - width() + 1)));
	// If the window is taller than the desktop, keep the toolbar reachable
	// instead of clamping to the top and hiding the close buttons.
	auto maxY = available.bottom() - height() + 1;
	position.setY(maxY < available.top() ? maxY : qBound(available.top(), position.y(), maxY));
	move(position);
}

void CaptureWindow::showEvent(QShowEvent *event)
{
	QWidget::showEvent(event);
	QTimer::singleShot(0, this, [this](){
		updateGeometry();
		alignToCaptureRect();
		// Take focus so Esc discards even when the previous app kept focus.
		raise();
		activateWindow();
	});
}

bool CaptureWindow::event(QEvent *event)
{
	// The OCR result panel is a sibling top-level window. Windows raises the
	// window that gets activated, so clicking the capture window would slide the
	// panel behind it and the result would look as if it had been dismissed.
	// Re-raise it (without stealing focus) whenever this window is activated.
	if(event->type() == QEvent::WindowActivate && !mIsClosing) {
		QPointer<OcrResultDialog> dialog = mOcrDialog;
		if(!dialog.isNull() && dialog->isVisible()) {
			QTimer::singleShot(0, this, [dialog](){
				if(!dialog.isNull() && dialog->isVisible()) {
					dialog->raise();
				}
			});
		}
	}
	return QWidget::event(event);
}

void CaptureWindow::paintBorder()
{
	auto globalRect = mAnnotator->imageGlobalRect();
	if(!globalRect.isValid()) {
		return;
	}
	QRect imageRect(mapFromGlobal(globalRect.topLeft()), mImageSize);

	QPainter painter(mBorderOverlay);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setBrush(Qt::NoBrush);
	for(int i = 4; i >= 1; --i) {
		painter.setPen(QPen(QColor(0, 0, 0, 3 + (4 - i) * 3), 1));
		painter.drawRoundedRect(imageRect.adjusted(-i, -i, i, i), 4, 4);
	}
	painter.setPen(QPen(QColor(255, 255, 255, 70), 1));
	painter.drawRoundedRect(imageRect.adjusted(-1, -1, 1, 1), 4, 4);
	auto accent = Theme::instance()->accent();
	accent.setAlpha(200);
	painter.setPen(QPen(accent, 1));
	painter.drawRoundedRect(imageRect, 3, 3);
}

void CaptureWindow::keyPressEvent(QKeyEvent *event)
{
	if(event->key() == Qt::Key_Escape) {
		discard();
		return;
	}
	QWidget::keyPressEvent(event);
}

bool CaptureWindow::eventFilter(QObject *watched, QEvent *event)
{
	if(watched == mBorderOverlay && event->type() == QEvent::Paint) {
		paintBorder();
		return true;
	}
	if(mMoveTarget != nullptr && watched == mMoveTarget && handleDrag(mMoveTarget, event)) {
		return true;
	}
	if(watched == mToolBar) {
		if(event->type() == QEvent::MouseButtonDblClick) {
			confirm();
			return true;
		}
		if(event->type() == QEvent::MouseButtonPress) {
			auto mouseEvent = static_cast<QMouseEvent*>(event);
			// Qt only synthesizes double clicks for accepted presses; detect it here.
			if(mouseEvent->button() == Qt::LeftButton && mBarPressTimer.isValid()
					&& mBarPressTimer.elapsed() < 500
					&& (mouseEvent->globalPos() - mBarPressPos).manhattanLength() < 6) {
				confirm();
				return true;
			}
			mBarPressTimer.restart();
			mBarPressPos = mouseEvent->globalPos();
		}
		if(handleDrag(mToolBar, event)) {
			return true;
		}
	}
	return QWidget::eventFilter(watched, event);
}

bool CaptureWindow::handleDrag(QWidget *source, QEvent *event)
{
	if(event->type() == QEvent::MouseButtonPress) {
		auto mouseEvent = static_cast<QMouseEvent*>(event);
		if(mouseEvent->button() == Qt::LeftButton) {
			mIsDragging = true;
			mUserMoved = true;
			mDragOffset = mouseEvent->globalPos() - frameGeometry().topLeft();
			// The press is consumed below, so Qt sets no implicit grab; without an
			// explicit grab a release outside the source would leave us dragging.
			source->grabMouse();
			// Let the toolbar see the press so Qt can synthesize double clicks.
			return source != mToolBar;
		}
	} else if(event->type() == QEvent::MouseMove && mIsDragging) {
		auto target = static_cast<QMouseEvent*>(event)->globalPos() - mDragOffset;
		// Keep the whole window (toolbar included) on screen while dragging.
		auto screen = QGuiApplication::screenAt(QCursor::pos());
		if(screen == nullptr) {
			screen = QGuiApplication::primaryScreen();
		}
		auto available = screen->availableGeometry();
		target.setX(qBound(available.left(), target.x(), qMax(available.left(), available.right() - width() + 1)));
		target.setY(qBound(available.top(), target.y(), qMax(available.top(), available.bottom() - height() + 1)));
		move(target);
		return true;
	} else if(event->type() == QEvent::MouseButtonRelease) {
		if(mIsDragging) {
			mIsDragging = false;
			source->releaseMouse();
		}
		return true;
	}
	return false;
}

void CaptureWindow::save()
{
	// The save flow opens its own dialog; keeping the OCR panel around would put
	// two floating windows in front of the file dialog.
	closeOcrDialog();
	SaveOperation operation(
			mAnnotator->image(),
			true,
			mNotificationService,
			mDependencyInjector->get<IRecentImageService>(),
			mDependencyInjector->get<IImageSaver>(),
			mDependencyInjector->get<ISavePathProvider>(),
			mDependencyInjector->get<IFileDialogService>(),
			mConfig,
			this);
	auto result = operation.execute();
	if(result.isSuccessful) {
		close();
	}
}

void CaptureWindow::confirm()
{
	mClipboard->setImage(mAnnotator->image());
	close();
}

void CaptureWindow::discard()
{
	close();
}

void CaptureWindow::runOcr()
{
	if(mIsClosing) {
		return;
	}
	// The result panel is window-modal; re-raise it instead of stacking a second one.
	if(!mOcrDialog.isNull()) {
		mOcrDialog->raise();
		mOcrDialog->activateWindow();
		return;
	}
	if(mConfig->ollamaEnabled()) {
		if(mOcr->isBusy()) {
			return;
		}
		setOcrBusy(true);
		mOcr->recognize(mAnnotator->image());
	} else {
		if(mWinOcr->isBusy()) {
			return;
		}
		setOcrBusy(true);
		mWinOcr->recognize(mAnnotator->image());
	}
}

void CaptureWindow::setOcrBusy(bool busy)
{
	mOcrButton->setEnabled(!busy);
	mOcrButton->setToolTip(busy ? tr("识别中...") : tr("OCR 文字识别"));
}

void CaptureWindow::ocrFinished(const QString &text)
{
	setOcrBusy(false);
	// A result that arrives after the window was dismissed must not open a dialog
	// on a dying window: that used to leave an orphaned panel behind.
	if(mIsClosing || !isVisible()) {
		return;
	}
	showOcrResult(text);
}

void CaptureWindow::ocrFailed(const QString &error)
{
	setOcrBusy(false);
	if(mIsClosing) {
		return;
	}
	mNotificationService->showWarning(tr("OCR 失败"), error, QString());
}

void CaptureWindow::showOcrResult(const QString &text)
{
	if(!mOcrDialog.isNull()) {
		mOcrDialog->raise();
		mOcrDialog->activateWindow();
		return;
	}
	auto dialog = new OcrResultDialog(text, this);
	mOcrDialog = dialog;
	connect(dialog, &QDialog::finished, this, [this](){
		// Hand focus back, otherwise Esc and the toolbar buttons stay dead until
		// the user clicks the floating window first.
		QTimer::singleShot(0, this, &CaptureWindow::restoreAfterOcrDialog);
	});
	dialog->show();
	dialog->raise();
	dialog->activateWindow();
}

void CaptureWindow::restoreAfterOcrDialog()
{
	if(mIsClosing || !isVisible()) {
		return;
	}
	// A save file dialog (or any other modal window) opened right after the panel
	// closed must keep the focus; activating the capture window would steal it.
	if(QApplication::activeModalWidget() != nullptr) {
		return;
	}
	mOcrButton->setEnabled(true);
	raise();
	activateWindow();
}

void CaptureWindow::closeOcrDialog()
{
	if(mOcrDialog.isNull()) {
		return;
	}
	// WA_DeleteOnClose removes the panel; drop the pointer right away so no
	// later raise() can touch a dialog that is already on its way out.
	auto dialog = mOcrDialog.data();
	mOcrDialog = nullptr;
	dialog->close();
}

void CaptureWindow::releaseMouseGrab()
{
	if(mIsDragging) {
		mIsDragging = false;
		if(mMoveTarget != nullptr) {
			mMoveTarget->releaseMouse();
		} else if(mToolBar != nullptr) {
			mToolBar->releaseMouse();
		}
	}
}

void CaptureWindow::closeEvent(QCloseEvent *event)
{
	mIsClosing = true;
	// Stop a running OCR so its late callback cannot fire against a closing window
	// and can no longer flip the busy state or spawn a dialog.
	mOcr->cancel();
	mWinOcr->cancel();
	releaseMouseGrab();
	closeOcrDialog();
	QWidget::closeEvent(event);
}