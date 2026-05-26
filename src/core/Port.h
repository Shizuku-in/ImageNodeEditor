#pragma once

#include <QString>

enum class PortDirection {
    Input,
    Output
};

enum class PortDataType {
    Image,
    Mask,
    ImageList
};

struct PortSpec {
    QString name;
    QString displayName;
    PortDirection direction = PortDirection::Input;
    PortDataType dataType = PortDataType::Image;
    bool required = true;
};

inline QString portDataTypeName(PortDataType type)
{
    switch (type) {
    case PortDataType::Image:
        return "Image";
    case PortDataType::Mask:
        return "Mask";
    case PortDataType::ImageList:
        return "ImageList";
    }
    return "Unknown";
}

