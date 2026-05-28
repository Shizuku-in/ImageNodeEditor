#pragma once

#include <QColor>
#include <QImage>
#include <QString>
#include <QVector>

namespace imageops {

QImage toArgb(const QImage &image);
QImage crop(const QImage &image, int x, int y, int width, int height, QString *error = nullptr);
QImage resize(const QImage &image, int width, int height, bool keepAspect);
QImage rotateFlip(const QImage &image, int angle, bool flipH, bool flipV);
QImage blur(const QImage &image, int radius);
QImage sharpen(const QImage &image, double amount);
QImage brightnessContrast(const QImage &image, int brightness, double contrast);
QImage grayscale(const QImage &image, const QString &mode);
QImage thresholdMask(const QImage &image, int threshold, bool invert);
QImage blend(const QImage &base, const QImage &overlay, double opacity, const QString &mode, const QImage *mask);
QImage textOverlay(const QImage &image, const QString &text, int fontSize, const QColor &color, int x, int y);
QImage collage(const QVector<QImage> &images, int columns, int gap, const QColor &background, QString *error = nullptr);
QImage collage(const QVector<QImage> &images, int rows, int columns, int gap, const QColor &background, QString *error = nullptr);

}
