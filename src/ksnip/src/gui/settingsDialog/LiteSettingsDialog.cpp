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

#include "LiteSettingsDialog.h"
#include "src/common/AppInfo.h"
#include "BuildConfig.h"

#include <QApplication>
#include <QCursor>
#include <QDesktopServices>
#include <QFrame>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QScreen>
#include <QScrollArea>
#include <QScrollBar>
#include <QShowEvent>
#include <QStackedWidget>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include <QCoreApplication>
#include <QSettings>
#include <QDir>

#include "src/gui/style/Theme.h"
#include "src/gui/style/Ui.h"

namespace {
// One shared label column keeps every form row on the same optical grid. The
// width lives in Ui so the hotkey page, which builds its own grid, matches it.
constexpr int kLabelWidth = Ui::FormLabelWidth;
constexpr int kFieldGap = Ui::FormGap;
constexpr int kControlHeight = 36;

QWidget *formRow(QWidget *parent, const QString &labelText, QWidget *field, bool fullWidth = true)
{
	auto row = new QWidget(parent);
	auto layout = new QHBoxLayout(row);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(kFieldGap);

	auto label = new QLabel(labelText, row);
	label->setObjectName(QLatin1String("liteFieldLabel"));
	label->setFixedWidth(kLabelWidth);
	label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	layout->addWidget(label, 0, Qt::AlignVCenter);

	if(fullWidth) {
		layout->addWidget(field, 1);
	} else {
		layout->addWidget(field, 0, Qt::AlignVCenter);
		layout->addStretch(1);
	}

	row->setMinimumHeight(kControlHeight);
	return row;
}

// Label + control stacked vertically, used for the multi-line prompt field.
QWidget *formBlock(QWidget *parent, const QString &labelText, QWidget *field)
{
	auto block = new QWidget(parent);
	auto layout = new QHBoxLayout(block);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(kFieldGap);

	auto label = new QLabel(labelText, block);
	label->setObjectName(QLatin1String("liteFieldLabel"));
	label->setFixedWidth(kLabelWidth);
	label->setAlignment(Qt::AlignLeft | Qt::AlignTop);
	label->setContentsMargins(0, 8, 0, 0);
	layout->addWidget(label, 0, Qt::AlignTop);
	layout->addWidget(field, 1);

	return block;
}

QWidget *checkboxRow(QWidget *parent, QCheckBox *checkBox)
{
	auto row = new QWidget(parent);
	auto layout = new QHBoxLayout(row);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(kFieldGap);
	layout->addSpacing(kLabelWidth + kFieldGap);
	layout->addWidget(checkBox, 0, Qt::AlignVCenter);
	layout->addStretch(1);
	row->setMinimumHeight(28);
	return row;
}
}

LiteSettingsDialog::LiteSettingsDialog(
		const QList<CaptureModes> &captureModes,
		const QSharedPointer<IConfig> &config,
		const QSharedPointer<IPlatformChecker> &platformChecker,
		const QSharedPointer<IFileDialogService> &fileDialogService,
		QWidget *parent) :
	QDialog(parent, Qt::WindowTitleHint | Qt::WindowCloseButtonHint),
	mConfig(config),
	mFileDialogService(fileDialogService),
	mHotKeySettings(new HotKeySettings(captureModes, platformChecker, mConfig)),
	mOcr(new OllamaOcr(mConfig, this)),
	mMouseHookEnabledCheckBox(new QCheckBox(this)),
	mSideButton1ComboBox(new QComboBox(this)),
	mSideButton2ComboBox(new QComboBox(this)),
	mDelayComboBox(new QComboBox(this)),
	mCaptureCursorCheckBox(new QCheckBox(this)),
	mFreezeScreenCheckBox(new QCheckBox(this)),
	mSaveDirectoryLineEdit(new QLineEdit(this)),
	mSaveFormatComboBox(new QComboBox(this)),
	mAutoCopyCheckBox(new QCheckBox(this)),
	mAutoSaveCheckBox(new QCheckBox(this)),
	mOllamaEnabledCheckBox(new QCheckBox(this)),
	mOllamaUrlLineEdit(new QLineEdit(this)),
	mOllamaModelLineEdit(new QLineEdit(this)),
	mOllamaPromptEdit(new QPlainTextEdit(this)),
	mOcrTestButton(new QPushButton(this)),
	mOcrTestResultLabel(new QLabel(this)),
	mAutoStartCheckBox(new QCheckBox(this)),
	mShowSnippingHintCheckBox(new QCheckBox(this)),
	mAppearanceComboBox(new QComboBox(this)),
	mSegmentedBar(nullptr),
	mPages(nullptr),
	mPageScroll(nullptr),
	mOllamaCard(nullptr)
{
	setObjectName(QLatin1String("liteDialog"));
	// The in-content header carries the title; keep the native bar to the app name.
	setWindowTitle(AppInfo::displayName());
	setFixedWidth(720);

	auto layout = new QVBoxLayout(this);
	layout->setContentsMargins(24, 20, 24, 20);
	layout->setSpacing(0);

	setupHeader(layout);

	setupSegments(layout);
	addPage(createHotKeyTab(captureModes, platformChecker), tr("快捷键"));
	addPage(createCaptureTab(), tr("截图与保存"));
	addPage(createOcrTab(), tr("OCR"));
	addPage(createGeneralTab(), tr("通用"));

	layout->addSpacing(16);
	layout->addWidget(Ui::hline(this));
	layout->addSpacing(14);

	auto cancelButton = Ui::button(tr("取消"), Ui::ButtonVariant::Outline, this);
	cancelButton->setAutoDefault(false);
	auto okButton = Ui::button(tr("确定"), Ui::ButtonVariant::Primary, this);
	okButton->setDefault(true);

	auto footer = Ui::buttonRow();
	footer->addStretch(1);
	footer->addWidget(cancelButton);
	footer->addWidget(okButton);
	layout->addLayout(footer);

	connect(okButton, &QPushButton::clicked, this, [this](){ saveSettings(); accept(); });
	connect(cancelButton, &QPushButton::clicked, this, &LiteSettingsDialog::reject);

	connect(mOcr, &OllamaOcr::testFinished, this, [this](bool successful, const QString &info){
		mOcrTestResultLabel->setText(info);
		auto tone = successful ? QLatin1String("success") : QLatin1String("destructive");
		mOcrTestResultLabel->setProperty("tone", tone);
		Ui::repolish(mOcrTestResultLabel);
	});

	loadSettings();
	showPage(0);
}

LiteSettingsDialog::~LiteSettingsDialog()
{
	delete mHotKeySettings;
}

void LiteSettingsDialog::setupHeader(QVBoxLayout *layout)
{
	auto title = new QLabel(tr("设置"), this);
	title->setProperty("role", QLatin1String("title"));
	auto description = new QLabel(tr("调整截图行为、保存方式与文字识别选项。"), this);
	description->setProperty("role", QLatin1String("hint"));

	layout->addWidget(title);
	layout->addSpacing(6);
	layout->addWidget(description);
	layout->addSpacing(20);
}

void LiteSettingsDialog::setupSegments(QVBoxLayout *layout)
{
	mSegmentedBar = new QWidget(this);
	mSegmentedBar->setObjectName(QLatin1String("liteSegmented"));
	mSegmentedBar->setAttribute(Qt::WA_StyledBackground, true);
	auto barLayout = new QHBoxLayout(mSegmentedBar);
	barLayout->setContentsMargins(3, 3, 3, 3);
	barLayout->setSpacing(3);

	mPages = new QStackedWidget(this);
	mPages->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

	mPageScroll = new QScrollArea(this);
	mPageScroll->setFrameShape(QFrame::NoFrame);
	mPageScroll->setWidgetResizable(true);
	mPageScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	mPageScroll->setWidget(mPages);
	mPageScroll->viewport()->setAutoFillBackground(false);

	layout->addWidget(mSegmentedBar);
	layout->addWidget(mPageScroll);
}

void LiteSettingsDialog::addPage(QWidget *page, const QString &title)
{
	auto segment = new QPushButton(title, mSegmentedBar);
	segment->setCheckable(true);
	segment->setCursor(Qt::PointingHandCursor);
	segment->setFixedHeight(32);
	segment->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	segment->setProperty("segment", true);

	auto index = mSegmentButtons.size();
	connect(segment, &QPushButton::clicked, this, [this, index](){ showPage(index); });

	static_cast<QHBoxLayout*>(mSegmentedBar->layout())->addWidget(segment, 1);
	mSegmentButtons.append(segment);
	mPages->addWidget(page);
}

void LiteSettingsDialog::showPage(int index)
{
	if(index < 0 || index >= mPages->count()) {
		return;
	}
	mPages->setCurrentIndex(index);
	for(int i = 0; i < mSegmentButtons.size(); ++i) {
		mSegmentButtons[i]->setChecked(i == index);
	}
	// No resize here on purpose: every page shares the base height applied on the
	// first show, so switching segments keeps the window perfectly still.
}

void LiteSettingsDialog::showEvent(QShowEvent *event)
{
	QDialog::showEvent(event);
	if(mCentered) {
		return;
	}
	mCentered = true;
	applyBaseLayout();
	centerOnScreen();
	// The window is not mapped yet while showEvent runs; re-assert activation so
	// the dialog does not open behind the window the capture was taken from.
	QTimer::singleShot(0, this, [this](){ raise(); activateWindow(); });
}

void LiteSettingsDialog::centerOnScreen()
{
	auto screen = QGuiApplication::screenAt(QCursor::pos());
	if(screen == nullptr) {
		screen = QGuiApplication::primaryScreen();
	}
	auto available = screen->availableGeometry();
	move(available.center() - rect().center());
	raise();
	activateWindow();
}

void LiteSettingsDialog::applyBaseLayout()
{
	if(mPages->count() == 0) {
		return;
	}

	// Measure with the scroll bar suppressed, otherwise its 10px would narrow the
	// page and change the wrapping we are about to measure.
	mPageScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	// Every page has to be measured as the current one: QStackedWidget only lays
	// out the visible page, so the others would report a stale height and, with
	// word-wrapped labels, a stale width.
	const auto visible = mPages->currentIndex();
	int needed = 0;
	for(int i = 0; i < mPages->count(); ++i) {
		mPages->setCurrentIndex(i);
		layout()->activate();
		auto page = mPages->widget(i);
		page->ensurePolished();
		page->adjustSize();
		needed = qMax(needed, page->sizeHint().height());
	}
	mPages->setCurrentIndex(visible);

	// Reserve the tallest page for all of them. Shorter pages keep the same
	// canvas and simply end higher, which reads as one surface instead of four
	// windows that happen to share a frame.
	mPageScroll->setFixedHeight(needed);
	layout()->activate();
	adjustSize();

	// Only if that height does not fit the screen does the viewport shrink and
	// start scrolling; the base stays identical across pages either way.
	auto screen = QGuiApplication::screenAt(QCursor::pos());
	if(screen == nullptr) {
		screen = QGuiApplication::primaryScreen();
	}
	const auto limit = screen->availableGeometry().height() * 4 / 5;
	if(height() > limit) {
		const auto chrome = height() - needed;
		mPageScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
		mPageScroll->setFixedHeight(qMax(240, limit - chrome));
		layout()->activate();
		adjustSize();
	}
}

QWidget* LiteSettingsDialog::createHotKeyTab(const QList<CaptureModes> &captureModes, const QSharedPointer<IPlatformChecker> &platformChecker)
{
	Q_UNUSED(captureModes)
	Q_UNUSED(platformChecker)

	auto page = new QWidget(this);
	auto layout = new QVBoxLayout(page);
	layout->setContentsMargins(0, 0, 0, 4);
	layout->setSpacing(14);

	QVBoxLayout *hotkeyBody;
	auto hotkeyCard = Ui::card(page, tr("全局快捷键"), tr("在任何应用中直接开始截图，无需先唤出本工具。"), hotkeyBody);
	hotkeyBody->addWidget(mHotKeySettings);
	layout->addWidget(hotkeyCard);

	QVBoxLayout *mouseBody;
	auto mouseCard = Ui::card(page, tr("鼠标侧键"), tr("把鼠标侧键映射为常用的截图方式。"), mouseBody);

	mMouseHookEnabledCheckBox->setText(tr("启用鼠标侧键快捷方式"));
	mouseBody->addWidget(checkboxRow(mouseCard, mMouseHookEnabledCheckBox));

	fillModeComboBox(mSideButton1ComboBox);
	fillModeComboBox(mSideButton2ComboBox);
	mouseBody->addWidget(formRow(mouseCard, tr("侧键 1（后退）"), mSideButton1ComboBox));
	mouseBody->addWidget(formRow(mouseCard, tr("侧键 2（前进）"), mSideButton2ComboBox));

	layout->addWidget(mouseCard);
	layout->addStretch();
	return page;
}

QWidget* LiteSettingsDialog::createCaptureTab()
{
	auto page = new QWidget(this);
	auto layout = new QVBoxLayout(page);
	layout->setContentsMargins(0, 0, 0, 4);
	layout->setSpacing(14);

	QVBoxLayout *captureBody;
	auto captureCard = Ui::card(page, tr("截图"), tr("控制取景方式与截取范围。"), captureBody);

	// Discrete presets instead of a spin box: matches the other dropdowns and
	// avoids the Qt style-sheet quirk that clips a spin box's first glyph.
	for(int seconds : {0, 1, 2, 3, 5, 8, 10, 15, 20, 30}) {
		mDelayComboBox->addItem(seconds == 0 ? tr("不延迟") : tr("%1 秒").arg(seconds), seconds);
	}
	mDelayComboBox->setFixedWidth(140);
	Ui::setControlHeight(mDelayComboBox, kControlHeight);
	captureBody->addWidget(formRow(captureCard, tr("截图延迟"), mDelayComboBox, false));

	mCaptureCursorCheckBox->setText(tr("截图中包含鼠标光标"));
	captureBody->addWidget(checkboxRow(captureCard, mCaptureCursorCheckBox));

	mFreezeScreenCheckBox->setText(tr("选择区域时冻结屏幕"));
	captureBody->addWidget(checkboxRow(captureCard, mFreezeScreenCheckBox));
	layout->addWidget(captureCard);

	QVBoxLayout *saveBody;
	auto saveCard = Ui::card(page, tr("保存"), tr("截图完成后的落盘与剪贴板行为。"), saveBody);

	auto directoryField = new QWidget(saveCard);
	auto directoryLayout = new QHBoxLayout(directoryField);
	directoryLayout->setContentsMargins(0, 0, 0, 0);
	directoryLayout->setSpacing(8);
	Ui::setControlHeight(mSaveDirectoryLineEdit, kControlHeight);
	directoryLayout->addWidget(mSaveDirectoryLineEdit, 1);
	auto browseButton = Ui::button(tr("浏览"), Ui::ButtonVariant::Outline, saveCard);
	browseButton->setMinimumWidth(72);
	connect(browseButton, &QPushButton::clicked, this, &LiteSettingsDialog::browseSaveDirectory);
	directoryLayout->addWidget(browseButton);
	saveBody->addWidget(formRow(saveCard, tr("保存目录"), directoryField));

	mSaveFormatComboBox->addItems({QLatin1String("png"), QLatin1String("jpg"), QLatin1String("bmp"), QLatin1String("webp")});
	mSaveFormatComboBox->setFixedWidth(132);
	Ui::setControlHeight(mSaveFormatComboBox, kControlHeight);
	saveBody->addWidget(formRow(saveCard, tr("保存格式"), mSaveFormatComboBox, false));

	mAutoCopyCheckBox->setText(tr("新截图自动复制到剪贴板"));
	saveBody->addWidget(checkboxRow(saveCard, mAutoCopyCheckBox));

	mAutoSaveCheckBox->setText(tr("新截图自动保存"));
	saveBody->addWidget(checkboxRow(saveCard, mAutoSaveCheckBox));

	layout->addWidget(saveCard);
	layout->addStretch();
	return page;
}

QWidget* LiteSettingsDialog::createOcrTab()
{
	auto page = new QWidget(this);
	auto layout = new QVBoxLayout(page);
	layout->setContentsMargins(0, 0, 0, 4);
	layout->setSpacing(14);

	QVBoxLayout *engineBody;
	auto engineCard = Ui::card(page, tr("识别引擎"),
			tr("关闭时使用 Windows 原生 OCR，无需安装依赖；开启后改用本地 Ollama 大模型识别。"), engineBody);
	mOllamaEnabledCheckBox->setText(tr("启用大模型识别（Ollama）"));
	engineBody->addWidget(checkboxRow(engineCard, mOllamaEnabledCheckBox));
	layout->addWidget(engineCard);

	QVBoxLayout *ollamaBody;
	auto ollamaCard = Ui::card(page, tr("Ollama 配置"), tr("本地大模型识别的服务地址、视觉模型与提示词。"), ollamaBody);
	mOllamaCard = ollamaCard;

	mOllamaUrlLineEdit->setPlaceholderText(QLatin1String("http://127.0.0.1:11434"));
	Ui::setControlHeight(mOllamaUrlLineEdit, kControlHeight);
	ollamaBody->addWidget(formRow(ollamaCard, tr("服务地址"), mOllamaUrlLineEdit));

	mOllamaModelLineEdit->setPlaceholderText(QLatin1String("xiaomi-ocr-0:bf16"));
	Ui::setControlHeight(mOllamaModelLineEdit, kControlHeight);
	ollamaBody->addWidget(formRow(ollamaCard, tr("视觉模型"), mOllamaModelLineEdit));

	mOllamaPromptEdit->setFixedHeight(84);
	ollamaBody->addWidget(formBlock(ollamaCard, tr("提示词"), mOllamaPromptEdit));

	auto testLayout = new QHBoxLayout;
	testLayout->setContentsMargins(0, 0, 0, 0);
	testLayout->setSpacing(12);
	mOcrTestButton->setText(tr("测试连接"));
	mOcrTestButton->setProperty("variant", QLatin1String("secondary"));
	mOcrTestButton->setCursor(Qt::PointingHandCursor);
	Ui::repolish(mOcrTestButton);
	Ui::setControlHeight(mOcrTestButton, kControlHeight);
	connect(mOcrTestButton, &QPushButton::clicked, this, &LiteSettingsDialog::testOcrConnection);
	// Keep the action on the field column so it lines up with the inputs above.
	testLayout->addSpacing(kLabelWidth + kFieldGap);
	testLayout->addWidget(mOcrTestButton, 0, Qt::AlignVCenter);

	mOcrTestResultLabel->setWordWrap(true);
	QFont smallFont = mOcrTestResultLabel->font();
	smallFont.setPixelSize(13);
	mOcrTestResultLabel->setFont(smallFont);
	testLayout->addWidget(mOcrTestResultLabel, 1, Qt::AlignVCenter);
	ollamaBody->addLayout(testLayout);
	layout->addWidget(ollamaCard);

	connect(mOllamaEnabledCheckBox, &QCheckBox::toggled, this, [this](bool enabled){
		if(mOllamaCard != nullptr) {
			mOllamaCard->setEnabled(enabled);
		}
	});

	layout->addStretch();
	return page;
}

QWidget* LiteSettingsDialog::createGeneralTab()
{
	auto page = new QWidget(this);
	auto layout = new QVBoxLayout(page);
	layout->setContentsMargins(0, 0, 0, 4);
	layout->setSpacing(14);

	QVBoxLayout *appearanceBody;
	auto appearanceCard = Ui::card(page, tr("外观"), tr("选择浅色、深色，或跟随 Windows 系统设置。"), appearanceBody);
	mAppearanceComboBox->addItem(tr("跟随系统"), QLatin1String("system"));
	mAppearanceComboBox->addItem(tr("浅色"), QLatin1String("light"));
	mAppearanceComboBox->addItem(tr("深色"), QLatin1String("dark"));
	mAppearanceComboBox->setAccessibleName(tr("外观"));
	mAppearanceComboBox->setFixedWidth(160);
	Ui::setControlHeight(mAppearanceComboBox, kControlHeight);
	appearanceBody->addWidget(formRow(appearanceCard, tr("主题"), mAppearanceComboBox, false));
	layout->addWidget(appearanceCard);

	QVBoxLayout *behaviorBody;
	auto behaviorCard = Ui::card(page, tr("行为"), tr("本工具常驻系统托盘，通过快捷键或鼠标侧键截图。"), behaviorBody);

	mAutoStartCheckBox->setText(tr("开机自动启动"));
	behaviorBody->addWidget(checkboxRow(behaviorCard, mAutoStartCheckBox));

	mShowSnippingHintCheckBox->setText(tr("截图时显示左上角操作提示"));
	mShowSnippingHintCheckBox->setToolTip(tr("默认仅首次截图显示，之后自动关闭；勾选可重新开启"));
	behaviorBody->addWidget(checkboxRow(behaviorCard, mShowSnippingHintCheckBox));
	layout->addWidget(behaviorCard);

	// The base height is the tallest page, so this one needs a third card or it
	// ends in a hole. It also carries the version and licence that the removed
	// About dialog used to hold, which nothing else in the Lite build exposes.
	QVBoxLayout *metaBody;
	auto metaCard = Ui::card(page, tr("配置与许可"), tr("便携运行：设置写在程序目录的 config.ini，不写注册表。"), metaBody);

	auto openFolder = Ui::button(tr("打开所在目录"), Ui::ButtonVariant::Outline, metaCard);
	openFolder->setMinimumWidth(120);
	openFolder->setToolTip(AppInfo::configFilePath());
	connect(openFolder, &QPushButton::clicked, this, [](){
		QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(AppInfo::configFilePath()).absolutePath()));
	});
	metaBody->addWidget(formRow(metaCard, tr("配置文件"), openFolder, false));

	auto licenseField = new QWidget(metaCard);
	auto licenseLayout = new QHBoxLayout(licenseField);
	licenseLayout->setContentsMargins(0, 0, 0, 0);
	licenseLayout->setSpacing(8);
	auto licenseLabel = new QLabel(tr("GPL-3.0 · 基于 ksnip %1").arg(QLatin1String(KSNIP_VERSION)), licenseField);
	licenseLabel->setProperty("role", QLatin1String("mono"));
	licenseLayout->addWidget(licenseLabel, 0, Qt::AlignVCenter);
	auto sourceButton = Ui::button(tr("源码仓库"), Ui::ButtonVariant::Outline, metaCard);
	sourceButton->setMinimumWidth(96);
	connect(sourceButton, &QPushButton::clicked, this, [](){
		QDesktopServices::openUrl(QUrl(QLatin1String("https://github.com/EtherealX2/Shizhen")));
	});
	licenseLayout->addWidget(sourceButton, 0, Qt::AlignVCenter);
	// Not full width: the pair keeps a readable distance from the licence text
	// instead of pinning the button to the far edge of the card.
	metaBody->addWidget(formRow(metaCard, tr("开源许可"), licenseField, false));
	layout->addWidget(metaCard);

	layout->addStretch();
	return page;
}

void LiteSettingsDialog::fillModeComboBox(QComboBox *comboBox)
{
	comboBox->addItem(tr("禁用"), -1);
	comboBox->addItem(tr("矩形区域"), static_cast<int>(CaptureModes::RectArea));
	comboBox->addItem(tr("活动窗口"), static_cast<int>(CaptureModes::ActiveWindow));
	comboBox->addItem(tr("全屏"), static_cast<int>(CaptureModes::FullScreen));
	comboBox->addItem(tr("当前屏幕"), static_cast<int>(CaptureModes::CurrentScreen));
	comboBox->addItem(tr("上次矩形区域"), static_cast<int>(CaptureModes::LastRectArea));
	Ui::setControlHeight(comboBox, kControlHeight);
}

void LiteSettingsDialog::loadSettings()
{
	mMouseHookEnabledCheckBox->setChecked(mConfig->mouseHookEnabled());
	mSideButton1ComboBox->setCurrentIndex(mSideButton1ComboBox->findData(mConfig->mouseSideButton1Mode()));
	mSideButton2ComboBox->setCurrentIndex(mSideButton2ComboBox->findData(mConfig->mouseSideButton2Mode()));

	auto delaySeconds = mConfig->captureDelay() / 1000;
	auto delayIndex = mDelayComboBox->findData(delaySeconds);
	mDelayComboBox->setCurrentIndex(delayIndex >= 0 ? delayIndex : 0);
	mCaptureCursorCheckBox->setChecked(mConfig->captureCursor());
	mFreezeScreenCheckBox->setChecked(mConfig->freezeImageWhileSnippingEnabled());
	mSaveDirectoryLineEdit->setText(QDir::toNativeSeparators(mConfig->saveDirectory()));
	mSaveFormatComboBox->setCurrentText(mConfig->saveFormat());
	mAutoCopyCheckBox->setChecked(mConfig->autoCopyToClipboardNewCaptures());
	mAutoSaveCheckBox->setChecked(mConfig->autoSaveNewCaptures());

	mOllamaUrlLineEdit->setText(mConfig->ollamaUrl());
	mOllamaModelLineEdit->setText(mConfig->ollamaModel());
	mOllamaPromptEdit->setPlainText(mConfig->ollamaPrompt());
	auto ollamaOn = mConfig->ollamaEnabled();
	mOllamaEnabledCheckBox->setChecked(ollamaOn);
	if(mOllamaCard != nullptr) {
		mOllamaCard->setEnabled(ollamaOn);
	}

	mAutoStartCheckBox->setChecked(autoStartEnabled());
	mShowSnippingHintCheckBox->setChecked(mConfig->showSnippingAreaInfoText());
	mAppearanceComboBox->setCurrentIndex(mAppearanceComboBox->findData(mConfig->appearanceTheme()));
}

void LiteSettingsDialog::saveSettings()
{
	mHotKeySettings->saveSettings();

	mConfig->setMouseHookEnabled(mMouseHookEnabledCheckBox->isChecked());
	mConfig->setMouseSideButton1Mode(mSideButton1ComboBox->currentData().toInt());
	mConfig->setMouseSideButton2Mode(mSideButton2ComboBox->currentData().toInt());

	mConfig->setCaptureDelay(mDelayComboBox->currentData().toInt() * 1000);
	mConfig->setCaptureCursor(mCaptureCursorCheckBox->isChecked());
	mConfig->setFreezeImageWhileSnippingEnabled(mFreezeScreenCheckBox->isChecked());
	mConfig->setSaveDirectory(QDir::fromNativeSeparators(mSaveDirectoryLineEdit->text()));
	mConfig->setSaveFormat(mSaveFormatComboBox->currentText());
	mConfig->setAutoCopyToClipboardNewCaptures(mAutoCopyCheckBox->isChecked());
	mConfig->setAutoSaveNewCaptures(mAutoSaveCheckBox->isChecked());

	mConfig->setOllamaUrl(mOllamaUrlLineEdit->text());
	mConfig->setOllamaModel(mOllamaModelLineEdit->text());
	mConfig->setOllamaPrompt(mOllamaPromptEdit->toPlainText());
	mConfig->setOllamaEnabled(mOllamaEnabledCheckBox->isChecked());

	setAutoStartEnabled(mAutoStartCheckBox->isChecked());
	mConfig->setShowSnippingAreaInfoText(mShowSnippingHintCheckBox->isChecked());
	mConfig->setAppearanceTheme(mAppearanceComboBox->currentData().toString());
}

void LiteSettingsDialog::browseSaveDirectory()
{
	auto directory = mFileDialogService->getExistingDirectory(this, tr("选择保存目录"), mSaveDirectoryLineEdit->text());
	if(!directory.isEmpty()) {
		mSaveDirectoryLineEdit->setText(QDir::toNativeSeparators(directory));
	}
}

void LiteSettingsDialog::testOcrConnection()
{
	mConfig->setOllamaUrl(mOllamaUrlLineEdit->text());
	mOcrTestResultLabel->setText(tr("测试中…"));
	mOcrTestResultLabel->setProperty("tone", QString());
	Ui::repolish(mOcrTestResultLabel);
	mOcr->testConnection();
}

bool LiteSettingsDialog::autoStartEnabled() const
{
#ifdef Q_OS_WIN
	QSettings runKey(QLatin1String("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run"), QSettings::NativeFormat);
	return runKey.contains(QLatin1String("Shizhen"));
#else
	return false;
#endif
}

void LiteSettingsDialog::setAutoStartEnabled(bool enabled)
{
#ifdef Q_OS_WIN
	QSettings runKey(QLatin1String("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run"), QSettings::NativeFormat);
	if(enabled) {
		runKey.setValue(QLatin1String("Shizhen"), QDir::toNativeSeparators(QCoreApplication::applicationFilePath()));
	} else {
		runKey.remove(QLatin1String("Shizhen"));
	}
#else
	Q_UNUSED(enabled)
#endif
}
