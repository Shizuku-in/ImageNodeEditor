#pragma once

#include "core/NodeFactory.h"
#include "core/WorkflowGraph.h"

#include <QJsonObject>

class WorkflowSerializer
{
public:
    static bool loadFromFile(const QString &path, const NodeFactory &factory, WorkflowGraph *graph, QString *error);
    static bool saveToFile(const QString &path, const WorkflowGraph &graph, QString *error);
    static QJsonObject toJson(const WorkflowGraph &graph);
    static bool fromJson(const QJsonObject &object, const NodeFactory &factory, WorkflowGraph *graph, QString *error);
};

