#include "core/Node.h"

Node::Node(QString id, QString title)
    : m_id(std::move(id))
    , m_title(std::move(title))
{
}

QString Node::id() const
{
    return m_id;
}

void Node::setId(const QString &id)
{
    m_id = id;
}

QString Node::title() const
{
    return m_title;
}

void Node::setTitle(const QString &title, bool custom)
{
    m_title = title;
    m_hasCustomTitle = custom;
}

bool Node::hasCustomTitle() const
{
    return m_hasCustomTitle;
}

QPointF Node::position() const
{
    return m_position;
}

void Node::setPosition(const QPointF &position)
{
    m_position = position;
}

QVariantMap Node::parameters() const
{
    return m_parameters;
}

void Node::setParameters(const QVariantMap &parameters)
{
    initializeDefaults();
    for (auto it = parameters.begin(); it != parameters.end(); ++it) {
        m_parameters[it.key()] = it.value();
    }
}

QVariant Node::parameter(const QString &name) const
{
    return m_parameters.value(name);
}

void Node::setParameter(const QString &name, const QVariant &value)
{
    m_parameters[name] = value;
}

NodeResult Node::preview(const QHash<QString, DataValue> &inputs)
{
    return process(inputs);
}

std::optional<PortSpec> Node::findPort(PortDirection direction, const QString &name) const
{
    const auto ports = direction == PortDirection::Input ? inputPorts() : outputPorts();
    for (const PortSpec &port : ports) {
        if (port.name == name) {
            return port;
        }
    }
    return std::nullopt;
}

void Node::initializeDefaults()
{
    m_parameters.clear();
    for (const ParameterSpec &spec : parameterSpecs()) {
        m_parameters.insert(spec.name, spec.defaultValue);
    }
}
