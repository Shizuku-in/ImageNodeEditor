#pragma once

#include "core/NodeFactory.h"
#include "core/WorkflowGraph.h"

#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QGraphicsScene>
#include <QHash>

class QPainter;
class ParameterPopup;
class QGraphicsSceneContextMenuEvent;
class QGraphicsProxyWidget;
class QStyleOptionGraphicsItem;
class NodeItem;

class PortItem : public QGraphicsEllipseItem
{
public:
    PortItem(NodeItem *owner, QString nodeId, QString portName, PortDirection direction, PortDataType dataType);

    QString nodeId() const;
    QString portName() const;
    PortDirection direction() const;
    PortDataType dataType() const;

private:
    QString m_nodeId;
    QString m_portName;
    PortDirection m_direction;
    PortDataType m_dataType;
};

class EdgeItem : public QGraphicsLineItem
{
public:
    explicit EdgeItem(const Edge &edge);
    Edge edge() const;

private:
    Edge m_edge;
};

class GraphScene : public QGraphicsScene
{
    Q_OBJECT

public:
    GraphScene(WorkflowGraph *graph, const NodeFactory *factory, QObject *parent = nullptr);

    void addNode(const QString &typeName, const QPointF &position);
    void rebuild();
    void refreshEdges();
    PortItem *portItem(const QString &nodeId, const QString &portName, PortDirection direction) const;
    NodeItem *nodeItem(const QString &nodeId) const;
    void showParameterPopup(const QString &nodeId);
    void closeParameterPopup();
    void updateParameterPopupPosition();

signals:
    void nodeSelected(const QString &nodeId);
    void editNodeRequested(const QString &nodeId);
    void graphChanged();
    void warningRaised(const QString &message);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void handleSelectionChanged();

private:
    PortItem *portAt(const QPointF &scenePos) const;
    NodeItem *nodeItemAt(const QPointF &scenePos) const;
    QRectF visibleViewRect() const;
    void addEdgeItem(const Edge &edge);

    WorkflowGraph *m_graph = nullptr;
    const NodeFactory *m_factory = nullptr;
    QHash<QString, NodeItem *> m_nodeItems;
    QList<EdgeItem *> m_edgeItems;
    QGraphicsProxyWidget *m_parameterProxy = nullptr;
    ParameterPopup *m_parameterPopup = nullptr;
    QString m_parameterNodeId;
    PortItem *m_connectStart = nullptr;
    QGraphicsLineItem *m_tempLine = nullptr;
};

class NodeItem : public QGraphicsRectItem
{
public:
    NodeItem(Node *node, GraphScene *scene);

    QString nodeId() const;
    PortItem *portItem(const QString &portName, PortDirection direction) const;
    void refreshParameterLabels();
    void setPreviewImage(const QImage &image);

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

private:
    QRectF editButtonRect() const;
    static QString formatParamValue(const ParameterSpec &spec, const QVariant &value);

    Node *m_node = nullptr;
    GraphScene *m_graphScene = nullptr;
    QHash<QString, PortItem *> m_ports;
    QList<QGraphicsTextItem *> m_paramLabels;
    qreal m_paramStartY = 0;
    QImage m_previewImage;
    qreal m_previewY = 0;
    bool m_supportsPreview = false;
};
