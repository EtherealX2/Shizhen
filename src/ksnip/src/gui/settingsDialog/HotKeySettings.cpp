/*
 * Copyright (C) 2019 Damir Porobic <damir.porobic@gmx.com>
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

#include "HotKeySettings.h"

#include "src/backend/config/ConfigOptions.h"

HotKeySettings::HotKeySettings(const QList<CaptureModes> &captureModes, const QSharedPointer<IPlatformChecker> &platformChecker, const QSharedPointer<IConfig> &config) :
	mConfig(config),
	mPlatformChecker(platformChecker),
	mCaptureModes(captureModes),
	mEnableGlobalHotKeysCheckBox(new QCheckBox(this)),
	mRectAreaLabel(new QLabel(this)),
	mLastRectAreaLabel(new QLabel(this)),
	mFullScreenLabel(new QLabel(this)),
	mCurrentScreenLabel(new QLabel(this)),
	mActiveWindowLabel(new QLabel(this)),
	mWindowUnderCursorLabel(new QLabel(this)),
	mPortalLabel(new QLabel(this)),
	mRectAreaClearPushButton(new QPushButton(this)),
	mLastRectAreaClearPushButton(new QPushButton(this)),
	mFullScreenClearPushButton(new QPushButton(this)),
	mCurrentScreenClearPushButton(new QPushButton(this)),
	mActiveWindowClearPushButton(new QPushButton(this)),
	mWindowUnderCursorClearPushButton(new QPushButton(this)),
	mPortalClearPushButton(new QPushButton(this)),
	mRestoreDefaultsPushButton(new QPushButton(this)),
	mLayout(new QGridLayout(this))
{
	Q_ASSERT(mConfig != nullptr);

	initGui();
	loadConfig();
}

HotKeySettings::~HotKeySettings()
{
	delete mEnableGlobalHotKeysCheckBox;
	delete mRectAreaLabel;
	delete mLastRectAreaLabel;
	delete mFullScreenLabel;
	delete mCurrentScreenLabel;
	delete mActiveWindowLabel;
	delete mWindowUnderCursorLabel;
	delete mPortalLabel;
	delete mRectAreaKeySequenceLineEdit;
	delete mLastRectAreaKeySequenceLineEdit;
	delete mFullScreenKeySequenceLineEdit;
	delete mCurrentScreenKeySequenceLineEdit;
	delete mActiveWindowKeySequenceLineEdit;
	delete mWindowUnderCursorKeySequenceLineEdit;
	delete mPortalKeySequenceLineEdit;
	delete mRectAreaClearPushButton;
	delete mLastRectAreaClearPushButton;
	delete mFullScreenClearPushButton;
	delete mCurrentScreenClearPushButton;
	delete mActiveWindowClearPushButton;
	delete mWindowUnderCursorClearPushButton;
	delete mPortalClearPushButton;
	delete mRestoreDefaultsPushButton;
	delete mLayout;
}

void HotKeySettings::saveSettings()
{
	mConfig->setGlobalHotKeysEnabled(mEnableGlobalHotKeysCheckBox->isChecked());
	mConfig->setRectAreaHotKey(mRectAreaKeySequenceLineEdit->value());
	mConfig->setLastRectAreaHotKey(mLastRectAreaKeySequenceLineEdit->value());
	mConfig->setFullScreenHotKey(mFullScreenKeySequenceLineEdit->value());
	mConfig->setCurrentScreenHotKey(mCurrentScreenKeySequenceLineEdit->value());
	mConfig->setActiveWindowHotKey(mActiveWindowKeySequenceLineEdit->value());
	mConfig->setWindowUnderCursorHotKey(mWindowUnderCursorKeySequenceLineEdit->value());
	mConfig->setPortalHotKey(mPortalKeySequenceLineEdit->value());
}

void HotKeySettings::initGui()
{
	auto allowedKeys = HotKeyMap::instance()->getAllKeys();
	mRectAreaKeySequenceLineEdit = new KeySequenceLineEdit(this, allowedKeys, mPlatformChecker);
	mLastRectAreaKeySequenceLineEdit = new KeySequenceLineEdit(this, allowedKeys, mPlatformChecker);
	mFullScreenKeySequenceLineEdit = new KeySequenceLineEdit(this, allowedKeys, mPlatformChecker);
	mCurrentScreenKeySequenceLineEdit = new KeySequenceLineEdit(this, allowedKeys, mPlatformChecker);
	mActiveWindowKeySequenceLineEdit = new KeySequenceLineEdit(this, allowedKeys, mPlatformChecker);
	mWindowUnderCursorKeySequenceLineEdit = new KeySequenceLineEdit(this, allowedKeys, mPlatformChecker);
	mPortalKeySequenceLineEdit = new KeySequenceLineEdit(this, allowedKeys, mPlatformChecker);

	mEnableGlobalHotKeysCheckBox->setText(tr("启用全局快捷键"));
	mEnableGlobalHotKeysCheckBox->setToolTip(tr("全局快捷键目前仅支持 Windows 和 X11。\n"
											       "禁用此选项后，快捷键仅在拾帧内有效。"));
	connect(mEnableGlobalHotKeysCheckBox, &QCheckBox::stateChanged, this, &HotKeySettings::globalHotKeysStateChanged);

	mRectAreaLabel->setText(tr("矩形区域截图"));
	mLastRectAreaLabel->setText(tr("上次矩形区域截图"));
	mFullScreenLabel->setText(tr("全屏截图"));
	mCurrentScreenLabel->setText(tr("当前屏幕截图"));
	mActiveWindowLabel->setText(tr("活动窗口截图"));
	mWindowUnderCursorLabel->setText(tr("光标下窗口截图"));
	mPortalLabel->setText(tr("使用 Portal 截图"));

	// shadcn form rhythm: one shared label column, quiet text buttons for the
	// per-row reset action so seven of them do not compete with the fields.
	const QList<QLabel*> fieldLabels = { mRectAreaLabel, mLastRectAreaLabel, mFullScreenLabel,
			mCurrentScreenLabel, mActiveWindowLabel, mWindowUnderCursorLabel, mPortalLabel };
	for(auto label : fieldLabels) {
		label->setObjectName(QLatin1String("liteFieldLabel"));
		label->setFixedWidth(120);
		label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	}

	auto clearText = tr("清除");
	const QList<QPushButton*> clearButtons = { mRectAreaClearPushButton, mLastRectAreaClearPushButton,
			mFullScreenClearPushButton, mCurrentScreenClearPushButton, mActiveWindowClearPushButton,
			mWindowUnderCursorClearPushButton, mPortalClearPushButton };
	for(auto button : clearButtons) {
		button->setText(clearText);
		button->setProperty("variant", QLatin1String("ghost"));
		button->setCursor(Qt::PointingHandCursor);
		button->setFixedSize(52, 36);
	}

	connect(mRectAreaClearPushButton, &QPushButton::clicked, mRectAreaKeySequenceLineEdit, &KeySequenceLineEdit::clear);
	connect(mLastRectAreaClearPushButton, &QPushButton::clicked, mLastRectAreaKeySequenceLineEdit, &KeySequenceLineEdit::clear);
	connect(mFullScreenClearPushButton, &QPushButton::clicked, mFullScreenKeySequenceLineEdit, &KeySequenceLineEdit::clear);
	connect(mCurrentScreenClearPushButton, &QPushButton::clicked, mCurrentScreenKeySequenceLineEdit, &KeySequenceLineEdit::clear);
	connect(mActiveWindowClearPushButton, &QPushButton::clicked, mActiveWindowKeySequenceLineEdit, &KeySequenceLineEdit::clear);
	connect(mWindowUnderCursorClearPushButton, &QPushButton::clicked, mWindowUnderCursorKeySequenceLineEdit, &KeySequenceLineEdit::clear);
	connect(mPortalClearPushButton, &QPushButton::clicked, mPortalKeySequenceLineEdit, &KeySequenceLineEdit::clear);

	mRestoreDefaultsPushButton->setText(tr("恢复默认"));
	mRestoreDefaultsPushButton->setToolTip(tr("将以上全局快捷键恢复为默认组合键"));
	mRestoreDefaultsPushButton->setProperty("variant", QLatin1String("secondary"));
	mRestoreDefaultsPushButton->setCursor(Qt::PointingHandCursor);
	mRestoreDefaultsPushButton->setFixedHeight(32);
	connect(mRestoreDefaultsPushButton, &QPushButton::clicked, this, &HotKeySettings::restoreDefaults);

	mLayout->setAlignment(Qt::AlignTop);
	mLayout->setContentsMargins(0, 0, 0, 0);
	mLayout->setHorizontalSpacing(16);
	mLayout->setVerticalSpacing(10);
	mLayout->setColumnMinimumWidth(0, 120);
	mLayout->setColumnStretch(1, 1);
	mLayout->addWidget(mEnableGlobalHotKeysCheckBox, 0, 0, 1, 3);

	mLayout->addWidget(mRectAreaLabel, 1, 0, 1, 1);
	mLayout->addWidget(mRectAreaKeySequenceLineEdit, 1, 1, 1, 1);
	mLayout->addWidget(mRectAreaClearPushButton, 1, 2, 1, 1);

	mLayout->addWidget(mLastRectAreaLabel, 2, 0,1,1);
	mLayout->addWidget(mLastRectAreaKeySequenceLineEdit, 2, 1, 1, 1);
	mLayout->addWidget(mLastRectAreaClearPushButton, 2, 2, 1, 1);

	mLayout->addWidget(mFullScreenLabel, 3, 0, 1, 1);
	mLayout->addWidget(mFullScreenKeySequenceLineEdit, 3, 1, 1, 1);
	mLayout->addWidget(mFullScreenClearPushButton, 3, 2, 1, 1);

	mLayout->addWidget(mCurrentScreenLabel, 4, 0, 1, 1);
	mLayout->addWidget(mCurrentScreenKeySequenceLineEdit, 4, 1, 1, 1);
	mLayout->addWidget(mCurrentScreenClearPushButton, 4, 2, 1, 1);

	mLayout->addWidget(mActiveWindowLabel, 5, 0, 1, 1);
	mLayout->addWidget(mActiveWindowKeySequenceLineEdit, 5, 1, 1, 1);
	mLayout->addWidget(mActiveWindowClearPushButton, 5, 2, 1, 1);

	mLayout->addWidget(mWindowUnderCursorLabel, 6, 0, 1, 1);
	mLayout->addWidget(mWindowUnderCursorKeySequenceLineEdit, 6, 1, 1, 1);
	mLayout->addWidget(mWindowUnderCursorClearPushButton, 6, 2, 1, 1);

    mLayout->addWidget(mPortalLabel, 7, 0, 1, 1);
    mLayout->addWidget(mPortalKeySequenceLineEdit, 7, 1, 1, 1);
    mLayout->addWidget(mPortalClearPushButton, 7, 2, 1, 1);

	mLayout->addWidget(mRestoreDefaultsPushButton, 8, 1, 1, 2, Qt::AlignRight);

	setTitle(QString());
	setObjectName(QLatin1String("liteFlatGroup"));
	setLayout(mLayout);
}

void HotKeySettings::loadConfig()
{
	mEnableGlobalHotKeysCheckBox->setChecked(mConfig->globalHotKeysEnabled());
	mEnableGlobalHotKeysCheckBox->setEnabled(!mConfig->isGlobalHotKeysEnabledReadOnly());
	mRectAreaKeySequenceLineEdit->setValue(mConfig->rectAreaHotKey());
	mLastRectAreaKeySequenceLineEdit->setValue(mConfig->lastRectAreaHotKey());
	mFullScreenKeySequenceLineEdit->setValue(mConfig->fullScreenHotKey());
	mCurrentScreenKeySequenceLineEdit->setValue(mConfig->currentScreenHotKey());
	mActiveWindowKeySequenceLineEdit->setValue(mConfig->activeWindowHotKey());
	mWindowUnderCursorKeySequenceLineEdit->setValue(mConfig->windowUnderCursorHotKey());
	mPortalKeySequenceLineEdit->setValue(mConfig->portalHotKey());
	globalHotKeysStateChanged();
}

void HotKeySettings::globalHotKeysStateChanged()
{
	auto hotKeysEnabled = mEnableGlobalHotKeysCheckBox->isChecked() && mEnableGlobalHotKeysCheckBox->isEnabled();
	auto isRectAreaSupported = mCaptureModes.contains(CaptureModes::RectArea);
	auto isLastRectAreaSupported = mCaptureModes.contains(CaptureModes::LastRectArea);
	auto isFullScreenSupported = mCaptureModes.contains(CaptureModes::FullScreen);
	auto isCurrentScreenSupported = mCaptureModes.contains(CaptureModes::CurrentScreen);
	auto isActiveWindowSupported = mCaptureModes.contains(CaptureModes::ActiveWindow);
	auto isWindowUnderCursorSupported = mCaptureModes.contains(CaptureModes::WindowUnderCursor);
	auto isPortalSupported = mCaptureModes.contains(CaptureModes::Portal);

	mRectAreaLabel->setEnabled(hotKeysEnabled && isRectAreaSupported);
	mRectAreaKeySequenceLineEdit->setEnabled(hotKeysEnabled && isRectAreaSupported);
	mRectAreaClearPushButton->setEnabled(hotKeysEnabled && isRectAreaSupported);

	mLastRectAreaLabel->setEnabled(hotKeysEnabled && isLastRectAreaSupported);
	mLastRectAreaKeySequenceLineEdit->setEnabled(hotKeysEnabled && isLastRectAreaSupported);
	mLastRectAreaClearPushButton->setEnabled(hotKeysEnabled && isLastRectAreaSupported);

	mFullScreenLabel->setEnabled(hotKeysEnabled && isFullScreenSupported);
	mFullScreenKeySequenceLineEdit->setEnabled(hotKeysEnabled && isFullScreenSupported);
	mFullScreenClearPushButton->setEnabled(hotKeysEnabled && isFullScreenSupported);

	mCurrentScreenLabel->setEnabled(hotKeysEnabled && isCurrentScreenSupported);
	mCurrentScreenKeySequenceLineEdit->setEnabled(hotKeysEnabled && isCurrentScreenSupported);
	mCurrentScreenClearPushButton->setEnabled(hotKeysEnabled && isCurrentScreenSupported);

	mActiveWindowLabel->setEnabled(hotKeysEnabled && isActiveWindowSupported);
	mActiveWindowKeySequenceLineEdit->setEnabled(hotKeysEnabled && isActiveWindowSupported);
	mActiveWindowClearPushButton->setEnabled(hotKeysEnabled && isActiveWindowSupported);

	mWindowUnderCursorLabel->setEnabled(hotKeysEnabled && isWindowUnderCursorSupported);
	mWindowUnderCursorKeySequenceLineEdit->setEnabled(hotKeysEnabled && isWindowUnderCursorSupported);
	mWindowUnderCursorClearPushButton->setEnabled(hotKeysEnabled && isWindowUnderCursorSupported);

    mPortalLabel->setEnabled(hotKeysEnabled && isPortalSupported);
    mPortalKeySequenceLineEdit->setEnabled(hotKeysEnabled && isPortalSupported);
    mPortalClearPushButton->setEnabled(hotKeysEnabled && isPortalSupported);

	mRestoreDefaultsPushButton->setEnabled(hotKeysEnabled);
}

void HotKeySettings::restoreDefaults()
{
	mRectAreaKeySequenceLineEdit->setValue(ConfigOptions::defaultHotKey(CaptureModes::RectArea));
	mLastRectAreaKeySequenceLineEdit->setValue(ConfigOptions::defaultHotKey(CaptureModes::LastRectArea));
	mFullScreenKeySequenceLineEdit->setValue(ConfigOptions::defaultHotKey(CaptureModes::FullScreen));
	mCurrentScreenKeySequenceLineEdit->setValue(ConfigOptions::defaultHotKey(CaptureModes::CurrentScreen));
	mActiveWindowKeySequenceLineEdit->setValue(ConfigOptions::defaultHotKey(CaptureModes::ActiveWindow));
	mWindowUnderCursorKeySequenceLineEdit->setValue(ConfigOptions::defaultHotKey(CaptureModes::WindowUnderCursor));
	mPortalKeySequenceLineEdit->setValue(ConfigOptions::defaultHotKey(CaptureModes::Portal));
}
