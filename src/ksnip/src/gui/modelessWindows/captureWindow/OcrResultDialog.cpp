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

#include "OcrResultDialog.h"
#include "src/common/AppInfo.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPlainTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QClipboard>
#include <QCursor>
#include <QGuiApplication>
#include <QApplication>
#include <QScreen>
#include <QShowEvent>
#include <QTimer>
#include <QIcon>
#include <QTextCursor>

#include "src/gui/style/Theme.h"
#include "src/gui/style/Ui.h"

namespace {
// A plain-text document is cheap to fill, but a pathological OCR result (a model
// that rambles for megabytes) would still stall the layout, so the view is capped
// while the clipboard always receives the full text.
constexpr int maxDisplayCharacters = 200000;
}

OcrResultDialog::OcrResultDialog(const QString &text, QWidget *parent) :
	QDialog(parent, Qt::Window | Qt::WindowStaysOnTopHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint),
	mFullText(text),
	mTextEdit(nullptr),
	mCharacterCount(nullptr),
	mHintLabel(nullptr)
{
	setObjectName(QLatin1String("liteOcrDialog"));
	setWindowTitle(AppInfo::displayName());
	setWindowIcon(QIcon(QStringLiteral(":/icons/app.png")));
	setAttribute(Qt::WA_DeleteOnClose);
	// Deliberately non-modal. Window modality disables the owner window at the
	// Win32 level, so the floating capture window stopped answering clicks: the
	// toolbar (discard/confirm) and Esc were dead while the panel was open and the
	// user could not dismiss the capture at all. Instead the panel behaves like a
	// satellite window: the capture window stays fully interactive and re-raises
	// this panel whenever it gets activated, so the panel never ends up buried.
	resize(600, 520);
	setMinimumSize(460, 360);

	auto titleLabel = new QLabel(tr("OCR 识别结果"), this);
	titleLabel->setProperty("role", QLatin1String("title"));
	mHintLabel = new QLabel(tr("识别结果只读，点「复制文本」可取得全文并保留原有换行。"), this);
	mHintLabel->setProperty("role", QLatin1String("hint"));
	mHintLabel->setWordWrap(true);

	// QPlainTextEdit keeps plain-text layout linear; QTextEdit's rich document
	// re-layouts the whole block on every wrap and was the slow path for long
	// results.
	mTextEdit = new QPlainTextEdit(this);
	mTextEdit->setObjectName(QLatin1String("liteOcrText"));
	mTextEdit->setReadOnly(true);
	mTextEdit->setPlainText(tr("正在载入识别结果…"));
	mTextEdit->setLineWrapMode(QPlainTextEdit::WidgetWidth);
	mTextEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	// Widget-level sheet beats the global QWidget font-family; prefer Source Han Sans.
	mTextEdit->setStyleSheet(QStringLiteral(
		"font-family: \"Source Han Sans SC\",\"Source Han Sans CN\",\"思源黑体\","
		"\"Noto Sans CJK SC\",\"Noto Sans SC\",\"Microsoft YaHei\"; font-size: 14px;"));

	mCharacterCount = new QLabel(tr("%1 字符").arg(text.size()), this);
	mCharacterCount->setProperty("role", QLatin1String("hint"));

	auto copyButton = Ui::button(tr("复制文本"), Ui::ButtonVariant::Primary, this);
	copyButton->setDefault(true);
	connect(copyButton, &QPushButton::clicked, this, [this](){
		QGuiApplication::clipboard()->setText(mFullText);
		close();
	});

	auto closeButton = Ui::button(tr("关闭"), Ui::ButtonVariant::Outline, this);
	closeButton->setAutoDefault(false);
	connect(closeButton, &QPushButton::clicked, this, &OcrResultDialog::close);

	auto footer = Ui::buttonRow();
	footer->addWidget(mCharacterCount, 0, Qt::AlignVCenter);
	footer->addStretch(1);
	footer->addWidget(closeButton);
	footer->addWidget(copyButton);

	auto layout = new QVBoxLayout(this);
	layout->setContentsMargins(24, 20, 24, 20);
	layout->setSpacing(0);
	layout->addWidget(titleLabel);
	layout->addSpacing(5);
	layout->addWidget(mHintLabel);
	layout->addSpacing(16);
	layout->addWidget(mTextEdit, 1);
	layout->addSpacing(16);
	layout->addLayout(footer);

	// Fill after the first paint: the window shows up immediately even when the
	// document is large, instead of appearing as an empty, unresponsive frame.
	QTimer::singleShot(0, this, [this](){ fillText(); });
}

void OcrResultDialog::fillText()
{
	if(mTextEdit == nullptr) {
		return;
	}
	bool truncated = false;
	auto text = displayText(&truncated);
	mTextEdit->setPlainText(text);
	mTextEdit->moveCursor(QTextCursor::Start);
	mTextEdit->ensureCursorVisible();
	if(truncated && mHintLabel != nullptr) {
		mHintLabel->setText(tr("识别结果较长，此处仅显示前 %1 字符；点「复制文本」可获取完整内容。")
						   .arg(maxDisplayCharacters));
	}
}

QString OcrResultDialog::displayText(bool *truncated) const
{
	if(truncated != nullptr) {
		*truncated = mFullText.size() > maxDisplayCharacters;
	}
	if(mFullText.size() <= maxDisplayCharacters) {
		return mFullText;
	}
	return mFullText.left(maxDisplayCharacters);
}

void OcrResultDialog::showEvent(QShowEvent *event)
{
	QDialog::showEvent(event);
	placeNextToOwner();
	raise();
	activateWindow();
	QTimer::singleShot(0, this, [this](){ raise(); activateWindow(); });
}

void OcrResultDialog::placeNextToOwner()
{
	auto screen = parentWidget() != nullptr
			? QGuiApplication::screenAt(parentWidget()->frameGeometry().center())
			: nullptr;
	if(screen == nullptr) {
		screen = QGuiApplication::screenAt(QCursor::pos());
	}
	if(screen == nullptr) {
		screen = QGuiApplication::primaryScreen();
	}
	if(screen == nullptr) {
		return;
	}
	auto available = screen->availableGeometry();
	auto size = geometry().size();
	// Prefer a spot beside the capture window so the panel does not cover the
	// image or the toolbar; a full-screen capture leaves no room and falls back
	// to the screen centre.
	QPoint position;
	auto owner = parentWidget() != nullptr ? parentWidget()->frameGeometry() : QRect();
	bool placed = false;
	if(owner.isValid() && !owner.isEmpty()) {
		const int gap = 14;
		if(owner.right() + gap + size.width() <= available.right()) {
			position = QPoint(owner.right() + gap, owner.top());
			placed = true;
		} else if(owner.left() - gap - size.width() >= available.left()) {
			position = QPoint(owner.left() - gap - size.width(), owner.top());
			placed = true;
		} else if(owner.bottom() + gap + size.height() <= available.bottom()) {
			position = QPoint(owner.left(), owner.bottom() + gap);
			placed = true;
		} else if(owner.top() - gap - size.height() >= available.top()) {
			position = QPoint(owner.left(), owner.top() - gap - size.height());
			placed = true;
		}
	}
	if(!placed) {
		position = available.center() - QPoint(size.width() / 2, size.height() / 2);
	}
	position.setX(qBound(available.left(), position.x(), qMax(available.left(), available.right() - size.width() + 1)));
	position.setY(qBound(available.top(), position.y(), qMax(available.top(), available.bottom() - size.height() + 1)));
	move(position);
}
