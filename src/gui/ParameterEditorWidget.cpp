#include "gui/ParameterEditorWidget.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDoubleSpinBox>
#include <QEvent>
#include <QFileDialog>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPixmap>
#include <QPointer>
#include <QPushButton>
#include <QSpinBox>
#include <QSvgRenderer>
#include <QTimer>
#include <QVBoxLayout>
#include <utility>

namespace {

class SvgButton final : public QPushButton
{
public:
    explicit SvgButton(QByteArray svgData, QWidget *parent = nullptr)
        : QPushButton(parent)
        , m_svgData(std::move(svgData))
    {
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        QPushButton::paintEvent(event);

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        QSvgRenderer renderer(m_svgData);
        const qreal iconSize = qMin(width(), height()) - 8;
        const QRectF iconRect((width() - iconSize) / 2.0,
                              (height() - iconSize) / 2.0,
                              iconSize,
                              iconSize);
        renderer.render(&painter, iconRect);
    }

private:
    QByteArray m_svgData;
};

} // namespace

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
            emit parameterChanged(name);
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
            emit parameterChanged(name);
        });
        return editor;
    }
    case ParameterKind::Bool: {
        auto *editor = new QCheckBox(this);
        editor->setChecked(node->parameter(spec.name).toBool());
        connect(editor, &QCheckBox::toggled, this, [this, node, name = spec.name](bool value) {
            node->setParameter(name, value);
            emit parametersChanged();
            emit parameterChanged(name);
        });
        return editor;
    }
    case ParameterKind::Choice: {
        auto *editor = new QComboBox(this);
        for (const QString &choice : spec.choices) {
            editor->addItem(QCoreApplication::translate("Nodes", choice.toUtf8().constData()), choice);
        }
        const int index = editor->findData(node->parameter(spec.name).toString());
        editor->setCurrentIndex(index >= 0 ? index : 0);
        connect(editor, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, editor, node, name = spec.name](int index) {
            const QString value = editor->itemData(index).toString();
            node->setParameter(name, value);
            emit parametersChanged();
            emit parameterChanged(name);
        });
        return editor;
    }
    case ParameterKind::FilePath: {
        auto *container = new QWidget(this);
        auto *hbox = new QHBoxLayout(container);
        hbox->setContentsMargins(0, 0, 0, 0);
        hbox->setSpacing(4);
        auto *lineEdit = new QLineEdit(node->parameter(spec.name).toString(), container);
        auto *browseBtn = new QPushButton(QString::fromUtf8("\xe2\x80\xa6"), container);
        browseBtn->setFixedWidth(28);
        hbox->addWidget(lineEdit, 1);
        hbox->addWidget(browseBtn);
        connect(lineEdit, &QLineEdit::textEdited, this, [this, node, name = spec.name](const QString &text) {
            node->setParameter(name, text);
            emit parametersChanged();
            emit parameterChanged(name);
        });
        const bool isSave = !spec.choices.isEmpty() && spec.choices.first() == "save";
        const QString filter = spec.choices.size() > 1 ? spec.choices.at(1) : QString();
        connect(browseBtn, &QPushButton::clicked, this, [this, lineEdit, node, name = spec.name, isSave, filter]() {
            QPointer<QLineEdit> safeEdit = lineEdit;
            QPointer<ParameterEditorWidget> self = this;
            QString currentPath = lineEdit->text();
            // Defer dialog to next event loop iteration to avoid corrupting
            // the QGraphicsProxyWidget / QGraphicsScene state.
            QTimer::singleShot(0, QApplication::instance(), [self, safeEdit, node, name, isSave, filter, currentPath]() {
                QWidget *dialogParent = QApplication::activeWindow();
                QString path;
                if (isSave) {
                    path = QFileDialog::getSaveFileName(dialogParent,
                        ParameterEditorWidget::tr("Select File"), currentPath, filter);
                } else {
                    path = QFileDialog::getOpenFileName(dialogParent,
                        ParameterEditorWidget::tr("Select File"), currentPath, filter);
                }
                if (!path.isEmpty()) {
                    node->setParameter(name, path);
                    if (safeEdit) {
                        safeEdit->setText(path);
                    }
                    if (self) {
                        emit self->parametersChanged();
                        emit self->parameterChanged(name);
                    }
                }
            });
        });
        return container;
    }
    case ParameterKind::String:
    case ParameterKind::Color: {
        auto *editor = new QLineEdit(node->parameter(spec.name).toString(), this);
        connect(editor, &QLineEdit::editingFinished, this, [this, editor, node, name = spec.name]() {
            node->setParameter(name, editor->text());
            emit parametersChanged();
            emit parameterChanged(name);
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
    setMinimumWidth(240);
    setMaximumWidth(280);
    QFont popupFont("Microsoft YaHei UI");
    popupFont.setPointSizeF(8.5);
    setFont(popupFont);
    setStyleSheet(
        "QWidget#ParameterPopup { background: #202124; border: 1px solid #5f6368; border-radius: 6px; }"
        "QLabel { color: #f8fafc; }"
        "QLabel#TypeLabel { color: #94a3b8; }"
    );

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 8, 10, 10);
    layout->setSpacing(6);

    auto *header = new QHBoxLayout();
    auto *titles = new QVBoxLayout();
    titles->setSpacing(0);
    m_titleLabel = new QLabel(this);
    QFont titleFont = m_titleLabel->font();
    titleFont.setBold(true);
    titleFont.setPointSizeF(9.5);
    m_titleLabel->setFont(titleFont);
    m_typeLabel = new QLabel(this);
    m_typeLabel->setObjectName("TypeLabel");
    titles->addWidget(m_titleLabel);
    titles->addWidget(m_typeLabel);

    static const QByteArray closeSvg =
        "<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24'>"
        "<path fill='#e5e7eb' d='M19 6.41 17.59 5 12 10.59 6.41 5 5 6.41 10.59 12 5 17.59 6.41 19 12 13.41 17.59 19 19 17.59 13.41 12z'/>"
        "</svg>";
    m_closeButton = new SvgButton(closeSvg, this);
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
    connect(m_editor, &ParameterEditorWidget::parameterChanged, this, &ParameterPopup::parameterChanged);
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
        m_closeButton->setToolTip(tr("Close"));
    }
}
