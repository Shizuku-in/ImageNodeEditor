#include "core/WorkflowGraph.h"

#include <QCoreApplication>
#include <QQueue>
#include <QSet>

namespace {
[[maybe_unused]] const char *const kWorkflowGraphI18nMarkers[] = {
    QT_TRANSLATE_NOOP("WorkflowGraph", "Cannot add a null node."),
    QT_TRANSLATE_NOOP("WorkflowGraph", "Node id cannot be empty."),
    QT_TRANSLATE_NOOP("WorkflowGraph", "Duplicate node id: %1"),
    QT_TRANSLATE_NOOP("WorkflowGraph", "Connection references a missing node."),
    QT_TRANSLATE_NOOP("WorkflowGraph", "Connection references a missing port."),
    QT_TRANSLATE_NOOP("WorkflowGraph", "%1 cannot connect to %2."),
    QT_TRANSLATE_NOOP("WorkflowGraph", "This connection already exists."),
    QT_TRANSLATE_NOOP("WorkflowGraph", "This input port is already connected."),
    QT_TRANSLATE_NOOP("WorkflowGraph", "This connection would create a cycle."),
    QT_TRANSLATE_NOOP("WorkflowGraph", "Node '%1' input '%2' is not connected."),
    QT_TRANSLATE_NOOP("WorkflowGraph", "Graph contains an edge with a missing node."),
    QT_TRANSLATE_NOOP("WorkflowGraph", "Workflow graph contains a cycle.")
};

QString trGraph(const char *text)
{
    return QCoreApplication::translate("WorkflowGraph", text);
}
}

bool WorkflowGraph::addNode(std::unique_ptr<Node> node, QString *error)
{
    if (!node) {
        if (error) *error = trGraph("Cannot add a null node.");
        return false;
    }
    if (node->id().isEmpty()) {
        if (error) *error = trGraph("Node id cannot be empty.");
        return false;
    }
    if (m_nodes.find(node->id()) != m_nodes.end()) {
        if (error) *error = trGraph("Duplicate node id: %1").arg(node->id());
        return false;
    }
    m_nodes.emplace(node->id(), std::move(node));
    return true;
}

std::unique_ptr<Node> WorkflowGraph::takeNode(const QString &id)
{
    auto it = m_nodes.find(id);
    if (it == m_nodes.end()) {
        return nullptr;
    }
    auto node = std::move(it->second);
    m_nodes.erase(it);
    clearEdgesForNode(id);
    return node;
}

bool WorkflowGraph::removeNode(const QString &id)
{
    return takeNode(id) != nullptr;
}

Node *WorkflowGraph::node(const QString &id) const
{
    auto it = m_nodes.find(id);
    return it == m_nodes.end() ? nullptr : it->second.get();
}

QList<Node *> WorkflowGraph::nodes() const
{
    QList<Node *> result;
    for (const auto &entry : m_nodes) {
        result.append(entry.second.get());
    }
    return result;
}

QStringList WorkflowGraph::nodeIds() const
{
    QStringList ids;
    for (const auto &entry : m_nodes) {
        ids.append(entry.first);
    }
    return ids;
}

bool WorkflowGraph::addEdge(const Edge &edge, QString *error)
{
    Node *from = node(edge.fromNode);
    Node *to = node(edge.toNode);
    if (!from || !to) {
        if (error) *error = trGraph("Connection references a missing node.");
        return false;
    }
    const auto fromPort = from->findPort(PortDirection::Output, edge.fromPort);
    const auto toPort = to->findPort(PortDirection::Input, edge.toPort);
    if (!fromPort || !toPort) {
        if (error) *error = trGraph("Connection references a missing port.");
        return false;
    }
    if (fromPort->dataType != toPort->dataType) {
        if (error) {
            *error = trGraph("%1 cannot connect to %2.")
                .arg(portDataTypeName(fromPort->dataType), portDataTypeName(toPort->dataType));
        }
        return false;
    }
    for (const Edge &existing : m_edges) {
        if (existing == edge) {
            if (error) *error = trGraph("This connection already exists.");
            return false;
        }
        if (existing.toNode == edge.toNode && existing.toPort == edge.toPort) {
            if (error) *error = trGraph("This input port is already connected.");
            return false;
        }
    }
    if (wouldCreateCycle(edge)) {
        if (error) *error = trGraph("This connection would create a cycle.");
        return false;
    }
    m_edges.append(edge);
    return true;
}

bool WorkflowGraph::removeEdge(const Edge &edge)
{
    const int removed = m_edges.removeAll(edge);
    return removed > 0;
}

void WorkflowGraph::clearEdgesForNode(const QString &nodeId)
{
    for (int i = m_edges.size() - 1; i >= 0; --i) {
        if (m_edges[i].fromNode == nodeId || m_edges[i].toNode == nodeId) {
            m_edges.removeAt(i);
        }
    }
}

QList<Edge> WorkflowGraph::edges() const
{
    return m_edges;
}

void WorkflowGraph::clear()
{
    m_edges.clear();
    m_nodes.clear();
}

bool WorkflowGraph::validate(QStringList *errors) const
{
    QStringList localErrors;
    for (Node *n : nodes()) {
        for (const PortSpec &input : n->inputPorts()) {
            if (!input.required) {
                continue;
            }
            bool connected = false;
            for (const Edge &edge : m_edges) {
                connected = edge.toNode == n->id() && edge.toPort == input.name;
                if (connected) {
                    break;
                }
            }
            if (!connected) {
                localErrors.append(trGraph("Node '%1' input '%2' is not connected.")
                    .arg(n->title(), input.displayName));
            }
        }
    }
    QString topoError;
    topologicalOrder(&topoError);
    if (!topoError.isEmpty()) {
        localErrors.append(topoError);
    }
    if (errors) {
        *errors = localErrors;
    }
    return localErrors.isEmpty();
}

bool WorkflowGraph::wouldCreateCycle(const Edge &edge) const
{
    return hasPath(edge.toNode, edge.fromNode, &edge);
}

QList<QString> WorkflowGraph::topologicalOrder(QString *error) const
{
    QHash<QString, int> indegree;
    for (const auto &entry : m_nodes) {
        const QString &id = entry.first;
        indegree.insert(id, 0);
    }
    for (const Edge &edge : m_edges) {
        if (!indegree.contains(edge.fromNode) || !indegree.contains(edge.toNode)) {
            if (error) *error = trGraph("Graph contains an edge with a missing node.");
            return {};
        }
        indegree[edge.toNode] += 1;
    }
    QQueue<QString> queue;
    for (auto it = indegree.begin(); it != indegree.end(); ++it) {
        if (it.value() == 0) {
            queue.enqueue(it.key());
        }
    }
    QList<QString> order;
    while (!queue.isEmpty()) {
        const QString id = queue.dequeue();
        order.append(id);
        for (const Edge &edge : outgoing(id)) {
            indegree[edge.toNode] -= 1;
            if (indegree[edge.toNode] == 0) {
                queue.enqueue(edge.toNode);
            }
        }
    }
    if (order.size() != static_cast<qsizetype>(m_nodes.size())) {
        if (error) *error = trGraph("Workflow graph contains a cycle.");
        return {};
    }
    if (error) error->clear();
    return order;
}

QString WorkflowGraph::newNodeId() const
{
    int index = m_nodes.size() + 1;
    QString id;
    do {
        id = QString("n%1").arg(index++);
    } while (m_nodes.find(id) != m_nodes.end());
    return id;
}

bool WorkflowGraph::hasPath(const QString &from, const QString &to, const Edge *extraEdge) const
{
    QSet<QString> visited;
    QQueue<QString> queue;
    queue.enqueue(from);
    while (!queue.isEmpty()) {
        const QString current = queue.dequeue();
        if (current == to) {
            return true;
        }
        if (visited.contains(current)) {
            continue;
        }
        visited.insert(current);
        for (const Edge &edge : outgoing(current, extraEdge)) {
            queue.enqueue(edge.toNode);
        }
    }
    return false;
}

QList<Edge> WorkflowGraph::outgoing(const QString &nodeId, const Edge *extraEdge) const
{
    QList<Edge> result;
    for (const Edge &edge : m_edges) {
        if (edge.fromNode == nodeId) {
            result.append(edge);
        }
    }
    if (extraEdge && extraEdge->fromNode == nodeId) {
        result.append(*extraEdge);
    }
    return result;
}
