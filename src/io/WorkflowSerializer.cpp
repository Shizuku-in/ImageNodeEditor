#include "io/WorkflowSerializer.h"

#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>

namespace {

[[maybe_unused]] const char *const kWorkflowSerializerI18nMarkers[] = {
    QT_TRANSLATE_NOOP("WorkflowSerializer", "Cannot open workflow: %1"),
    QT_TRANSLATE_NOOP("WorkflowSerializer", "Invalid JSON workflow: %1"),
    QT_TRANSLATE_NOOP("WorkflowSerializer", "Cannot write workflow: %1"),
    QT_TRANSLATE_NOOP("WorkflowSerializer", "Graph pointer is null."),
    QT_TRANSLATE_NOOP("WorkflowSerializer", "Unknown node type: %1")
};

QString trSerializer(const char *text)
{
    return QCoreApplication::translate("WorkflowSerializer", text);
}

QVariant jsonToVariant(const QJsonValue &value)
{
    if (value.isBool()) return value.toBool();
    if (value.isDouble()) return value.toDouble();
    if (value.isString()) return value.toString();
    if (value.isArray()) return value.toArray().toVariantList();
    if (value.isObject()) return value.toObject().toVariantMap();
    return {};
}

QJsonValue variantToJson(const QVariant &value)
{
    return QJsonValue::fromVariant(value);
}

} // namespace

bool WorkflowSerializer::loadFromFile(const QString &path, const NodeFactory &factory, WorkflowGraph *graph, QString *error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error) *error = trSerializer("Cannot open workflow: %1").arg(path);
        return false;
    }
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (error) *error = trSerializer("Invalid JSON workflow: %1").arg(parseError.errorString());
        return false;
    }
    return fromJson(document.object(), factory, graph, error);
}

bool WorkflowSerializer::saveToFile(const QString &path, const WorkflowGraph &graph, QString *error)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (error) *error = trSerializer("Cannot write workflow: %1").arg(path);
        return false;
    }
    file.write(QJsonDocument(toJson(graph)).toJson(QJsonDocument::Indented));
    return true;
}

QJsonObject WorkflowSerializer::toJson(const WorkflowGraph &graph)
{
    QJsonObject root;
    root["version"] = 1;

    QJsonArray nodes;
    for (Node *node : graph.nodes()) {
        QJsonObject item;
        item["id"] = node->id();
        item["type"] = node->typeName();
        item["title"] = node->title();
        QJsonArray pos;
        pos.append(node->position().x());
        pos.append(node->position().y());
        item["pos"] = pos;

        QJsonObject params;
        const QVariantMap parameterMap = node->parameters();
        for (auto it = parameterMap.begin(); it != parameterMap.end(); ++it) {
            params[it.key()] = variantToJson(it.value());
        }
        item["params"] = params;
        nodes.append(item);
    }
    root["nodes"] = nodes;

    QJsonArray edges;
    for (const Edge &edge : graph.edges()) {
        QJsonObject item;
        item["fromNode"] = edge.fromNode;
        item["fromPort"] = edge.fromPort;
        item["toNode"] = edge.toNode;
        item["toPort"] = edge.toPort;
        edges.append(item);
    }
    root["edges"] = edges;
    return root;
}

bool WorkflowSerializer::fromJson(const QJsonObject &object, const NodeFactory &factory, WorkflowGraph *graph, QString *error)
{
    if (!graph) {
        if (error) *error = trSerializer("Graph pointer is null.");
        return false;
    }
    WorkflowGraph loaded;
    const QJsonArray nodes = object.value("nodes").toArray();
    for (const QJsonValue &value : nodes) {
        const QJsonObject item = value.toObject();
        const QString id = item.value("id").toString();
        const QString type = item.value("type").toString();
        auto node = factory.create(type, id);
        if (!node) {
            if (error) *error = trSerializer("Unknown node type: %1").arg(type);
            return false;
        }
        if (item.contains("title")) {
            node->setTitle(item.value("title").toString());
        }
        const QJsonArray pos = item.value("pos").toArray();
        if (pos.size() >= 2) {
            node->setPosition(QPointF(pos[0].toDouble(), pos[1].toDouble()));
        }
        QVariantMap params;
        const QJsonObject paramObject = item.value("params").toObject();
        for (auto it = paramObject.begin(); it != paramObject.end(); ++it) {
            params.insert(it.key(), jsonToVariant(it.value()));
        }
        node->setParameters(params);

        QString addError;
        if (!loaded.addNode(std::move(node), &addError)) {
            if (error) *error = addError;
            return false;
        }
    }

    const QJsonArray edges = object.value("edges").toArray();
    for (const QJsonValue &value : edges) {
        const QJsonObject item = value.toObject();
        Edge edge {
            item.value("fromNode").toString(),
            item.value("fromPort").toString(),
            item.value("toNode").toString(),
            item.value("toPort").toString()
        };
        QString edgeError;
        if (!loaded.addEdge(edge, &edgeError)) {
            if (error) *error = edgeError;
            return false;
        }
    }

    *graph = std::move(loaded);
    return true;
}
