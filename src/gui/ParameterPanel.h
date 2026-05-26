#pragma once

#include "core/Node.h"

#include <QWidget>

class QFormLayout;
class QLabel;

class ParameterPanel : public QWidget
{
    Q_OBJECT

public:
    explicit ParameterPanel(QWidget *parent = nullptr);
    void setNode(Node *node);

signals:
    void parametersChanged();

protected:
    void changeEvent(QEvent *event) override;

private:
    void rebuild();
    QWidget *editorFor(const ParameterSpec &spec, Node *node);

    QFormLayout *m_form = nullptr;
    QLabel *m_titleLabel = nullptr;
    Node *m_node = nullptr;
};
