#include "imageops/ImageOps.h"

#include <QPainter>
#include <QtGlobal>
#include <QtMath>
#include <algorithm>

namespace {

int clampChannel(int value)
{
    return std::clamp(value, 0, 255);
}

QColor pixelAtClamped(const QImage &image, int x, int y)
{
    x = std::clamp(x, 0, image.width() - 1);
    y = std::clamp(y, 0, image.height() - 1);
    return QColor::fromRgba(image.pixel(x, y));
}

int luminance(const QColor &color)
{
    return clampChannel(qRound(0.299 * color.red() + 0.587 * color.green() + 0.114 * color.blue()));
}

int softLight(int base, int blend)
{
    const double b = base / 255.0;
    const double s = blend / 255.0;
    const double out = s < 0.5 ? b - (1.0 - 2.0 * s) * b * (1.0 - b)
                               : b + (2.0 * s - 1.0) * (std::sqrt(b) - b);
    return clampChannel(qRound(out * 255.0));
}

} // namespace

namespace imageops {

QImage toArgb(const QImage &image)
{
    return image.convertToFormat(QImage::Format_ARGB32);
}

QImage crop(const QImage &image, int x, int y, int width, int height, QString *error)
{
    if (image.isNull()) {
        if (error) *error = "Input image is empty.";
        return {};
    }
    if (width <= 0 || height <= 0) {
        if (error) *error = "Crop width and height must be positive.";
        return {};
    }
    const QRect sourceRect = QRect(0, 0, image.width(), image.height()).intersected(QRect(x, y, width, height));
    if (sourceRect.isEmpty()) {
        if (error) *error = "Crop rectangle is outside the image.";
        return {};
    }
    return image.copy(sourceRect).convertToFormat(QImage::Format_ARGB32);
}

QImage resize(const QImage &image, int width, int height, bool keepAspect)
{
    if (width <= 0 || height <= 0 || image.isNull()) {
        return {};
    }
    const Qt::AspectRatioMode mode = keepAspect ? Qt::KeepAspectRatio : Qt::IgnoreAspectRatio;
    return image.scaled(width, height, mode, Qt::SmoothTransformation).convertToFormat(QImage::Format_ARGB32);
}

QImage rotateFlip(const QImage &image, int angle, bool flipH, bool flipV)
{
    QImage result = image.convertToFormat(QImage::Format_ARGB32);
    if (flipH || flipV) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
        Qt::Orientations orientations;
        if (flipH) orientations |= Qt::Horizontal;
        if (flipV) orientations |= Qt::Vertical;
        result = result.flipped(orientations);
#else
        result = result.mirrored(flipH, flipV);
#endif
    }
    if (angle != 0) {
        QTransform transform;
        transform.rotate(angle);
        result = result.transformed(transform, Qt::SmoothTransformation);
    }
    return result.convertToFormat(QImage::Format_ARGB32);
}

QImage blur(const QImage &image, int radius)
{
    const QImage source = image.convertToFormat(QImage::Format_ARGB32);
    if (radius <= 0 || source.isNull()) {
        return source;
    }
    QImage result(source.size(), QImage::Format_ARGB32);
    const int kernel = radius * 2 + 1;
    const int area = kernel * kernel;
    for (int y = 0; y < source.height(); ++y) {
        QRgb *out = reinterpret_cast<QRgb *>(result.scanLine(y));
        for (int x = 0; x < source.width(); ++x) {
            int r = 0, g = 0, b = 0, a = 0;
            for (int ky = -radius; ky <= radius; ++ky) {
                for (int kx = -radius; kx <= radius; ++kx) {
                    const QColor c = pixelAtClamped(source, x + kx, y + ky);
                    r += c.red();
                    g += c.green();
                    b += c.blue();
                    a += c.alpha();
                }
            }
            out[x] = qRgba(r / area, g / area, b / area, a / area);
        }
    }
    return result;
}

QImage sharpen(const QImage &image, double amount)
{
    const QImage source = image.convertToFormat(QImage::Format_ARGB32);
    if (amount <= 0.0 || source.isNull()) {
        return source;
    }
    QImage result(source.size(), QImage::Format_ARGB32);
    for (int y = 0; y < source.height(); ++y) {
        QRgb *out = reinterpret_cast<QRgb *>(result.scanLine(y));
        for (int x = 0; x < source.width(); ++x) {
            const QColor center = pixelAtClamped(source, x, y);
            const QColor left = pixelAtClamped(source, x - 1, y);
            const QColor right = pixelAtClamped(source, x + 1, y);
            const QColor up = pixelAtClamped(source, x, y - 1);
            const QColor down = pixelAtClamped(source, x, y + 1);
            const int r = clampChannel(qRound(center.red() * (1.0 + 4.0 * amount)
                - amount * (left.red() + right.red() + up.red() + down.red())));
            const int g = clampChannel(qRound(center.green() * (1.0 + 4.0 * amount)
                - amount * (left.green() + right.green() + up.green() + down.green())));
            const int b = clampChannel(qRound(center.blue() * (1.0 + 4.0 * amount)
                - amount * (left.blue() + right.blue() + up.blue() + down.blue())));
            out[x] = qRgba(r, g, b, center.alpha());
        }
    }
    return result;
}

QImage brightnessContrast(const QImage &image, int brightness, double contrast)
{
    const QImage source = image.convertToFormat(QImage::Format_ARGB32);
    QImage result(source.size(), QImage::Format_ARGB32);
    for (int y = 0; y < source.height(); ++y) {
        const QRgb *in = reinterpret_cast<const QRgb *>(source.constScanLine(y));
        QRgb *out = reinterpret_cast<QRgb *>(result.scanLine(y));
        for (int x = 0; x < source.width(); ++x) {
            const QColor c = QColor::fromRgba(in[x]);
            const int r = clampChannel(qRound((c.red() - 128) * contrast + 128 + brightness));
            const int g = clampChannel(qRound((c.green() - 128) * contrast + 128 + brightness));
            const int b = clampChannel(qRound((c.blue() - 128) * contrast + 128 + brightness));
            out[x] = qRgba(r, g, b, c.alpha());
        }
    }
    return result;
}

QImage grayscale(const QImage &image, const QString &mode)
{
    const QImage source = image.convertToFormat(QImage::Format_ARGB32);
    QImage result(source.size(), QImage::Format_ARGB32);
    for (int y = 0; y < source.height(); ++y) {
        const QRgb *in = reinterpret_cast<const QRgb *>(source.constScanLine(y));
        QRgb *out = reinterpret_cast<QRgb *>(result.scanLine(y));
        for (int x = 0; x < source.width(); ++x) {
            const QColor c = QColor::fromRgba(in[x]);
            int gray = luminance(c);
            if (mode == "Average") {
                gray = (c.red() + c.green() + c.blue()) / 3;
            } else if (mode == "Red") {
                gray = c.red();
            } else if (mode == "Green") {
                gray = c.green();
            } else if (mode == "Blue") {
                gray = c.blue();
            }
            out[x] = qRgba(gray, gray, gray, c.alpha());
        }
    }
    return result;
}

QImage thresholdMask(const QImage &image, int threshold, bool invert)
{
    const QImage source = image.convertToFormat(QImage::Format_ARGB32);
    QImage result(source.size(), QImage::Format_Grayscale8);
    for (int y = 0; y < source.height(); ++y) {
        uchar *out = result.scanLine(y);
        for (int x = 0; x < source.width(); ++x) {
            const int value = luminance(QColor::fromRgba(source.pixel(x, y))) >= threshold ? 255 : 0;
            out[x] = static_cast<uchar>(invert ? 255 - value : value);
        }
    }
    return result;
}

QImage blend(const QImage &base, const QImage &overlay, double opacity, const QString &mode, const QImage *mask)
{
    const QImage b = base.convertToFormat(QImage::Format_ARGB32);
    const QImage o = overlay.scaled(b.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation)
        .convertToFormat(QImage::Format_ARGB32);
    QImage m;
    if (mask && !mask->isNull()) {
        m = mask->scaled(b.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation)
            .convertToFormat(QImage::Format_Grayscale8);
    }

    QImage result(b.size(), QImage::Format_ARGB32);
    opacity = std::clamp(opacity, 0.0, 1.0);
    for (int y = 0; y < b.height(); ++y) {
        QRgb *out = reinterpret_cast<QRgb *>(result.scanLine(y));
        for (int x = 0; x < b.width(); ++x) {
            const QColor bc = QColor::fromRgba(b.pixel(x, y));
            const QColor oc = QColor::fromRgba(o.pixel(x, y));
            double localOpacity = opacity;
            if (!m.isNull()) {
                localOpacity *= qGray(m.pixel(x, y)) / 255.0;
            }
            auto channel = [&](int baseChannel, int overlayChannel) {
                int blended = overlayChannel;
                if (mode == "Multiply") {
                    blended = baseChannel * overlayChannel / 255;
                } else if (mode == "Screen") {
                    blended = 255 - (255 - baseChannel) * (255 - overlayChannel) / 255;
                } else if (mode == "SoftLight") {
                    blended = softLight(baseChannel, overlayChannel);
                }
                return clampChannel(qRound(baseChannel * (1.0 - localOpacity) + blended * localOpacity));
            };
            out[x] = qRgba(channel(bc.red(), oc.red()), channel(bc.green(), oc.green()),
                           channel(bc.blue(), oc.blue()), bc.alpha());
        }
    }
    return result;
}

QImage textOverlay(const QImage &image, const QString &text, int fontSize, const QColor &color, int x, int y)
{
    QImage result = image.convertToFormat(QImage::Format_ARGB32);
    QPainter painter(&result);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(color);
    QFont font;
    font.setPointSize(std::max(1, fontSize));
    painter.setFont(font);
    painter.drawText(QPoint(x, y), text);
    return result;
}

QImage collage(const QVector<QImage> &images, int columns, int gap, const QColor &background, QString *error)
{
    if (images.isEmpty()) {
        if (error) *error = "Collage needs at least one image.";
        return {};
    }
    columns = std::max(1, columns);
    gap = std::max(0, gap);
    QSize cell(1, 1);
    for (const QImage &image : images) {
        cell.setWidth(std::max(cell.width(), image.width()));
        cell.setHeight(std::max(cell.height(), image.height()));
    }
    const int rows = qCeil(images.size() / static_cast<double>(columns));
    QImage result(columns * cell.width() + (columns + 1) * gap,
                  rows * cell.height() + (rows + 1) * gap,
                  QImage::Format_ARGB32);
    result.fill(background);
    QPainter painter(&result);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    for (int i = 0; i < images.size(); ++i) {
        const int col = i % columns;
        const int row = i / columns;
        QRect target(gap + col * (cell.width() + gap),
                     gap + row * (cell.height() + gap),
                     cell.width(),
                     cell.height());
        painter.drawImage(target, images[i].convertToFormat(QImage::Format_ARGB32));
    }
    return result;
}

} // namespace imageops
