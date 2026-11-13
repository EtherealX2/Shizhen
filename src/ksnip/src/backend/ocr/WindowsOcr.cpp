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

#include "WindowsOcr.h"

#include <QDir>
#include <QFile>
#include <QProcessEnvironment>
#include <QUuid>

namespace {
const char *ocrScript = R"PS(
$ErrorActionPreference = 'Stop'
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
Add-Type -AssemblyName System.Runtime.WindowsRuntime
$asTaskGeneric = ([System.WindowsRuntimeSystemExtensions].GetMethods() | Where-Object { $_.Name -eq 'AsTask' -and $_.GetParameters().Count -eq 1 -and $_.GetParameters()[0].ParameterType.Name -eq 'IAsyncOperation`1' })[0]
function Await($WinRtTask, $ResultType) {
  $asTask = $asTaskGeneric.MakeGenericMethod($ResultType)
  $netTask = $asTask.Invoke($null, @($WinRtTask))
  $netTask.Wait(-1) | Out-Null
  $netTask.Result
}
[Windows.Media.Ocr.OcrEngine,Windows.Foundation,ContentType=WindowsRuntime] | Out-Null
[Windows.Media.Ocr.OcrResult,Windows.Foundation,ContentType=WindowsRuntime] | Out-Null
[Windows.Graphics.Imaging.BitmapDecoder,Windows.Foundation,ContentType=WindowsRuntime] | Out-Null
[Windows.Graphics.Imaging.SoftwareBitmap,Windows.Foundation,ContentType=WindowsRuntime] | Out-Null
[Windows.Storage.StorageFile,Windows.Foundation,ContentType=WindowsRuntime] | Out-Null
[Windows.Storage.Streams.IRandomAccessStream,Windows.Foundation,ContentType=WindowsRuntime] | Out-Null
try {
$engine = [Windows.Media.Ocr.OcrEngine]::TryCreateFromUserProfileLanguages()
if ($null -eq $engine) { throw 'Windows OCR engine unavailable (missing language pack?)' }
$file = Await ([Windows.Storage.StorageFile]::GetFileFromPathAsync($env:KSNIP_OCR_IMAGE)) ([Windows.Storage.StorageFile])
$stream = Await ($file.OpenAsync([Windows.Storage.FileAccessMode]::Read)) ([Windows.Storage.Streams.IRandomAccessStream])
$decoder = Await ([Windows.Graphics.Imaging.BitmapDecoder]::CreateAsync($stream)) ([Windows.Graphics.Imaging.BitmapDecoder])
$bitmap = Await ($decoder.GetSoftwareBitmapAsync()) ([Windows.Graphics.Imaging.SoftwareBitmap])
if ($bitmap.PixelWidth -gt $engine.MaxImageDimension -or $bitmap.PixelHeight -gt $engine.MaxImageDimension) {
  $scale = [Math]::Min([double]$engine.MaxImageDimension / $bitmap.PixelWidth, [double]$engine.MaxImageDimension / $bitmap.PixelHeight)
  $transform = New-Object Windows.Graphics.Imaging.BitmapTransform
  $transform.ScaledWidth = [uint32]($bitmap.PixelWidth * $scale)
  $transform.ScaledHeight = [uint32]($bitmap.PixelHeight * $scale)
  $bitmap = Await ($decoder.GetSoftwareBitmapAsync($bitmap.BitmapPixelFormat, $bitmap.BitmapAlphaMode, $transform, [Windows.Graphics.Imaging.ExifOrientationMode]::IgnoreExifOrientation, [Windows.Graphics.Imaging.ColorManagementMode]::DoNotColorManage)) ([Windows.Graphics.Imaging.SoftwareBitmap])
}
$result = Await ($engine.RecognizeAsync($bitmap)) ([Windows.Media.Ocr.OcrResult])
# Join per-line text with LF so the original line breaks survive into the result box.
$lineTexts = @($result.Lines | ForEach-Object { $_.Text })
[Console]::WriteLine([string]::Join("`n", $lineTexts))
} catch {
  $ex = $_.Exception
  while ($null -ne $ex.InnerException) { $ex = $ex.InnerException }
  [Console]::WriteLine($ex.Message)
  exit 1
}
)PS";
}

WindowsOcr::WindowsOcr(QObject *parent) :
	QObject(parent),
	mProcess(this),
	mTimeoutTimer(this),
	mCancelled(false)
{
	mTimeoutTimer.setSingleShot(true);
	mTimeoutTimer.setInterval(30000);
	connect(&mTimeoutTimer, &QTimer::timeout, this, [this](){
		mProcess.kill();
		removeTempFile();
		emit failed(tr("Windows OCR 超时"));
	});
	connect(&mProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
			this, [this](int exitCode, QProcess::ExitStatus exitStatus){
		mTimeoutTimer.stop();
		removeTempFile();
		// A cancelled run must stay silent: the window it belonged to is gone.
		if(mCancelled) {
			mCancelled = false;
			return;
		}
		auto text = QString::fromUtf8(mProcess.readAllStandardOutput()).trimmed();
		if(exitStatus == QProcess::NormalExit && exitCode == 0) {
			if(text.isEmpty()) {
				emit failed(tr("未识别到文字"));
			} else {
				emit finished(text);
			}
		} else {
			auto error = text;
			if(error.isEmpty()) {
				error = QString::fromLocal8Bit(mProcess.readAllStandardError()).trimmed();
			}
			emit failed(error.isEmpty() ? tr("Windows OCR 识别失败") : error);
		}
	});
	connect(&mProcess, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error){
		if(error == QProcess::FailedToStart) {
			mTimeoutTimer.stop();
			removeTempFile();
			emit failed(tr("无法启动 PowerShell，Windows OCR 不可用"));
		}
	});
	// CREATE_NO_WINDOW, so the PowerShell console never shows.
	mProcess.setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments *args){
		args->flags |= 0x08000000;
	});
}

WindowsOcr::~WindowsOcr()
{
	if(mProcess.state() != QProcess::NotRunning) {
		mProcess.kill();
		mProcess.waitForFinished(1000);
	}
	removeTempFile();
}

void WindowsOcr::recognize(const QImage &image)
{
	if(isBusy()) {
		return;
	}
	mCancelled = false;
	removeTempFile();
	mTempPath = QDir::tempPath() + QLatin1String("/shizhen-ocr-") + QUuid::createUuid().toString().mid(1, 36) + QLatin1String(".png");
	if(!image.save(mTempPath, "PNG")) {
		emit failed(tr("无法写入临时图片"));
		return;
	}
	auto env = QProcessEnvironment::systemEnvironment();
	// WinRT GetFileFromPathAsync rejects forward slashes, so use native separators.
	env.insert(QLatin1String("KSNIP_OCR_IMAGE"), QDir::toNativeSeparators(mTempPath));
	mProcess.setProcessEnvironment(env);
	mProcess.start(QLatin1String("powershell.exe"),
				   QStringList() << QLatin1String("-NoProfile") << QLatin1String("-NonInteractive")
								 << QLatin1String("-Command") << QString::fromUtf8(ocrScript));
	mTimeoutTimer.start();
}

bool WindowsOcr::isBusy() const
{
	return mProcess.state() != QProcess::NotRunning;
}

void WindowsOcr::cancel()
{
	mTimeoutTimer.stop();
	if(mProcess.state() != QProcess::NotRunning) {
		mCancelled = true;
		mProcess.kill();
		mProcess.waitForFinished(500);
	}
	mCancelled = false;
	removeTempFile();
}

void WindowsOcr::removeTempFile()
{
	if(!mTempPath.isEmpty()) {
		QFile::remove(mTempPath);
		mTempPath.clear();
	}
}