#pragma once

#include "core/Port.h"

#include <QImage>
#include <QVector>
#include <variant>

struct DataValue {
    PortDataType type = PortDataType::Image;
    std::variant<QImage, QVector<QImage>> value;

    static DataValue image(const QImage &image)
    {
        return {PortDataType::Image, image};
    }

    static DataValue mask(const QImage &image)
    {
        return {PortDataType::Mask, image};
    }

    static DataValue imageList(const QVector<QImage> &images)
    {
        return {PortDataType::ImageList, images};
    }

    QImage asImage() const
    {
        return std::get<QImage>(value);
    }

    QVector<QImage> asImageList() const
    {
        return std::get<QVector<QImage>>(value);
    }
};

