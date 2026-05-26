# ImageNodeEditor 节点式图像处理工具

ImageNodeEditor 是一个使用 C++、Qt6、CMake 和 QImage 实现的节点式图像处理工具。程序支持在图形界面中添加图像处理节点、连接端口、修改参数、执行工作流并预览结果，也支持通过命令行加载 JSON 工作流后无界面执行。

本项目不依赖 OpenCV，所有图像处理算法均基于 Qt 自带的 `QImage`、`QPainter` 等类实现，便于在普通 Qt 环境中编译和提交。

## 功能概览

- 支持可视化节点画布：添加节点、移动节点、拖拽端口连线、删除节点或连线。
- 支持参数面板：选中节点后可修改路径、数值、开关、颜色、模式等参数。
- 支持节点输出预览：运行工作流后，选中带有 `image` 输出的节点即可查看结果。
- 支持工作流保存和加载：使用 JSON 保存节点、位置、参数和连接关系。
- 支持命令行执行：`--workflow workflow.json --no-gui` 可直接执行工作流并自动退出。
- 支持国际化：目前支持英文和简体中文，可通过系统语言、GUI 菜单或 `--lang` 参数选择。
- 支持类型安全端口：`Image`、`Mask`、`ImageList` 三类端口严格匹配，错误连接会被拒绝。
- 支持 DAG 校验：连接时和加载 JSON 后都会检查环路，保证工作流是有向无环图。
- 支持错误提示：文件缺失、图片读取失败、导出失败、端口类型不匹配、缺少必需输入等情况不会崩溃，会给出清晰错误信息。

## 已实现节点

| 节点类型 | 功能 | 输入 | 输出 |
|---|---|---|---|
| `ImageInput` | 从路径读取图片 | 无 | `Image` |
| `ImageExport` | 导出图片到文件 | `Image` | `Image` |
| `Display` | 用于标记和预览中间图像 | `Image` | `Image` |
| `Crop` | 裁剪图像 | `Image` | `Image` |
| `Resize` | 缩放图像，可保持宽高比 | `Image` | `Image` |
| `RotateFlip` | 旋转和水平/垂直翻转 | `Image` | `Image` |
| `Blur` | 盒式模糊 | `Image` | `Image` |
| `Sharpen` | 卷积锐化 | `Image` | `Image` |
| `BrightnessContrast` | 亮度和对比度调整 | `Image` | `Image` |
| `Grayscale` | 灰度化，支持亮度/平均/R/G/B 通道模式 | `Image` | `Image` |
| `ThresholdMask` | 按阈值生成遮罩 | `Image` | `Mask` |
| `Blend` | 两张图混合，可选遮罩 | `Image`, `Image`, optional `Mask` | `Image` |
| `TextOverlay` | 在图片上绘制文字 | `Image` | `Image` |
| `ImageToList` | 将多张图像汇成图像列表 | 多个 `Image` | `ImageList` |
| `Collage` | 根据图像列表生成拼图 | `ImageList` | `Image` |

## 项目结构

```text
.
├── CMakeLists.txt
├── README.md
├── docs/
│   └── analysis.md              # 分析文档
├── samples/
│   └── input.png                # 示例输入图片
├── workflows/
│   ├── demo.json                # 基础示例工作流
│   └── branch_blend.json        # 分支与汇合示例工作流
├── output/                      # 示例输出目录
├── src/
│   ├── core/                    # 节点抽象、端口、图结构、执行器
│   ├── gui/                     # 主窗口、画布、参数面板
│   ├── imageops/                # QImage 图像算法
│   ├── io/                      # JSON 保存和加载
│   ├── nodes/                   # 具体节点实现与注册
│   └── main.cpp                 # GUI/CLI 程序入口
├── tests/
│   └── core_tests.cpp           # QtTest 单元测试
└── translations/
    └── ImageNodeEditor_zh_CN.ts # 简体中文翻译源文件
```

## 环境要求

- Qt 6，已验证环境为 `Qt 6.11.1 mingw_64`
- CMake 3.20 或更高版本
- Qt 安装包自带的 MinGW 编译器

重要：建议使用 Qt 套件自带的 MinGW，例如 `C:\Qt\Tools\mingw1310_64\bin\g++.exe`。不要混用 MSYS2 或其他 MinGW 编译器。混用时可能出现“能编译成功，但运行时崩溃”的问题。

## 构建方法

在 PowerShell 中进入项目根目录：

```powershell
cd "E:\Reports\11. OQK\Code"
```

配置 CMake：

```powershell
cmake -S . -B build-qt -G "MinGW Makefiles" ^
  -DCMAKE_PREFIX_PATH=C:/Qt/6.11.1/mingw_64 ^
  -DCMAKE_CXX_COMPILER=C:/Qt/Tools/mingw1310_64/bin/g++.exe
```

编译：

```powershell
cmake --build build-qt -j 4
```

编译完成后，可执行文件位于：

```text
build-qt/ImageNodeEditor.exe
```

## 运行图形界面

运行前建议先把 Qt 和 MinGW 的 DLL 路径加入当前 PowerShell 会话的 `PATH`：

```powershell
$env:PATH="C:\Qt\6.11.1\mingw_64\bin;C:\Qt\Tools\mingw1310_64\bin;$env:PATH"
```

启动 GUI：

```powershell
.\build-qt\ImageNodeEditor.exe
```

GUI 基本操作：

- 左侧节点列表中双击节点类型，即可在画布上添加节点。
- 在画布中拖动节点可以调整位置。
- 从输出端口拖到输入端口可以创建连接。
- 选中节点后，右侧参数面板会显示该节点参数。
- 选中节点或连线后按 `Delete` 可以删除。
- 点击工具栏 `Run` 执行整个工作流。
- 点击 `Open` / `Save` 加载或保存 JSON 工作流。
- 运行后选中有 `image` 输出的节点，底部预览区会显示该节点输出。
- 菜单栏 `Language` 可选择 `English` 或 `Simplified Chinese`，选择后重启程序生效。

## 语言与国际化

项目使用 Qt 自带的国际化机制：

- QObject 类中使用 `tr("xxx")` 标记可翻译文本。
- 非 QObject 的核心模块使用 `QCoreApplication::translate()`，并配合 `QT_TRANSLATE_NOOP()` 保证 `lupdate` 能提取文本。
- 翻译源文件位于 `translations/ImageNodeEditor_zh_CN.ts`。
- 使用 `lupdate` 提取源码文本到 `.ts`。
- 使用 Qt Linguist 打开 `.ts` 进行人工翻译。
- 使用 `lrelease` 将 `.ts` 编译为 `.qm`。
- 程序启动时使用 `QTranslator` 加载 `.qm` 到 `QApplication`。
- 构建完成后 `.qm` 会被复制到 `build-qt/translations/ImageNodeEditor_zh_CN.qm`。

程序启动时会按以下顺序选择语言：

1. 命令行 `--lang` 参数，例如 `--lang zh_CN`。
2. GUI 语言菜单保存的设置。
3. 操作系统默认语言。

强制使用中文启动 GUI：

```powershell
.\build-qt\ImageNodeEditor.exe --lang zh_CN
```

强制使用英文启动 GUI：

```powershell
.\build-qt\ImageNodeEditor.exe --lang en_US
```

动态切换语言：

- 在 GUI 菜单栏中打开 `Language`。
- 选择 `English` 或 `Simplified Chinese`。
- 程序会立即移除旧翻译器、安装新翻译器，并刷新菜单、工具栏、节点库、参数面板和画布端口文本，不需要重启。

更新翻译文件：

```powershell
cmake --build build-qt --target update_translations
```

这一步会调用 `lupdate`，把代码中新增的 `tr("xxx")`、`QCoreApplication::translate()`、`QT_TRANSLATE_NOOP()` 文本同步到：

```text
translations/ImageNodeEditor_zh_CN.ts
```

人工翻译：

```powershell
C:\Qt\6.11.1\mingw_64\bin\linguist.exe translations\ImageNodeEditor_zh_CN.ts
```

编译翻译文件：

```powershell
cmake --build build-qt --target ImageNodeEditorTranslations
```

普通构建 `cmake --build build-qt -j 4` 也会自动生成并复制 `.qm` 文件。

## 命令行运行

命令行模式不会启动主窗口，适合批量执行或验证 workflow。

基础示例：

```powershell
$env:PATH="C:\Qt\6.11.1\mingw_64\bin;C:\Qt\Tools\mingw1310_64\bin;$env:PATH"
.\build-qt\ImageNodeEditor.exe --workflow workflows\demo.json --no-gui
```

指定中文输出：

```powershell
.\build-qt\ImageNodeEditor.exe --workflow workflows\demo.json --no-gui --lang zh_CN
```

分支与汇合示例：

```powershell
.\build-qt\ImageNodeEditor.exe --workflow workflows\branch_blend.json --no-gui
```

成功时会输出类似内容：

```text
Executed Input (ImageInput).
Executed Bright Color (BrightnessContrast).
Executed Caption (TextOverlay).
Executed Export (ImageExport).
Workflow executed successfully
```

示例输出文件：

```text
output/result.png
output/branch_blend.png
```

## 工作流 JSON 格式

工作流文件由节点和连线组成。节点保存 `id`、`type`、`title`、`pos` 和 `params`；连线保存起点节点/端口和终点节点/端口。

示例：

```json
{
  "version": 1,
  "nodes": [
    {
      "id": "n1",
      "type": "ImageInput",
      "title": "Input",
      "pos": [80, 120],
      "params": {
        "path": "samples/input.png"
      }
    },
    {
      "id": "n2",
      "type": "BrightnessContrast",
      "title": "Bright Color",
      "pos": [330, 90],
      "params": {
        "brightness": 18,
        "contrast": 1.18
      }
    }
  ],
  "edges": [
    {
      "fromNode": "n1",
      "fromPort": "image",
      "toNode": "n2",
      "toPort": "image"
    }
  ]
}
```

路径建议使用相对路径，例如 `samples/input.png`、`output/result.png`，不要写死本机绝对路径。

## 测试

本项目包含 QtTest 单元测试，覆盖：

- 端口类型不兼容时拒绝连接。
- 会形成环路的连接会被拒绝。
- JSON 中未知节点类型会报错。
- 基础图像算法如亮度调整和阈值遮罩。

运行测试：

```powershell
$env:PATH="C:\Qt\6.11.1\mingw_64\bin;C:\Qt\Tools\mingw1310_64\bin;$env:PATH"
ctest --test-dir build-qt --output-on-failure
```

预期结果：

```text
100% tests passed, 0 tests failed out of 1
```

## 已验证结果

在当前环境中已验证：

- `cmake --build build-qt -j 4` 编译成功。
- `ctest --test-dir build-qt --output-on-failure` 测试通过。
- `workflows/demo.json` 命令行执行成功，生成 `output/result.png`。
- `workflows/branch_blend.json` 命令行执行成功，生成 `output/branch_blend.png`。
- `--lang zh_CN` 命令行执行成功，输出中文日志。

## 设计说明

代码没有把所有逻辑堆在主窗口中，而是分成以下层次：

- `core`：只关心节点、端口、连接、DAG 校验和执行顺序。
- `nodes`：每种节点继承 `Node`，通过 `NodeFactory` 注册和创建。
- `imageops`：只负责具体图像算法，不依赖 GUI。
- `io`：负责 JSON 保存和加载。
- `gui`：负责用户交互和界面显示。

GUI 和命令行共用 `WorkflowGraph`、`WorkflowExecutor`、`WorkflowSerializer` 和节点实现，因此同一份 JSON 工作流可以在两种模式下执行。
