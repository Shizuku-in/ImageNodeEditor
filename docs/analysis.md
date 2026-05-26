# ImageNodeEditor 分析文档

## 题目理解与设计目标

本项目实现一个基于 Qt 的节点式图像处理工具。用户通过可视化节点搭建有向无环图工作流，节点之间通过类型安全的端口传递图像数据，最终可在 GUI 中预览，也可通过命令行批量执行。

设计目标是保持课程项目可读、可编译、可解释：图形界面、节点逻辑、图像算法、工作流读写分离；节点通过继承和工厂注册扩展；GUI 与 CLI 共用同一套图执行逻辑。

## 节点与端口设计

端口数据类型包括：

| 类型 | 含义 | 表示 |
|---|---|---|
| Image | 彩色或灰度图，统一转为 ARGB32 | QImage |
| Mask | 单通道遮罩 | QImage::Format_Grayscale8 |
| ImageList | 多张图像 | QVector<QImage> |

兼容规则采用严格匹配：Image 只能连 Image，Mask 只能连 Mask，ImageList 只能连 ImageList。需要转换时使用专门节点。

实现节点包括：ImageInput、ImageExport、Display、Crop、Resize、RotateFlip、Blur、Sharpen、BrightnessContrast、Grayscale、ThresholdMask、Blend、TextOverlay、ImageToList、Collage。它们覆盖读图、导出、显示、变换、滤波、色彩、蒙版混合、文字叠加和拼图。

## GUI 设计

界面采用四区布局：左侧 Node Library，中间 QGraphicsView 画布，右侧参数面板，底部日志与预览。用户可双击添加节点，拖动节点移动，拖拽端口创建连线，按 Delete 删除选中节点或连线。连接时立即检查端口方向、类型和环路。

运行工作流后，底部日志显示每个节点的执行状态，选中节点如果存在 image 输出，则在预览区显示该节点输出图像。

## JSON 工作流格式

工作流 JSON 保存 version、nodes 与 edges。节点保存 id、type、title、pos 和 params；边保存 fromNode/fromPort/toNode/toPort。GUI 和命令行使用同一格式。

命令行模式：

```bash
ImageNodeEditor --workflow workflows/demo.json --no-gui
```

## 架构设计

核心模块分为：

| 模块 | 职责 |
|---|---|
| core | Node、PortSpec、WorkflowGraph、WorkflowExecutor |
| nodes | 具体节点类，通过 NodeFactory 注册 |
| imageops | QImage 算法函数 |
| io | WorkflowSerializer JSON 读写 |
| gui | MainWindow、GraphScene、ParameterPanel |

执行流程：Serializer 加载 JSON，Factory 创建节点，Graph 校验连接和 DAG，Executor 拓扑排序后依次调用每个节点的 process()，输出缓存按 nodeId/portName 保存并传递给下游节点。

## 异常处理策略

文件缺失、图片解码失败、导出失败、参数非法、端口类型不匹配、必需输入缺失、环路等都返回清晰错误。GUI 使用 QMessageBox 与日志面板提示；CLI 输出错误并返回非零退出码。

## AI 工具使用说明

在方案阶段使用 AI 讨论了节点类型、端口类型、JSON 格式、DAG 校验和异常处理策略。采纳的主要建议包括：使用 QImage-only 降低依赖风险；使用 NodeFactory 注册节点避免按类型写超长 if-else；GUI 与 CLI 共享 WorkflowExecutor；增加 Mask 和 ImageList 让分支汇合、拼图等工作流更有表现力。

