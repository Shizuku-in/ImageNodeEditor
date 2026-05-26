#pragma once

#include "core/Node.h"

#include <QWidget>

class QFormLayout;
class QLabel;
class QPushButton;

class ParameterEditorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ParameterEditorWidget(QWidget *parent = nullptr);
    void setNode(Node *node);

signals:
    void parametersChanged();

protected:
    void changeEvent(QEvent *event) override;

private:
    void rebuild();
    QWidget *editorFor(const ParameterSpec &spec, Node *node);

    QFormLayout *m_form = nullptr;
    QLabel *m_emptyLabel = nullptr;
    Node *m_node = nullptr;
};

class ParameterPopup : public QWidget
{
    Q_OBJECT

public:
    explicit ParameterPopup(QWidget *parent = nullptr);
    void setNode(Node *node);

signals:
    void closeRequested();
    void parametersChanged();

protected:
    void changeEvent(QEvent *event) override;

private:
    void retranslateUi();

    QLabel *m_titleLabel = nullptr;
    QLabel *m_typeLabel = nullptr;
    QPushButton *m_closeButton = nullptr;
    ParameterEditorWidget *m_editor = nullptr;
    Node *m_node = nullptr;
};
