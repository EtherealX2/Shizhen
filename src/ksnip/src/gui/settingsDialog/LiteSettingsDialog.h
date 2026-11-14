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

#ifndef KSNIP_LITESETTINGSDIALOG_H
#define KSNIP_LITESETTINGSDIALOG_H

#include <QDialog>
#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QList>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>
#include <QSpinBox>
#include <QLabel>

#include "src/backend/config/IConfig.h"
#include "src/gui/settingsDialog/HotKeySettings.h"
#include "src/backend/ocr/OllamaOcr.h"
#include "src/common/adapter/fileDialog/IFileDialogService.h"
#include "src/common/platform/IPlatformChecker.h"

class QScrollArea;
class QStackedWidget;
class QVBoxLayout;

class LiteSettingsDialog : public QDialog
{
	Q_OBJECT
public:
	LiteSettingsDialog(
			const QList<CaptureModes> &captureModes,
			const QSharedPointer<IConfig> &config,
			const QSharedPointer<IPlatformChecker> &platformChecker,
			const QSharedPointer<IFileDialogService> &fileDialogService,
			QWidget *parent = nullptr);
	~LiteSettingsDialog() override;

protected:
	void showEvent(QShowEvent *event) override;

private:
	QSharedPointer<IConfig> mConfig;
	QSharedPointer<IFileDialogService> mFileDialogService;
	HotKeySettings *mHotKeySettings;
	OllamaOcr *mOcr;

	QCheckBox *mMouseHookEnabledCheckBox;
	QComboBox *mSideButton1ComboBox;
	QComboBox *mSideButton2ComboBox;

	QComboBox *mDelayComboBox;
	QCheckBox *mCaptureCursorCheckBox;
	QCheckBox *mFreezeScreenCheckBox;
	QLineEdit *mSaveDirectoryLineEdit;
	QComboBox *mSaveFormatComboBox;
	QCheckBox *mAutoCopyCheckBox;
	QCheckBox *mAutoSaveCheckBox;

	QCheckBox *mOllamaEnabledCheckBox;
	QLineEdit *mOllamaUrlLineEdit;
	QLineEdit *mOllamaModelLineEdit;
	QPlainTextEdit *mOllamaPromptEdit;
	QPushButton *mOcrTestButton;
	QLabel *mOcrTestResultLabel;

	QCheckBox *mAutoStartCheckBox;
	QCheckBox *mShowSnippingHintCheckBox;
	QComboBox *mAppearanceComboBox;

	QWidget *mSegmentedBar;
	QStackedWidget *mPages;
	QScrollArea *mPageScroll;
	QWidget *mOllamaCard;
	QList<QPushButton*> mSegmentButtons;
	bool mCentered = false;

	void setupHeader(QVBoxLayout *layout);
	void setupSegments(QVBoxLayout *layout);
	void addPage(QWidget *page, const QString &title);
	void showPage(int index);
	// Sizes the page viewport once to the tallest of the four pages so switching
	// segments never resizes or re-centres the window.
	void applyBaseLayout();
	void centerOnScreen();

	QWidget* createHotKeyTab(const QList<CaptureModes> &captureModes, const QSharedPointer<IPlatformChecker> &platformChecker);
	QWidget* createCaptureTab();
	QWidget* createOcrTab();
	QWidget* createGeneralTab();
	void fillModeComboBox(QComboBox *comboBox);
	void loadSettings();
	void saveSettings();
	void browseSaveDirectory();
	void testOcrConnection();
	bool autoStartEnabled() const;
	void setAutoStartEnabled(bool enabled);
};

#endif //KSNIP_LITESETTINGSDIALOG_H
