#include "core/NodeFactory.h"

void NodeFactory::registerType(const QString &typeName, Creator creator)
{
    m_creators.insert(typeName, std::move(creator));
}

std::unique_ptr<Node> NodeFactory::create(const QString &typeName, const QString &id) const
{
    const auto it = m_creators.find(typeName);
    if (it == m_creators.end()) {
        return nullptr;
    }
    return it.value()(id);
}

QStringList NodeFactory::availableTypes() const
{
    return m_creators.keys();
}

bool NodeFactory::contains(const QString &typeName) const
{
    return m_creators.contains(typeName);
}

