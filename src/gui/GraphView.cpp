#include "gui/GraphView.h"

#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <QWheelEvent>
#include <cmath>

GraphView::GraphView(QGraphicsScene *scene, QWidget *parent)
    : QGraphicsView(scene, parent)
{
    setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
    setDragMode(QGraphicsView::NoDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
}

void GraphView::wheelEvent(QWheelEvent *event)
{
    const Qt::KeyboardModifiers modifiers = event->modifiers();
    if (modifiers.testFlag(Qt::ControlModifier)) {
        const double steps = event->angleDelta().y() / 120.0;
        if (steps != 0.0) {
            const qreal factor = std::pow(1.15, steps);
            const qreal nextScale = transform().m11() * factor;
            if (nextScale >= 0.2 && nextScale <= 4.0) {
                scale(factor, factor);
            }
        }
        event->accept();
        return;
    }

    if (modifiers.testFlag(Qt::AltModifier)) {
        event->accept();
        return;
    }

    QGraphicsView::wheelEvent(event);
}

void GraphView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && !itemAt(event->pos())) {
        m_panning = true;
        m_lastPanPos = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }

    QGraphicsView::mousePressEvent(event);
}

void GraphView::mouseMoveEvent(QMouseEvent *event)
{
    if (m_panning) {
        const QPoint delta = event->pos() - m_lastPanPos;
        m_lastPanPos = event->pos();
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
        event->accept();
        return;
    }

    QGraphicsView::mouseMoveEvent(event);
}

void GraphView::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_panning) {
        m_panning = false;
        unsetCursor();
        event->accept();
        return;
    }

    QGraphicsView::mouseReleaseEvent(event);
}
