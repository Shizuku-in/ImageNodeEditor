#pragma once

#include "core/Node.h"

#include <QStringList>
#include <map>
#include <memory>

struct Edge {
    QString fromNode;
    QString fromPort;
    QString toNode;
    QString toPort;
};

class WorkflowGraph
{
public:
    bool addNode(std::unique_ptr<Node> node, QString *error = nullptr);
    std::unique_ptr<Node> takeNode(const QString &id);
    bool removeNode(const QString &id);
    Node *node(const QString &id) const;
    QList<Node *> nodes() const;
    QStringList nodeIds() const;

    bool addEdge(const Edge &edge, QString *error = nullptr);
    bool removeEdge(const Edge &edge);
    void clearEdgesForNode(const QString &nodeId);
    QList<Edge> edges() const;
    void clear();

    bool validate(QStringList *errors = nullptr) const;
    bool wouldCreateCycle(const Edge &edge) const;
    QList<QString> topologicalOrder(QString *error = nullptr) const;
    QString newNodeId() const;

private:
    bool hasPath(const QString &from, const QString &to, const Edge *extraEdge = nullptr) const;
    QList<Edge> outgoing(const QString &nodeId, const Edge *extraEdge = nullptr) const;

    std::map<QString, std::unique_ptr<Node>> m_nodes;
    QList<Edge> m_edges;
};

inline bool operator==(const Edge &a, const Edge &b)
{
    return a.fromNode == b.fromNode && a.fromPort == b.fromPort
        && a.toNode == b.toNode && a.toPort == b.toPort;
}
