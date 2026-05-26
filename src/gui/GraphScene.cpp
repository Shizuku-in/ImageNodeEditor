#include "gui/GraphScene.h"

#include "gui/ParameterEditorWidget.h"

#include <QBrush>
#include <QGraphicsProxyWidget>
#include <QGraphicsSceneMouseEvent>
#include <QKeyEvent>
#include <QPainter>
#include <QPen>
#include <QSvgRenderer>
#include <cmath>

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
    const QVector<ParameterSpec> specs = node->parameterSpecs();
    const int paramCount = specs.size();
    const qreal width = 190;
    const qreal portAreaEnd = 52 + rows * 24;
    const qreal paramAreaHeight = paramCount > 0 ? 6 + paramCount * 18 : 0;
    const qreal height = portAreaEnd + paramAreaHeight;
    setRect(0, 0, width, height);
    setBrush(QColor("#1f2937"));
    setPen(QPen(QColor("#64748b"), 1.2));
    setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemSendsGeometryChanges);
    setPos(node->position());

    auto *title = new QGraphicsTextItem(node->title(), this);
    title->setDefaultTextColor(Qt::white);
    title->setPos(10, 4);
    title->setTextWidth(width - 44);

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

    // Parameter summary area
    m_paramStartY = portAreaEnd;
    if (paramCount > 0) {
        QFont paramFont;
        paramFont.setPointSizeF(8.0);

        for (int i = 0; i < paramCount; ++i) {
            const ParameterSpec &spec = specs[i];
            const QString text = spec.displayName + ": " + formatParamValue(spec, node->parameter(spec.name));
            auto *label = new QGraphicsTextItem(text, this);
            label->setDefaultTextColor(QColor("#94a3b8"));
            label->setFont(paramFont);
            label->setPos(10, m_paramStartY + i * 18 - 2);
            label->setTextWidth(width - 20);
            m_paramLabels.append(label);
        }
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

void NodeItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    QGraphicsRectItem::paint(painter, option, widget);

    const QRectF button = editButtonRect();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(QPen(QColor("#94a3b8"), 1));
    painter->setBrush(isSelected() ? QColor("#334155") : QColor("#273449"));
    painter->drawRoundedRect(button, 4, 4);

    static const QByteArray svgData =
        "<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24'>"
        "<path fill='#e5e7eb' d='"
        "M19.43 12.98c.04-.32.07-.64.07-.98s-.03-.66-.07-.98l2.11-1.65"
        "c.19-.15.24-.42.12-.64l-2-3.46c-.09-.16-.26-.25-.44-.25-.06 0-.12.01-.17.03"
        "l-2.49 1c-.52-.4-1.08-.73-1.69-.98l-.38-2.65C14.46 2.18 14.25 2 14 2h-4"
        "c-.25 0-.46.18-.49.42l-.38 2.65c-.61.25-1.17.59-1.69.98l-2.49-1"
        "q-.09-.03-.18-.03c-.17 0-.34.09-.43.25l-2 3.46c-.13.22-.07.49.12.64"
        "l2.11 1.65c-.04.32-.07.65-.07.98s.03.66.07.98l-2.11 1.65"
        "c-.19.15-.24.42-.12.64l2 3.46c.09.16.26.25.44.25.06 0 .12-.01.17-.03"
        "l2.49-1c.52.4 1.08.73 1.69.98l.38 2.65c.03.24.24.42.49.42h4"
        "c.25 0 .46-.18.49-.42l.38-2.65c.61-.25 1.17-.59 1.69-.98l2.49 1"
        "q.09.03.18.03c.17 0 .34-.09.43-.25l2-3.46c.12-.22.07-.49-.12-.64"
        "zm-1.98-1.71c.04.31.05.52.05.73s-.02.43-.05.73l-.14 1.13.89.7 1.08.84"
        "-.7 1.21-1.27-.51-1.04-.42-.9.68c-.43.32-.84.56-1.25.73l-1.06.43-.16 1.13"
        "-.2 1.35h-1.4l-.19-1.35-.16-1.13-1.06-.43c-.43-.18-.83-.41-1.23-.71"
        "l-.91-.7-1.06.43-1.27.51-.7-1.21 1.08-.84.89-.7-.14-1.13"
        "c-.03-.31-.05-.54-.05-.74s.02-.43.05-.73l.14-1.13-.89-.7-1.08-.84"
        ".7-1.21 1.27.51 1.04.42.9-.68c.43-.32.84-.56 1.25-.73l1.06-.43.16-1.13"
        ".2-1.35h1.39l.19 1.35.16 1.13 1.06.43c.43.18.83.41 1.23.71l.91.7 1.06-.43"
        " 1.27-.51.7 1.21-1.07.85-.89.7zM12 8c-2.21 0-4 1.79-4 4s1.79 4 4 4"
        " 4-1.79 4-4-1.79-4-4-4m0 6c-1.1 0-2-.9-2-2s.9-2 2-2 2 .9 2 2-.9 2-2 2"
        "'/></svg>";

    QSvgRenderer renderer(svgData);
    const qreal iconSize = qMin(button.width(), button.height()) - 4;
    const QRectF iconRect(button.center().x() - iconSize / 2,
                          button.center().y() - iconSize / 2,
                          iconSize, iconSize);
    renderer.render(painter, iconRect);

    // Draw separator line above parameter area
    if (!m_paramLabels.isEmpty()) {
        painter->setPen(QPen(QColor("#3f3f46"), 0.8));
        painter->drawLine(QPointF(8, m_paramStartY), QPointF(rect().width() - 8, m_paramStartY));
    }
}

void NodeItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && editButtonRect().contains(event->pos())) {
        if (m_graphScene) {
            setSelected(true);
            m_graphScene->showParameterPopup(nodeId());
        }
        event->accept();
        return;
    }
    QGraphicsRectItem::mousePressEvent(event);
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

QRectF NodeItem::editButtonRect() const
{
    const QRectF r = rect();
    return QRectF(r.right() - 32, r.top() + 8, 22, 22);
}

void NodeItem::refreshParameterLabels()
{
    if (!m_node) return;
    const QVector<ParameterSpec> specs = m_node->parameterSpecs();
    for (int i = 0; i < m_paramLabels.size() && i < specs.size(); ++i) {
        const ParameterSpec &spec = specs[i];
        const QString text = spec.displayName + ": " + formatParamValue(spec, m_node->parameter(spec.name));
        m_paramLabels[i]->setPlainText(text);
    }
}

QString NodeItem::formatParamValue(const ParameterSpec &spec, const QVariant &value)
{
    switch (spec.kind) {
    case ParameterKind::Bool:
        return value.toBool() ? QString::fromUtf8("\xe2\x9c\x93") : QString::fromUtf8("\xe2\x9c\x97");
    case ParameterKind::Double:
        return QString::number(value.toDouble(), 'f', 2);
    case ParameterKind::String: {
        QString s = value.toString();
        if (s.length() > 18) {
            s = s.left(16) + QString::fromUtf8("\xe2\x80\xa6");
        }
        return s;
    }
    case ParameterKind::Int:
    case ParameterKind::Color:
    case ParameterKind::Choice:
        return value.toString();
    }
    return value.toString();
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
    closeParameterPopup();
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

void GraphScene::showParameterPopup(const QString &nodeId)
{
    Node *node = m_graph->node(nodeId);
    NodeItem *item = nodeItem(nodeId);
    if (!node || !item) {
        closeParameterPopup();
        return;
    }

    closeParameterPopup();
    emit editNodeRequested(nodeId);

    m_parameterPopup = new ParameterPopup();
    m_parameterPopup->setNode(node);
    connect(m_parameterPopup, &ParameterPopup::closeRequested, this, &GraphScene::closeParameterPopup);
    connect(m_parameterPopup, &ParameterPopup::parametersChanged, this, [this, item]() {
        item->refreshParameterLabels();
        emit graphChanged();
    });

    m_parameterProxy = addWidget(m_parameterPopup);
    m_parameterProxy->setZValue(20);
    m_parameterPopup->adjustSize();

    const QRectF nodeRect = item->sceneBoundingRect();
    const QSizeF popupSize = m_parameterProxy->boundingRect().size();
    QPointF popupPos = nodeRect.topRight() + QPointF(16, 0);
    if (popupPos.x() + popupSize.width() > sceneRect().right()) {
        popupPos = nodeRect.bottomLeft() + QPointF(0, 16);
    }
    if (popupPos.y() + popupSize.height() > sceneRect().bottom()) {
        popupPos.setY(nodeRect.top() - popupSize.height() - 16);
    }
    m_parameterProxy->setPos(popupPos);
}

void GraphScene::closeParameterPopup()
{
    if (!m_parameterProxy) {
        m_parameterPopup = nullptr;
        return;
    }
    removeItem(m_parameterProxy);
    delete m_parameterProxy;
    m_parameterProxy = nullptr;
    m_parameterPopup = nullptr;
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
        closeParameterPopup();
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
