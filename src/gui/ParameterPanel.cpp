#include "gui/ParameterPanel.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QEvent>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

ParameterPanel::ParameterPanel(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    m_titleLabel = new QLabel(this);
    QFont font = m_titleLabel->font();
    font.setBold(true);
    m_titleLabel->setFont(font);
    layout->addWidget(m_titleLabel);
    m_form = new QFormLayout();
    layout->addLayout(m_form);
    layout->addStretch();
    m_titleLabel->setText(tr("Parameters"));
}

void ParameterPanel::setNode(Node *node)
{
    m_node = node;
    rebuild();
}

void ParameterPanel::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        m_titleLabel->setText(tr("Parameters"));
        rebuild();
    }
    QWidget::changeEvent(event);
}

void ParameterPanel::rebuild()
{
    while (QLayoutItem *item = m_form->takeAt(0)) {
        delete item->widget();
        delete item;
    }
    if (!m_node) {
        m_form->addRow(new QLabel(tr("No node selected."), this));
        return;
    }
    m_form->addRow(new QLabel(QString("%1 (%2)").arg(m_node->title(), m_node->typeName()), this));
    for (const ParameterSpec &spec : m_node->parameterSpecs()) {
        m_form->addRow(spec.displayName, editorFor(spec, m_node));
    }
}

QWidget *ParameterPanel::editorFor(const ParameterSpec &spec, Node *node)
{
    switch (spec.kind) {
    case ParameterKind::Int: {
        auto *editor = new QSpinBox(this);
        editor->setRange(static_cast<int>(spec.minValue), static_cast<int>(spec.maxValue));
        editor->setValue(node->parameter(spec.name).toInt());
        connect(editor, &QSpinBox::valueChanged, this, [this, node, name = spec.name](int value) {
            node->setParameter(name, value);
            emit parametersChanged();
        });
        return editor;
    }
    case ParameterKind::Double: {
        auto *editor = new QDoubleSpinBox(this);
        editor->setRange(spec.minValue, spec.maxValue);
        editor->setSingleStep(spec.step);
        editor->setDecimals(3);
        editor->setValue(node->parameter(spec.name).toDouble());
        connect(editor, &QDoubleSpinBox::valueChanged, this, [this, node, name = spec.name](double value) {
            node->setParameter(name, value);
            emit parametersChanged();
        });
        return editor;
    }
    case ParameterKind::Bool: {
        auto *editor = new QCheckBox(this);
        editor->setChecked(node->parameter(spec.name).toBool());
        connect(editor, &QCheckBox::toggled, this, [this, node, name = spec.name](bool value) {
            node->setParameter(name, value);
            emit parametersChanged();
        });
        return editor;
    }
    case ParameterKind::Choice: {
        auto *editor = new QComboBox(this);
        editor->addItems(spec.choices);
        editor->setCurrentText(node->parameter(spec.name).toString());
        connect(editor, &QComboBox::currentTextChanged, this, [this, node, name = spec.name](const QString &value) {
            node->setParameter(name, value);
            emit parametersChanged();
        });
        return editor;
    }
    case ParameterKind::String:
    case ParameterKind::Color: {
        auto *editor = new QLineEdit(node->parameter(spec.name).toString(), this);
        connect(editor, &QLineEdit::editingFinished, this, [this, editor, node, name = spec.name]() {
            node->setParameter(name, editor->text());
            emit parametersChanged();
        });
        return editor;
    }
    }
    return new QLabel(tr("Unsupported"), this);
}
