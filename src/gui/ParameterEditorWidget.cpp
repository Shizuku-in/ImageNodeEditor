#include "gui/ParameterEditorWidget.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QEvent>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

ParameterEditorWidget::ParameterEditorWidget(QWidget *parent)
    : QWidget(parent)
{
    m_form = new QFormLayout(this);
    m_form->setContentsMargins(0, 0, 0, 0);
    m_form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
}

void ParameterEditorWidget::setNode(Node *node)
{
    m_node = node;
    rebuild();
}

void ParameterEditorWidget::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        rebuild();
    }
    QWidget::changeEvent(event);
}

void ParameterEditorWidget::rebuild()
{
    while (QLayoutItem *item = m_form->takeAt(0)) {
        delete item->widget();
        delete item;
    }
    if (!m_node) {
        m_form->addRow(new QLabel(tr("No node selected."), this));
        return;
    }
    const QVector<ParameterSpec> specs = m_node->parameterSpecs();
    if (specs.isEmpty()) {
        m_form->addRow(new QLabel(tr("No editable parameters."), this));
        return;
    }
    for (const ParameterSpec &spec : specs) {
        m_form->addRow(spec.displayName, editorFor(spec, m_node));
    }
}

QWidget *ParameterEditorWidget::editorFor(const ParameterSpec &spec, Node *node)
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

ParameterPopup::ParameterPopup(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("ParameterPopup");
    setMinimumWidth(280);
    setStyleSheet(
        "QWidget#ParameterPopup { background: #202124; border: 1px solid #5f6368; border-radius: 6px; }"
        "QLabel { color: #f8fafc; }"
        "QLabel#TypeLabel { color: #94a3b8; }"
    );

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 10, 12, 12);
    layout->setSpacing(8);

    auto *header = new QHBoxLayout();
    auto *titles = new QVBoxLayout();
    titles->setSpacing(0);
    m_titleLabel = new QLabel(this);
    QFont titleFont = m_titleLabel->font();
    titleFont.setBold(true);
    m_titleLabel->setFont(titleFont);
    m_typeLabel = new QLabel(this);
    m_typeLabel->setObjectName("TypeLabel");
    titles->addWidget(m_titleLabel);
    titles->addWidget(m_typeLabel);

    m_closeButton = new QPushButton(this);
    m_closeButton->setFixedSize(28, 24);
    m_closeButton->setFlat(true);
    connect(m_closeButton, &QPushButton::clicked, this, &ParameterPopup::closeRequested);

    header->addLayout(titles, 1);
    header->addWidget(m_closeButton);
    layout->addLayout(header);

    auto *line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet("color: #3f3f46;");
    layout->addWidget(line);

    m_editor = new ParameterEditorWidget(this);
    connect(m_editor, &ParameterEditorWidget::parametersChanged, this, &ParameterPopup::parametersChanged);
    layout->addWidget(m_editor);

    retranslateUi();
}

void ParameterPopup::setNode(Node *node)
{
    m_node = node;
    if (m_editor) {
        m_editor->setNode(node);
    }
    retranslateUi();
}

void ParameterPopup::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QWidget::changeEvent(event);
}

void ParameterPopup::retranslateUi()
{
    if (m_titleLabel) {
        m_titleLabel->setText(m_node ? m_node->title() : tr("Edit Parameters"));
    }
    if (m_typeLabel) {
        m_typeLabel->setText(m_node ? m_node->typeName() : QString());
    }
    if (m_closeButton) {
        m_closeButton->setText(tr("Close"));
        m_closeButton->setToolTip(tr("Close"));
    }
}

