# 开发与构建记录

本文面向维护者，记录拾帧的构建方式、瘦身过程、代码改动与实机验证数据。面向使用者的功能说明见根目录 [`README.md`](../README.md)。

## 一、目录结构

```
ksnip/
├─ Shizhen/                绿色版，整夹可拷任意路径运行（38.4 MB）
│  ├─ Shizhen.exe          3.4 MB
│  ├─ README.txt           给使用者看的说明
│  ├─ Qt5*.dll(8)          Core / Gui / Widgets / Network / Svg / PrintSupport / Xml / WinExtras
│  ├─ libgcc_s_seh-1.dll、libstdc++-6.dll、libwinpthread-1.dll
│  ├─ platforms\ imageformats\ iconengines\ printsupport\   Qt 插件（最小集）
│  └─ translations\        仅 zh_CN / zh_Hant
├─ src/                    完整可编译源码
│  ├─ ksnip/  kImageAnnotator/  kColorPicker/
│  └─ build-official-qt.sh 一键重建脚本（官方 Qt 工具链）
├─ screenshots/            实机截图（浅色 / 深色）
├─ docs/                   本文件
└─ README.md
```

## 二、瘦身结果

| 指标 | 改造前（MSYS2 Qt） | 改造后（Qt 官方 + MinGW 8.1） |
|---|---|---|
| 目录体积 | **80 MB** | **38.4 MB**（−52%） |
| 根目录 DLL 数 | 30 个 | **11 个** |
| 文件总数 | 43 | 21 |
| 最大单体 | `libicudt78.dll` 33.1 MB | `Qt5Gui.dll` 9.2 MB |
| 字体/编码栈 | ICU + HarfBuzz + Graphite2 + GLib + FreeType + iconv + intl + bz2 + brotli + zstd + pcre2 … | 无（Qt 内部静态编译，走系统 DirectWrite/GDI 与 Win32 NLS） |

被彻底移除的依赖（原包内实际存在）：

| DLL | 大小 | 为什么可以没有 |
|---|---|---|
| `libicudt78.dll` | 33.1 MB | Qt 官方 Windows 包用 `-no-icu` 编译，本地化走 Win32 NLS |
| `libicuin78.dll` / `libicuuc78.dll` | 3.2 + 2.0 MB | 同上 |
| `libharfbuzz-0.dll` / `libgraphite2.dll` | 1.9 + 0.2 MB | 官方包把 HarfBuzz 静态编进 Qt5Gui，字形回退交给 DirectWrite |
| `libglib-2.0-0.dll` / `libintl-8.dll` | 1.6 + 0.3 MB | MSYS2 交叉编译链的产物，Windows 下不需要 |
| `libfreetype-6.dll` / `libpng16-16.dll` | 0.8 + 0.3 MB | 官方包内置静态 FreeType / libpng |
| `libiconv-2.dll`、`libbz2-1.dll`、`libbrotli*.dll`、`libzstd.dll`、`libpcre2-*.dll`、`libmd4c.dll`、`libdouble-conversion.dll`、`libwebp*.dll`、`libsharpyuv-0.dll`、`zlib1.dll` | ≈ 5 MB | 都被静态编入官方 Qt5Core / Qt5Gui |
| `qwindowsvistastyle.dll` | 0.2 MB | 界面固定 Fusion 风格，不需要原生样式插件 |

依赖闭包现在只剩：8 个 Qt DLL + 3 个 MinGW 运行时 DLL + 系统 DLL（`objdump` 实测，无其他三方 DLL）。

另外：

- **MinGW 运行时静态链接**（`-static-libgcc -static-libstdc++`），程序自身不再依赖 `libstdc++`；包里那 3 个运行时 DLL 是 `Qt5Core.dll` 自己要用的（官方 Qt 用 GCC 8.1 编译）。
- **`-s` 剥离符号表**，exe 从 3.8 MB 降到 3.4 MB。
- **翻译裁剪**：只保留 `zh_CN` / `zh_Hant`，translations 从 1.5 MB 降到 88 KB。
- 未使用 UPX 压缩（避免杀软误报与启动解压开销）。

## 三、代码与配置层面的 Lite 化

### 1. 便携模式（真·配置随包走）

- 新增 `src/common/AppInfo.h`：品牌名、tagline、配置路径的唯一来源。
- `Config` 与 `ImagePathStorage` 不再使用默认 `QSettings`（注册表），改为显式 `QSettings(<exe目录>/config.ini, IniFormat)`。
- `kImageAnnotator` 的 `SettingsAdapter` 支持通过 `SHIZHEN_CONFIG` 环境变量指向同一份 ini，标注工具的颜色/字号等设置也写进 `config.ini`，不再写 `HKCU\Software\…`。
- 程序目录只读时（例如装在 `Program Files`），自动回退到 `%LOCALAPPDATA%\Shizhen\Shizhen\config.ini`，不会静默丢设置。
- 实测：删除 `HKCU\Software\Shizhen` 后运行，注册表**不再被创建**，`config.ini` 在同级目录生成。

### 2. 网络与上传模块

- 菜单里的 **Upload（Imgur / FTP / 脚本上传）入口已移除**，`Shift+U` 快捷键一并取消，纯本地使用不会触发任何外链上传。
- `Qt5Network` 仍保留（2.6 MB）：**OCR 的 Ollama 通道需要它**（http://127.0.0.1:11434）。Windows 原生 OCR 通道不联网。
- 因此没有引入 OpenSSL：`libssl` / `libcrypto` 一个都不在包里（官方 Qt 的 OpenSSL 是运行时按需加载）。

### 3. 品牌与文案

- `QApplication::setApplicationDisplayName("拾帧")`，窗口标题自动变成 `截图 - 拾帧`。
- 托盘提示、提示气泡、设置/关于/OCR 结果窗标题、单实例 IPC 名、临时文件名、默认保存文件名（`Shizhen_$Y$M$D-$T`）、开机自启注册表项名，全部改为拾帧 / `Shizhen`。
- `ksnip_zh_CN.ts` / `ksnip_zh_Hant.ts` 中所有译文里的 “ksnip” 改为「拾帧」（保留原作者署名：关于页写明“基于开源的 ksnip 1.11.0 构建，(C) 2021 Damir Porobic”）。

### 4. 工具链适配（官方 Qt 的差异点）

- `src/CMakeLists.txt` 增加 `-D_WIN32_WINNT=0x0603 -DWINVER=0x0603`：MinGW 8.1 头文件默认目标版本过低，`MOD_NOREPEAT`、`ICONINFOEXW` 等 Win7+ API 未声明。
- `OUTPUT_NAME` 改为 `Shizhen`，内部 target 名不变，避免大面积改动。
- `windeployqt` 在纯净环境里会报 “Unable to find the platform plugin”，构建脚本改为**直接拷贝最小插件集**，结果可控且可复现。
- `build-official-qt.sh` 里 `ROOT` 需用 `cygpath -m` 转成 `D:/...`：MSYS 风格 `/d/...` 传给 `CMAKE_PREFIX_PATH` 时，ksnip 的 `find_package(kColorPicker-Qt5)` 会失败。

## 四、界面实现（BoardUI 设计语言）

| 层次 | 说明 |
|---|---|
| 令牌层 | `gui/style/Theme.cpp` 按 [BoardUI](https://www.boardui.com/zh-hans) 的语义令牌重写（background / card / raised / field / muted / border / input / primary / ring / destructive / success）。画布着色、卡片纯白、hairline 描边，主按钮是 `accent-500 → accent-600` 的垂直渐变 |
| 字阶 | Title 20/600（对话框标题）、Headline 16/600（卡片标题）、Body 14、Body2 13（说明）、Caption 12；字段标签 14/500，正文行高由控件高度 36px 约束 |
| 圆角 | 按钮 / 输入框 10px，卡片与菜单 12px，悬浮工具条 14px，小控件 8px |
| 抬升表面 | `surfaceRaised()`：浅色主题取纯白、深色主题取比卡片亮一档的灰 —— 深色下选中态不能比容器更暗，否则看起来像“按下去了” |
| 工厂层 | `gui/style/Ui.h/.cpp`：`card()` `hline()` `cardTitle()` `mutedLabel()` `button(text, 变体)`，5 种按钮变体在四个设置页复用 |
| 设置对话框 | 分段控件替代 QTabWidget；自绘 footer（取消 outline + 确定 primary）；卡片分组、两列对齐；四页共用同一个基底高度（取最高页，首次显示时测量一次），切换分段时窗口不改变尺寸也不重新居中 |
| 悬浮工具条 | 圆角容器 + 投影、32×32 图标按钮、竖线分隔、✕ ghost / ✔ 蓝色渐变。选中工具用 accent 10% 底 + 30% 描边 |
| 图标 | 56 个 Lucide 风格 SVG，每图标两套墨色（Qt SVG 不支持 `currentColor`），`Theme::icon()` 按主题挑选 |
| 已知限制 | QSS 无过渡动画，渐变只能是两档静态色；`QFrame` 必须设 `WA_StyledBackground` 才会绘制背景；`QFrame#x:disabled QLabel` 这类后代选择器在 Qt 5.15 里会误命中所有实例，改用别的表达方式；`QSpinBox` 在 QSS 下首个字形会被裁切，改用下拉框 |

## 五、OCR 结果窗交互修复（两轮）

第一轮：截图悬浮窗与 OCR 结果窗都是 topmost，点击悬浮窗会把结果窗压到下面，看起来像卡死 → 当时把结果窗改成了 `Qt::WindowModal`。

第二轮（真正的“识别后关不掉悬浮窗”）：`WindowModal` 会在 Win32 层对悬浮窗执行 `EnableWindow(FALSE)`。实测弹窗打开时悬浮窗 `IsWindowEnabled=False`：工具条上的放弃/完成/保存、画布点击、Esc 全部失效，用户只能先关掉结果窗。改法：

- 结果窗恢复**非模态**（“卫星窗”），悬浮窗全程可操作，X / Esc / 完成随时能关掉整组窗口；
- 悬浮窗被激活时用 `raise()`（`SWP_NOACTIVATE`，不抢焦点、不改变输入）把结果窗重新抬到最上层，既不会像以前那样被压到画布下面，也不会出现 z 序反复争夺的卡顿；
- 结果窗优先贴在截图窗右侧 / 左侧 / 下方 / 上方并夹紧到屏幕内，不遮挡画布和工具条；全屏截图没有空位时回落到屏幕居中；
- “保存”前与关闭悬浮窗时统一走 `closeOcrDialog()`，不会再残留孤儿结果窗；OCR 进行中直接关窗仍由 `mIsClosing` + 取消机制拦住迟到结果。

其余加固：单实例 `QPointer<OcrResultDialog>`；`QPlainTextEdit` + 显示后延时填充；显示上限 20 万字符（复制仍是全文）；`closeEvent` 取消 OCR；Ollama 响应 > 8MB 判失败。

## 六、重新编译

```bash
# 一次性准备（可用清华 pypi 源装 aqtinstall）
python -m aqt install-qt windows desktop 5.15.2 win64_mingw81 -O C:/Qt \
       --archives qtbase qtsvg qtwinextras
python -m aqt install-tool windows desktop tools_mingw qt.tools.win64_mingw810 -O C:/Qt

# 构建（在 MSYS2 / Git Bash 里执行，默认找 C:/Qt，可用 QTDIR / MINGW 覆盖）
cd ksnip/src
bash build-official-qt.sh
```

脚本会依次编译 kColorPicker → kImageAnnotator → ksnip，安装到 `../Shizhen`，并部署最小 Qt 运行库。实测从零构建约 1 分 24 秒（8 核）。

改 QSS / 图标需要重编，因为 `.qrc` 编进了 exe。

## 七、验证记录（交付版实测）

| 项目 | 结果 |
|---|---|
| 启动 | 进程常驻、无报错；到首个可见窗口 ≈1.3 s（其中含 800 ms 常驻提示延迟） |
| 内存 | 空闲 4 s 后：私有 ≈30.8 MB，工作集 ≈60 MB（工作集含 Qt DLL 共享页），15 线程 |
| 便携 | 同级生成 `config.ini`；`HKCU\Software\Shizhen` 不再创建 |
| 依赖 | `objdump` 全量扫描：仅 8 个 Qt DLL + 3 个 MinGW 运行时 + 系统 DLL |
| 界面 | 设置四页、悬浮工具条、OCR 弹窗、提示气泡在浅 / 深色下均正常，图标无缺失（`iconengines/qsvgicon.dll`、`imageformats/qsvg.dll` 必须随包，缺了图标会变空白方块） |
| 功能 | 区域截图 → 工具条 → OCR → 复制到剪贴板（实测识别 506 字符文本）→ 关闭，全流程通过 |
| OCR 关闭回归 | 弹窗打开时点“放弃”/“完成”均立即关闭整组窗口；`Esc` 同样生效；点击画布后结果窗仍在最上层（无压层、无卡顿）；识别中直接关窗不会冒出孤儿结果窗；重复点 OCR 只保留一个结果窗（UIA 自动化实测） |

## 八、还可以继续做的

- **换 MSVC + LTCG**：理论上还能再小 2–4 MB，并能去掉那 3 个 MinGW 运行时 DLL；代价是要改 CMake 工具链、重编三个模块，且 Qt 官方 MSVC 包同样不带 ICU。
- **静态 Qt（`-static` + `qt5-static`）**：可做成单文件 exe，但 MSYS2 的静态 Qt 带静态 ICU，体积反而回涨到 50 MB 以上，不划算。
- **恢复多语言**：现在只留中文；若要出英文 / 日文版，把 `translations/*.qm` 放回，并把 `src/ksnip/translations` 对应 `.ts` 加入构建即可。
