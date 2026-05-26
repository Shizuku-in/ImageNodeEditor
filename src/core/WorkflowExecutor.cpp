#include "core/WorkflowExecutor.h"

#include <QCoreApplication>

namespace {
[[maybe_unused]] const char *const kWorkflowExecutorI18nMarkers[] = {
    QT_TRANSLATE_NOOP("WorkflowExecutor", "Node '%1' did not produce required output '%2'."),
    QT_TRANSLATE_NOOP("WorkflowExecutor", "%1: %2"),
    QT_TRANSLATE_NOOP("WorkflowExecutor", "Executed %1 (%2)."),
    QT_TRANSLATE_NOOP("WorkflowExecutor", "Workflow is empty.")
};

QString trExecutor(const char *text)
{
    return QCoreApplication::translate("WorkflowExecutor", text);
}
}

ExecutionReport WorkflowExecutor::execute(WorkflowGraph &graph)
{
    clear();
    ExecutionReport report;

    QStringList validationErrors;
    if (!graph.validate(&validationErrors)) {
        report.ok = false;
        report.messages = validationErrors;
        return report;
    }

    QString topoError;
    const QList<QString> order = graph.topologicalOrder(&topoError);
    if (!topoError.isEmpty()) {
        report.ok = false;
        report.messages.append(topoError);
        return report;
    }

    for (const QString &nodeId : order) {
        Node *node = graph.node(nodeId);
        QHash<QString, DataValue> inputs;
        for (const Edge &edge : graph.edges()) {
            if (edge.toNode != nodeId) {
                continue;
            }
            if (!m_outputs.contains(edge.fromNode) || !m_outputs[edge.fromNode].contains(edge.fromPort)) {
                report.ok = false;
                report.messages.append(trExecutor("Node '%1' did not produce required output '%2'.")
                    .arg(edge.fromNode, edge.fromPort));
                return report;
            }
            inputs.insert(edge.toPort, m_outputs[edge.fromNode][edge.fromPort]);
        }

        const NodeResult result = node->process(inputs);
        if (!result.ok) {
            report.ok = false;
            report.messages.append(trExecutor("%1: %2").arg(node->title(), result.error));
            return report;
        }
        m_outputs.insert(nodeId, result.outputs);
        report.messages.append(trExecutor("Executed %1 (%2).").arg(node->title(), node->typeName()));
    }

    if (order.isEmpty()) {
        report.messages.append(trExecutor("Workflow is empty."));
    }
    return report;
}

QHash<QString, QHash<QString, DataValue>> WorkflowExecutor::outputs() const
{
    return m_outputs;
}

void WorkflowExecutor::clear()
{
    m_outputs.clear();
}
