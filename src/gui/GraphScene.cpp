#include "gui/GraphScene.h"

#include <QBrush>
#include <QGraphicsSceneMouseEvent>
#include <QKeyEvent>
#include <QPen>

namespace {

QString portKey(const QString &name, PortDirection direction)
{
    return QString("%1:%2").arg(direction == PortDirection::Input ? "in" : "out", name);
}

QColor typeColor(PortDataType type)
{
    switch (type) {
    case PortDataType::Image:
        return QColor("#3b82f6");
    case PortDataType::Mask:
        return QColor("#22c55e");
    case PortDataType::ImageList:
        return QColor("#f97316");
    }
    return Qt::gray;
}

} // namespace

PortItem::PortItem(NodeItem *owner, QString nodeId, QString portName, PortDirection direction, PortDataType dataType)
    : QGraphicsEllipseItem(-6, -6, 12, 12, owner)
    , m_nodeId(std::move(nodeId))
    , m_portName(std::move(portName))
    , m_direction(direction)
    , m_dataType(dataType)
{
    setBrush(typeColor(dataType));
    setPen(QPen(Qt::white, 1));
    setZValue(5);
    setToolTip(QString("%1 (%2)").arg(m_portName, portDataTypeName(m_dataType)));
}

QString PortItem::nodeId() const { return m_nodeId; }
QString PortItem::portName() const { return m_portName; }
PortDirection PortItem::direction() const { return m_direction; }
PortDataType PortItem::dataType() const { return m_dataType; }

EdgeItem::EdgeItem(const Edge &edge)
    : m_edge(edge)
{
    setPen(QPen(QColor("#94a3b8"), 2));
    setZValue(-1);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
}

Edge EdgeItem::edge() const
{
    return m_edge;
}

NodeItem::NodeItem(Node *node, GraphScene *scene)
    : m_node(node)
    , m_graphScene(scene)
{
    const int inputCount = node->inputPorts().size();
    const int outputCount = node->outputPorts().size();
    const int rows = std::max(inputCount, outputCount);
    const qreal width = 190;
    const qreal height = 52 + rows * 24;
    setRect(0, 0, width, height);
    setBrush(QColor("#1f2937"));
    setPen(QPen(QColor("#64748b"), 1.2));
    setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemSendsGeometryChanges);
    setPos(node->position());

    auto *title = new QGraphicsTextItem(node->title(), this);
    title->setDefaultTextColor(Qt::white);
    title->setPos(10, 4);

    int row = 0;
    for (const PortSpec &port : node->inputPorts()) {
        auto *item = new PortItem(this, node->id(), port.name, PortDirection::Input, port.dataType);
        item->setPos(0, 44 + row * 24);
        m_ports.insert(portKey(port.name, PortDirection::Input), item);
        auto *label = new QGraphicsTextItem(port.displayName, this);
        label->setDefaultTextColor(QColor("#dbeafe"));
        label->setPos(12, 31 + row * 24);
        ++row;
    }

    row = 0;
    for (const PortSpec &port : node->outputPorts()) {
        auto *item = new PortItem(this, node->id(), port.name, PortDirection::Output, port.dataType);
        item->setPos(width, 44 + row * 24);
        m_ports.insert(portKey(port.name, PortDirection::Output), item);
        auto *label = new QGraphicsTextItem(port.displayName, this);
        label->setDefaultTextColor(QColor("#fee2e2"));
        label->setPos(width - 82, 31 + row * 24);
        ++row;
    }
}

QString NodeItem::nodeId() const
{
    return m_node ? m_node->id() : QString();
}

PortItem *NodeItem::portItem(const QString &portName, PortDirection direction) const
{
    return m_ports.value(portKey(portName, direction), nullptr);
}

QVariant NodeItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == QGraphicsItem::ItemPositionHasChanged && m_node) {
        m_node->setPosition(pos());
        if (m_graphScene) {
            m_graphScene->refreshEdges();
        }
    }
    return QGraphicsRectItem::itemChange(change, value);
}

GraphScene::GraphScene(WorkflowGraph *graph, const NodeFactory *factory, QObject *parent)
    : QGraphicsScene(parent)
    , m_graph(graph)
    , m_factory(factory)
{
    setSceneRect(-1000, -1000, 3000, 2200);
    connect(this, &QGraphicsScene::selectionChanged, this, &GraphScene::handleSelectionChanged);
}

void GraphScene::addNode(const QString &typeName, const QPointF &position)
{
    auto node = m_factory->create(typeName, m_graph->newNodeId());
    if (!node) {
        emit warningRaised(tr("Unknown node type: %1").arg(typeName));
        return;
    }
    node->setPosition(position);
    QString error;
    if (!m_graph->addNode(std::move(node), &error)) {
        emit warningRaised(error);
        return;
    }
    rebuild();
    emit graphChanged();
}

void GraphScene::rebuild()
{
    clear();
    m_nodeItems.clear();
    m_edgeItems.clear();
    for (Node *node : m_graph->nodes()) {
        auto *item = new NodeItem(node, this);
        addItem(item);
        m_nodeItems.insert(node->id(), item);
    }
    for (const Edge &edge : m_graph->edges()) {
        addEdgeItem(edge);
    }
    refreshEdges();
}

void GraphScene::refreshEdges()
{
    for (EdgeItem *edgeItem : m_edgeItems) {
        PortItem *from = portItem(edgeItem->edge().fromNode, edgeItem->edge().fromPort, PortDirection::Output);
        PortItem *to = portItem(edgeItem->edge().toNode, edgeItem->edge().toPort, PortDirection::Input);
        if (from && to) {
            edgeItem->setLine(QLineF(from->scenePos(), to->scenePos()));
        }
    }
}

PortItem *GraphScene::portItem(const QString &nodeId, const QString &portName, PortDirection direction) const
{
    NodeItem *item = m_nodeItems.value(nodeId, nullptr);
    return item ? item->portItem(portName, direction) : nullptr;
}

NodeItem *GraphScene::nodeItem(const QString &nodeId) const
{
    return m_nodeItems.value(nodeId, nullptr);
}

void GraphScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    PortItem *port = portAt(event->scenePos());
    if (port && event->button() == Qt::LeftButton) {
        m_connectStart = port;
        m_tempLine = addLine(QLineF(port->scenePos(), event->scenePos()), QPen(QColor("#38bdf8"), 2, Qt::DashLine));
        event->accept();
        return;
    }
    QGraphicsScene::mousePressEvent(event);
}

void GraphScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_tempLine && m_connectStart) {
        m_tempLine->setLine(QLineF(m_connectStart->scenePos(), event->scenePos()));
        event->accept();
        return;
    }
    QGraphicsScene::mouseMoveEvent(event);
}

void GraphScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_tempLine && m_connectStart) {
        removeItem(m_tempLine);
        delete m_tempLine;
        m_tempLine = nullptr;

        PortItem *end = portAt(event->scenePos());
        if (end && end != m_connectStart && end->direction() != m_connectStart->direction()) {
            PortItem *from = m_connectStart->direction() == PortDirection::Output ? m_connectStart : end;
            PortItem *to = m_connectStart->direction() == PortDirection::Input ? m_connectStart : end;
            Edge edge {from->nodeId(), from->portName(), to->nodeId(), to->portName()};
            QString error;
            if (m_graph->addEdge(edge, &error)) {
                addEdgeItem(edge);
                refreshEdges();
                emit graphChanged();
            } else {
                emit warningRaised(error);
            }
        }
        m_connectStart = nullptr;
        event->accept();
        return;
    }
    QGraphicsScene::mouseReleaseEvent(event);
}

void GraphScene::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        const QList<QGraphicsItem *> selected = selectedItems();
        for (QGraphicsItem *item : selected) {
            if (auto *nodeItem = dynamic_cast<NodeItem *>(item)) {
                m_graph->removeNode(nodeItem->nodeId());
            } else if (auto *edgeItem = dynamic_cast<EdgeItem *>(item)) {
                m_graph->removeEdge(edgeItem->edge());
            }
        }
        rebuild();
        emit graphChanged();
        event->accept();
        return;
    }
    QGraphicsScene::keyPressEvent(event);
}

void GraphScene::handleSelectionChanged()
{
    for (QGraphicsItem *item : selectedItems()) {
        if (auto *nodeItem = dynamic_cast<NodeItem *>(item)) {
            emit nodeSelected(nodeItem->nodeId());
            return;
        }
    }
    emit nodeSelected(QString());
}

PortItem *GraphScene::portAt(const QPointF &scenePos) const
{
    for (QGraphicsItem *item : items(scenePos)) {
        if (auto *port = dynamic_cast<PortItem *>(item)) {
            return port;
        }
    }
    return nullptr;
}

void GraphScene::addEdgeItem(const Edge &edge)
{
    auto *item = new EdgeItem(edge);
    addItem(item);
    m_edgeItems.append(item);
}
