#include "nodes/ImageNodes.h"

#include "imageops/ImageOps.h"

#include <QCoreApplication>
#include <QColor>
#include <QDir>
#include <QFileInfo>

namespace {

[[maybe_unused]] const char *const kNodeI18nMarkers[] = {
    QT_TRANSLATE_NOOP("Nodes", "Image Input"),
    QT_TRANSLATE_NOOP("Nodes", "Image Export"),
    QT_TRANSLATE_NOOP("Nodes", "Display"),
    QT_TRANSLATE_NOOP("Nodes", "Crop"),
    QT_TRANSLATE_NOOP("Nodes", "Resize"),
    QT_TRANSLATE_NOOP("Nodes", "Rotate / Flip"),
    QT_TRANSLATE_NOOP("Nodes", "Blur"),
    QT_TRANSLATE_NOOP("Nodes", "Sharpen"),
    QT_TRANSLATE_NOOP("Nodes", "Brightness / Contrast"),
    QT_TRANSLATE_NOOP("Nodes", "Grayscale"),
    QT_TRANSLATE_NOOP("Nodes", "Threshold Mask"),
    QT_TRANSLATE_NOOP("Nodes", "Blend"),
    QT_TRANSLATE_NOOP("Nodes", "Text Overlay"),
    QT_TRANSLATE_NOOP("Nodes", "Image To List"),
    QT_TRANSLATE_NOOP("Nodes", "Collage"),
    QT_TRANSLATE_NOOP("Nodes", "Image"),
    QT_TRANSLATE_NOOP("Nodes", "Base"),
    QT_TRANSLATE_NOOP("Nodes", "Overlay"),
    QT_TRANSLATE_NOOP("Nodes", "Mask"),
    QT_TRANSLATE_NOOP("Nodes", "Images"),
    QT_TRANSLATE_NOOP("Nodes", "Image 1"),
    QT_TRANSLATE_NOOP("Nodes", "Image 2"),
    QT_TRANSLATE_NOOP("Nodes", "Image 3"),
    QT_TRANSLATE_NOOP("Nodes", "Image 4"),
    QT_TRANSLATE_NOOP("Nodes", "Path"),
    QT_TRANSLATE_NOOP("Nodes", "Format"),
    QT_TRANSLATE_NOOP("Nodes", "Quality"),
    QT_TRANSLATE_NOOP("Nodes", "X"),
    QT_TRANSLATE_NOOP("Nodes", "Y"),
    QT_TRANSLATE_NOOP("Nodes", "Width"),
    QT_TRANSLATE_NOOP("Nodes", "Height"),
    QT_TRANSLATE_NOOP("Nodes", "Keep Aspect"),
    QT_TRANSLATE_NOOP("Nodes", "Angle"),
    QT_TRANSLATE_NOOP("Nodes", "Flip H"),
    QT_TRANSLATE_NOOP("Nodes", "Flip V"),
    QT_TRANSLATE_NOOP("Nodes", "Radius"),
    QT_TRANSLATE_NOOP("Nodes", "Amount"),
    QT_TRANSLATE_NOOP("Nodes", "Brightness"),
    QT_TRANSLATE_NOOP("Nodes", "Contrast"),
    QT_TRANSLATE_NOOP("Nodes", "Mode"),
    QT_TRANSLATE_NOOP("Nodes", "Threshold"),
    QT_TRANSLATE_NOOP("Nodes", "Invert"),
    QT_TRANSLATE_NOOP("Nodes", "Opacity"),
    QT_TRANSLATE_NOOP("Nodes", "Text"),
    QT_TRANSLATE_NOOP("Nodes", "Font Size"),
    QT_TRANSLATE_NOOP("Nodes", "Color"),
    QT_TRANSLATE_NOOP("Nodes", "Columns"),
    QT_TRANSLATE_NOOP("Nodes", "Gap"),
    QT_TRANSLATE_NOOP("Nodes", "Background"),
    QT_TRANSLATE_NOOP("Nodes", "Image path is empty."),
    QT_TRANSLATE_NOOP("Nodes", "Cannot read image: %1"),
    QT_TRANSLATE_NOOP("Nodes", "Export path is empty."),
    QT_TRANSLATE_NOOP("Nodes", "Cannot create export directory: %1"),
    QT_TRANSLATE_NOOP("Nodes", "Cannot save image: %1"),
    QT_TRANSLATE_NOOP("Nodes", "Resize failed. Width and height must be positive.")
};

QString trNodes(const char *text)
{
    return QCoreApplication::translate("Nodes", text);
}

QVector<PortSpec> imageIn(const QString &name = "image", const QString &display = "Image", bool required = true)
{
    return {{name, trNodes(display.toUtf8().constData()), PortDirection::Input, PortDataType::Image, required}};
}

QVector<PortSpec> imageOut(const QString &name = "image", const QString &display = "Image")
{
    return {{name, trNodes(display.toUtf8().constData()), PortDirection::Output, PortDataType::Image, true}};
}

QImage getImage(const QHash<QString, DataValue> &inputs, const QString &port)
{
    return inputs.value(port).asImage();
}

class ImageInputNode final : public Node
{
public:
    explicit ImageInputNode(const QString &id) : Node(id, trNodes("Image Input")) { initializeDefaults(); }
    QString typeName() const override { return "ImageInput"; }
    QVector<PortSpec> inputPorts() const override { return {}; }
    QVector<PortSpec> outputPorts() const override { return imageOut(); }
    bool supportsPreview() const override { return true; }
    QVector<ParameterSpec> parameterSpecs() const override
    {
        return {{"path", trNodes("Path"), ParameterKind::FilePath, "", 0, 0, 1, {"open", "Images (*.png *.jpg *.jpeg *.bmp *.gif *.tiff);;All files (*)"}}};
    }
    NodeResult process(const QHash<QString, DataValue> &) override
    {
        const QString path = parameter("path").toString();
        if (path.trimmed().isEmpty()) {
            return NodeResult::failure(trNodes("Image path is empty."));
        }
        QImage image(path);
        if (image.isNull()) {
            return NodeResult::failure(trNodes("Cannot read image: %1").arg(path));
        }
        NodeResult result;
        result.outputs.insert("image", DataValue::image(imageops::toArgb(image)));
        return result;
    }
};

class ImageExportNode final : public Node
{
public:
    explicit ImageExportNode(const QString &id) : Node(id, trNodes("Image Export")) { initializeDefaults(); }
    QString typeName() const override { return "ImageExport"; }
    QVector<PortSpec> inputPorts() const override { return imageIn(); }
    QVector<PortSpec> outputPorts() const override { return imageOut(); }
    bool supportsPreview() const override { return true; }
    QVector<ParameterSpec> parameterSpecs() const override
    {
        return {
            {"path", trNodes("Path"), ParameterKind::FilePath, "output/result.png", 0, 0, 1, {"save", "PNG (*.png);;JPEG (*.jpg *.jpeg);;BMP (*.bmp);;All files (*)"}},
            {"format", trNodes("Format"), ParameterKind::Choice, "png", 0, 0, 1, {"png", "jpg", "bmp"}},
            {"quality", trNodes("Quality"), ParameterKind::Int, 95, 1, 100, 1}
        };
    }
    NodeResult process(const QHash<QString, DataValue> &inputs) override
    {
        const QImage image = getImage(inputs, "image");
        const QString path = parameter("path").toString();
        if (path.trimmed().isEmpty()) {
            return NodeResult::failure(trNodes("Export path is empty."));
        }
        QFileInfo info(path);
        if (!info.dir().exists() && !info.dir().mkpath(".")) {
            return NodeResult::failure(trNodes("Cannot create export directory: %1").arg(info.dir().path()));
        }
        const QByteArray format = parameter("format").toString().toLatin1();
        if (!image.save(path, format.constData(), parameter("quality").toInt())) {
            return NodeResult::failure(trNodes("Cannot save image: %1").arg(path));
        }
        NodeResult result;
        result.outputs.insert("image", DataValue::image(image));
        return result;
    }
    NodeResult preview(const QHash<QString, DataValue> &inputs) override
    {
        NodeResult result;
        result.outputs.insert("image", DataValue::image(getImage(inputs, "image")));
        return result;
    }
};

class DisplayNode final : public Node
{
public:
    explicit DisplayNode(const QString &id) : Node(id, trNodes("Display")) { initializeDefaults(); }
    QString typeName() const override { return "Display"; }
    QVector<PortSpec> inputPorts() const override { return imageIn(); }
    QVector<PortSpec> outputPorts() const override { return imageOut(); }
    QVector<ParameterSpec> parameterSpecs() const override { return {}; }
    NodeResult process(const QHash<QString, DataValue> &inputs) override
    {
        NodeResult result;
        result.outputs.insert("image", inputs.value("image"));
        return result;
    }
};

class CropNode final : public Node
{
public:
    explicit CropNode(const QString &id) : Node(id, trNodes("Crop")) { initializeDefaults(); }
    QString typeName() const override { return "Crop"; }
    QVector<PortSpec> inputPorts() const override { return imageIn(); }
    QVector<PortSpec> outputPorts() const override { return imageOut(); }
    QVector<ParameterSpec> parameterSpecs() const override
    {
        return {
            {"x", trNodes("X"), ParameterKind::Int, 0, 0, 10000},
            {"y", trNodes("Y"), ParameterKind::Int, 0, 0, 10000},
            {"width", trNodes("Width"), ParameterKind::Int, 256, 1, 10000},
            {"height", trNodes("Height"), ParameterKind::Int, 256, 1, 10000}
        };
    }
    NodeResult process(const QHash<QString, DataValue> &inputs) override
    {
        QString error;
        QImage out = imageops::crop(getImage(inputs, "image"), parameter("x").toInt(), parameter("y").toInt(),
                                    parameter("width").toInt(), parameter("height").toInt(), &error);
        if (out.isNull()) {
            return NodeResult::failure(error);
        }
        NodeResult result;
        result.outputs.insert("image", DataValue::image(out));
        return result;
    }
};

class ResizeNode final : public Node
{
public:
    explicit ResizeNode(const QString &id) : Node(id, trNodes("Resize")) { initializeDefaults(); }
    QString typeName() const override { return "Resize"; }
    QVector<PortSpec> inputPorts() const override { return imageIn(); }
    QVector<PortSpec> outputPorts() const override { return imageOut(); }
    QVector<ParameterSpec> parameterSpecs() const override
    {
        return {
            {"width", trNodes("Width"), ParameterKind::Int, 512, 1, 10000},
            {"height", trNodes("Height"), ParameterKind::Int, 512, 1, 10000},
            {"keepAspect", trNodes("Keep Aspect"), ParameterKind::Bool, true}
        };
    }
    NodeResult process(const QHash<QString, DataValue> &inputs) override
    {
        QImage out = imageops::resize(getImage(inputs, "image"), parameter("width").toInt(),
                                      parameter("height").toInt(), parameter("keepAspect").toBool());
        if (out.isNull()) {
            return NodeResult::failure(trNodes("Resize failed. Width and height must be positive."));
        }
        NodeResult result;
        result.outputs.insert("image", DataValue::image(out));
        return result;
    }
};

class RotateFlipNode final : public Node
{
public:
    explicit RotateFlipNode(const QString &id) : Node(id, trNodes("Rotate / Flip")) { initializeDefaults(); }
    QString typeName() const override { return "RotateFlip"; }
    QVector<PortSpec> inputPorts() const override { return imageIn(); }
    QVector<PortSpec> outputPorts() const override { return imageOut(); }
    QVector<ParameterSpec> parameterSpecs() const override
    {
        return {
            {"angle", trNodes("Angle"), ParameterKind::Int, 0, -360, 360},
            {"flipH", trNodes("Flip H"), ParameterKind::Bool, false},
            {"flipV", trNodes("Flip V"), ParameterKind::Bool, false}
        };
    }
    NodeResult process(const QHash<QString, DataValue> &inputs) override
    {
        NodeResult result;
        result.outputs.insert("image", DataValue::image(imageops::rotateFlip(getImage(inputs, "image"),
            parameter("angle").toInt(), parameter("flipH").toBool(), parameter("flipV").toBool())));
        return result;
    }
};

class BlurNode final : public Node
{
public:
    explicit BlurNode(const QString &id) : Node(id, trNodes("Blur")) { initializeDefaults(); }
    QString typeName() const override { return "Blur"; }
    QVector<PortSpec> inputPorts() const override { return imageIn(); }
    QVector<PortSpec> outputPorts() const override { return imageOut(); }
    QVector<ParameterSpec> parameterSpecs() const override
    {
        return {{"radius", trNodes("Radius"), ParameterKind::Int, 2, 0, 20}};
    }
    NodeResult process(const QHash<QString, DataValue> &inputs) override
    {
        NodeResult result;
        result.outputs.insert("image", DataValue::image(imageops::blur(getImage(inputs, "image"), parameter("radius").toInt())));
        return result;
    }
};

class SharpenNode final : public Node
{
public:
    explicit SharpenNode(const QString &id) : Node(id, trNodes("Sharpen")) { initializeDefaults(); }
    QString typeName() const override { return "Sharpen"; }
    QVector<PortSpec> inputPorts() const override { return imageIn(); }
    QVector<PortSpec> outputPorts() const override { return imageOut(); }
    QVector<ParameterSpec> parameterSpecs() const override
    {
        return {{"amount", trNodes("Amount"), ParameterKind::Double, 0.6, 0.0, 3.0, 0.1}};
    }
    NodeResult process(const QHash<QString, DataValue> &inputs) override
    {
        NodeResult result;
        result.outputs.insert("image", DataValue::image(imageops::sharpen(getImage(inputs, "image"), parameter("amount").toDouble())));
        return result;
    }
};

class BrightnessContrastNode final : public Node
{
public:
    explicit BrightnessContrastNode(const QString &id) : Node(id, trNodes("Brightness / Contrast")) { initializeDefaults(); }
    QString typeName() const override { return "BrightnessContrast"; }
    QVector<PortSpec> inputPorts() const override { return imageIn(); }
    QVector<PortSpec> outputPorts() const override { return imageOut(); }
    QVector<ParameterSpec> parameterSpecs() const override
    {
        return {
            {"brightness", trNodes("Brightness"), ParameterKind::Int, 0, -255, 255},
            {"contrast", trNodes("Contrast"), ParameterKind::Double, 1.0, 0.0, 4.0, 0.1}
        };
    }
    NodeResult process(const QHash<QString, DataValue> &inputs) override
    {
        NodeResult result;
        result.outputs.insert("image", DataValue::image(imageops::brightnessContrast(getImage(inputs, "image"),
            parameter("brightness").toInt(), parameter("contrast").toDouble())));
        return result;
    }
};

class GrayscaleNode final : public Node
{
public:
    explicit GrayscaleNode(const QString &id) : Node(id, trNodes("Grayscale")) { initializeDefaults(); }
    QString typeName() const override { return "Grayscale"; }
    QVector<PortSpec> inputPorts() const override { return imageIn(); }
    QVector<PortSpec> outputPorts() const override { return imageOut(); }
    QVector<ParameterSpec> parameterSpecs() const override
    {
        return {{"mode", trNodes("Mode"), ParameterKind::Choice, "Luminance", 0, 0, 1, {"Luminance", "Average", "Red", "Green", "Blue"}}};
    }
    NodeResult process(const QHash<QString, DataValue> &inputs) override
    {
        NodeResult result;
        result.outputs.insert("image", DataValue::image(imageops::grayscale(getImage(inputs, "image"), parameter("mode").toString())));
        return result;
    }
};

class ThresholdMaskNode final : public Node
{
public:
    explicit ThresholdMaskNode(const QString &id) : Node(id, trNodes("Threshold Mask")) { initializeDefaults(); }
    QString typeName() const override { return "ThresholdMask"; }
    QVector<PortSpec> inputPorts() const override { return imageIn(); }
    QVector<PortSpec> outputPorts() const override { return {{"mask", trNodes("Mask"), PortDirection::Output, PortDataType::Mask, true}}; }
    QVector<ParameterSpec> parameterSpecs() const override
    {
        return {
            {"threshold", trNodes("Threshold"), ParameterKind::Int, 128, 0, 255},
            {"invert", trNodes("Invert"), ParameterKind::Bool, false}
        };
    }
    NodeResult process(const QHash<QString, DataValue> &inputs) override
    {
        NodeResult result;
        result.outputs.insert("mask", DataValue::mask(imageops::thresholdMask(getImage(inputs, "image"),
            parameter("threshold").toInt(), parameter("invert").toBool())));
        return result;
    }
};

class BlendNode final : public Node
{
public:
    explicit BlendNode(const QString &id) : Node(id, trNodes("Blend")) { initializeDefaults(); }
    QString typeName() const override { return "Blend"; }
    QVector<PortSpec> inputPorts() const override
    {
        return {
            {"base", trNodes("Base"), PortDirection::Input, PortDataType::Image, true},
            {"overlay", trNodes("Overlay"), PortDirection::Input, PortDataType::Image, true},
            {"mask", trNodes("Mask"), PortDirection::Input, PortDataType::Mask, false}
        };
    }
    QVector<PortSpec> outputPorts() const override { return imageOut(); }
    QVector<ParameterSpec> parameterSpecs() const override
    {
        return {
            {"opacity", trNodes("Opacity"), ParameterKind::Double, 0.5, 0.0, 1.0, 0.05},
            {"mode", trNodes("Mode"), ParameterKind::Choice, "Normal", 0, 0, 1, {"Normal", "Multiply", "Screen", "SoftLight"}}
        };
    }
    NodeResult process(const QHash<QString, DataValue> &inputs) override
    {
        QImage mask;
        const QImage *maskPtr = nullptr;
        if (inputs.contains("mask")) {
            mask = inputs.value("mask").asImage();
            maskPtr = &mask;
        }
        NodeResult result;
        result.outputs.insert("image", DataValue::image(imageops::blend(getImage(inputs, "base"),
            getImage(inputs, "overlay"), parameter("opacity").toDouble(), parameter("mode").toString(), maskPtr)));
        return result;
    }
};

class TextOverlayNode final : public Node
{
public:
    explicit TextOverlayNode(const QString &id) : Node(id, trNodes("Text Overlay")) { initializeDefaults(); }
    QString typeName() const override { return "TextOverlay"; }
    QVector<PortSpec> inputPorts() const override { return imageIn(); }
    QVector<PortSpec> outputPorts() const override { return imageOut(); }
    QVector<ParameterSpec> parameterSpecs() const override
    {
        return {
            {"text", trNodes("Text"), ParameterKind::String, "Hello ImageNodeEditor"},
            {"fontSize", trNodes("Font Size"), ParameterKind::Int, 32, 1, 300},
            {"color", trNodes("Color"), ParameterKind::Color, "#ffffff"},
            {"x", trNodes("X"), ParameterKind::Int, 30, 0, 10000},
            {"y", trNodes("Y"), ParameterKind::Int, 60, 0, 10000}
        };
    }
    NodeResult process(const QHash<QString, DataValue> &inputs) override
    {
        QColor color(parameter("color").toString());
        if (!color.isValid()) {
            color = Qt::white;
        }
        NodeResult result;
        result.outputs.insert("image", DataValue::image(imageops::textOverlay(getImage(inputs, "image"),
            parameter("text").toString(), parameter("fontSize").toInt(), color,
            parameter("x").toInt(), parameter("y").toInt())));
        return result;
    }
};

class ImageToListNode final : public Node
{
public:
    explicit ImageToListNode(const QString &id) : Node(id, trNodes("Image To List")) { initializeDefaults(); }
    QString typeName() const override { return "ImageToList"; }
    QVector<PortSpec> inputPorts() const override
    {
        return {
            {"image1", trNodes("Image 1"), PortDirection::Input, PortDataType::Image, true},
            {"image2", trNodes("Image 2"), PortDirection::Input, PortDataType::Image, false},
            {"image3", trNodes("Image 3"), PortDirection::Input, PortDataType::Image, false},
            {"image4", trNodes("Image 4"), PortDirection::Input, PortDataType::Image, false}
        };
    }
    QVector<PortSpec> outputPorts() const override
    {
        return {{"images", trNodes("Images"), PortDirection::Output, PortDataType::ImageList, true}};
    }
    QVector<ParameterSpec> parameterSpecs() const override { return {}; }
    NodeResult process(const QHash<QString, DataValue> &inputs) override
    {
        QVector<QImage> images;
        for (const QString name : {"image1", "image2", "image3", "image4"}) {
            if (inputs.contains(name)) {
                images.append(inputs.value(name).asImage());
            }
        }
        NodeResult result;
        result.outputs.insert("images", DataValue::imageList(images));
        return result;
    }
};

class CollageNode final : public Node
{
public:
    explicit CollageNode(const QString &id) : Node(id, trNodes("Collage")) { initializeDefaults(); }
    QString typeName() const override { return "Collage"; }
    QVector<PortSpec> inputPorts() const override
    {
        return {{"images", trNodes("Images"), PortDirection::Input, PortDataType::ImageList, true}};
    }
    QVector<PortSpec> outputPorts() const override { return imageOut(); }
    QVector<ParameterSpec> parameterSpecs() const override
    {
        return {
            {"columns", trNodes("Columns"), ParameterKind::Int, 2, 1, 10},
            {"gap", trNodes("Gap"), ParameterKind::Int, 12, 0, 200},
            {"background", trNodes("Background"), ParameterKind::Color, "#202020"}
        };
    }
    NodeResult process(const QHash<QString, DataValue> &inputs) override
    {
        QString error;
        QColor bg(parameter("background").toString());
        if (!bg.isValid()) {
            bg = QColor("#202020");
        }
        QImage out = imageops::collage(inputs.value("images").asImageList(), parameter("columns").toInt(),
                                       parameter("gap").toInt(), bg, &error);
        if (out.isNull()) {
            return NodeResult::failure(error);
        }
        NodeResult result;
        result.outputs.insert("image", DataValue::image(out));
        return result;
    }
};

template <typename T>
void registerNode(NodeFactory &factory, const QString &name)
{
    factory.registerType(name, [](const QString &id) { return std::make_unique<T>(id); });
}

} // namespace

void registerImageNodes(NodeFactory &factory)
{
    registerNode<ImageInputNode>(factory, "ImageInput");
    registerNode<ImageExportNode>(factory, "ImageExport");
    registerNode<DisplayNode>(factory, "Display");
    registerNode<CropNode>(factory, "Crop");
    registerNode<ResizeNode>(factory, "Resize");
    registerNode<RotateFlipNode>(factory, "RotateFlip");
    registerNode<BlurNode>(factory, "Blur");
    registerNode<SharpenNode>(factory, "Sharpen");
    registerNode<BrightnessContrastNode>(factory, "BrightnessContrast");
    registerNode<GrayscaleNode>(factory, "Grayscale");
    registerNode<ThresholdMaskNode>(factory, "ThresholdMask");
    registerNode<BlendNode>(factory, "Blend");
    registerNode<TextOverlayNode>(factory, "TextOverlay");
    registerNode<ImageToListNode>(factory, "ImageToList");
    registerNode<CollageNode>(factory, "Collage");
}
