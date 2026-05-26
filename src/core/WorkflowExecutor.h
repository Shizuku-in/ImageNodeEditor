#pragma once

#include "core/WorkflowGraph.h"

struct ExecutionReport {
    bool ok = true;
    QStringList messages;
};

class WorkflowExecutor
{
public:
    ExecutionReport execute(WorkflowGraph &graph);
    QHash<QString, QHash<QString, DataValue>> outputs() const;
    void clear();

private:
    QHash<QString, QHash<QString, DataValue>> m_outputs;
};

