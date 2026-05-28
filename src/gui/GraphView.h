#pragma once

#include <QGraphicsView>
#include <QPoint>

class GraphView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit GraphView(QGraphicsScene *scene, QWidget *parent = nullptr);

protected:
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    bool m_panning = false;
    QPoint m_lastPanPos;
};
