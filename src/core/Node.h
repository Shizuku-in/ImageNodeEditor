#pragma once

#include "core/DataValue.h"
#include "core/Port.h"

#include <QJsonObject>
#include <QPointF>
#include <QVariant>
#include <QVector>
#include <optional>
#include <utility>

enum class ParameterKind {
    Int,
    Double,
    Bool,
    String,
    Color,
    Choice,
    FilePath
};

struct ParameterSpec {
    ParameterSpec() = default;
    ParameterSpec(QString name,
                  QString displayName,
                  ParameterKind kind,
                  QVariant defaultValue,
                  double minValue = 0.0,
                  double maxValue = 100.0,
                  double step = 1.0,
                  QStringList choices = {})
        : name(std::move(name))
        , displayName(std::move(displayName))
        , kind(kind)
        , defaultValue(std::move(defaultValue))
        , minValue(minValue)
        , maxValue(maxValue)
        , step(step)
        , choices(std::move(choices))
    {
    }

    QString name;
    QString displayName;
    ParameterKind kind = ParameterKind::String;
    QVariant defaultValue;
    double minValue = 0.0;
    double maxValue = 100.0;
    double step = 1.0;
    QStringList choices;
};

struct NodeResult {
    bool ok = true;
    QString error;
    QHash<QString, DataValue> outputs;

    static NodeResult failure(const QString &message)
    {
        NodeResult result;
        result.ok = false;
        result.error = message;
        return result;
    }
};

class Node
{
public:
    Node(QString id, QString title);
    virtual ~Node() = default;

    QString id() const;
    void setId(const QString &id);

    QString title() const;
    void setTitle(const QString &title, bool custom = true);
    bool hasCustomTitle() const;

    QPointF position() const;
    void setPosition(const QPointF &position);

    virtual QString typeName() const = 0;
    virtual QVector<PortSpec> inputPorts() const = 0;
    virtual QVector<PortSpec> outputPorts() const = 0;
    virtual QVector<ParameterSpec> parameterSpecs() const = 0;
    virtual NodeResult process(const QHash<QString, DataValue> &inputs) = 0;
    virtual NodeResult preview(const QHash<QString, DataValue> &inputs);
    virtual bool supportsPreview() const { return false; }

    QVariantMap parameters() const;
    void setParameters(const QVariantMap &parameters);
    QVariant parameter(const QString &name) const;
    void setParameter(const QString &name, const QVariant &value);

    std::optional<PortSpec> findPort(PortDirection direction, const QString &name) const;

protected:
    void initializeDefaults();

private:
    QString m_id;
    QString m_title;
    bool m_hasCustomTitle = false;
    QPointF m_position;
    QVariantMap m_parameters;
};
