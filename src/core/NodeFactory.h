#pragma once

#include "core/Node.h"

#include <QMap>
#include <functional>
#include <memory>

class NodeFactory
{
public:
    using Creator = std::function<std::unique_ptr<Node>(const QString &)>;

    void registerType(const QString &typeName, Creator creator);
    std::unique_ptr<Node> create(const QString &typeName, const QString &id) const;
    QStringList availableTypes() const;
    bool contains(const QString &typeName) const;

private:
    QMap<QString, Creator> m_creators;
};

