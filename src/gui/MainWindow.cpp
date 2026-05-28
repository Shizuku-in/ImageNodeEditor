#include "gui/MainWindow.h"

#include "gui/GraphScene.h"
#include "gui/GraphView.h"
#include "i18n/LanguageManager.h"
#include "io/WorkflowSerializer.h"
#include "nodes/ImageNodes.h"

#include <QAction>
#include <QEvent>
#include <QFileDialog>
#include <QImage>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QSplitter>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    registerImageNodes(m_factory);
    setupUi();
    connect(&LanguageManager::instance(), &LanguageManager::languageChanged, this, &MainWindow::retranslateUi);
}

void MainWindow::setupUi()
{
    setWindowTitle("ImageNodeEditor");
    resize(1280, 820);

    m_workflowToolBar = addToolBar(QString());
    m_newAction = m_workflowToolBar->addAction(QString(), this, &MainWindow::newWorkflow);
    m_openAction = m_workflowToolBar->addAction(QString(), this, &MainWindow::openWorkflow);
    m_saveAction = m_workflowToolBar->addAction(QString(), this, &MainWindow::saveWorkflow);
    m_workflowToolBar->addSeparator();
    m_runAction = m_workflowToolBar->addAction(QString(), this, &MainWindow::runWorkflow);
    m_workflowToolBar->addSeparator();

    m_languageMenu = new QMenu(this);
    m_englishAction = m_languageMenu->addAction(QString(), this, &MainWindow::switchToEnglish);
    m_chineseAction = m_languageMenu->addAction(QString(), this, &MainWindow::switchToChinese);
    m_languageButton = new QToolButton(this);
    m_languageButton->setPopupMode(QToolButton::InstantPopup);
    m_languageButton->setMenu(m_languageMenu);
    m_languageButton->setAutoRaise(true);
    m_workflowToolBar->addWidget(m_languageButton);

    m_nodeList = new QListWidget(this);
    populateNodeList();
    connect(m_nodeList, &QListWidget::itemDoubleClicked, this, &MainWindow::addSelectedNode);

    m_scene = new GraphScene(&m_graph, &m_factory, this);
    connect(m_scene, &GraphScene::nodeSelected, this, &MainWindow::onNodeSelected);
    connect(m_scene, &GraphScene::warningRaised, this, &MainWindow::showWarning);
    connect(m_scene, &GraphScene::graphChanged, this, [this]() {
        markDirty();
        updateNodePreviews();
    });
    auto *view = new GraphView(m_scene, this);

    m_log = new QPlainTextEdit(this);
    m_log->setReadOnly(true);
    m_log->setMaximumBlockCount(500);

    auto *leftPanel = new QWidget(this);
    auto *leftLayout = new QVBoxLayout(leftPanel);
    m_nodeLibraryLabel = new QLabel(this);
    leftLayout->addWidget(m_nodeLibraryLabel);
    leftLayout->addWidget(m_nodeList);

    auto *center = new QSplitter(Qt::Vertical, this);
    center->addWidget(view);
    center->addWidget(m_log);
    center->setStretchFactor(0, 4);
    center->setStretchFactor(1, 1);

    auto *root = new QSplitter(Qt::Horizontal, this);
    root->addWidget(leftPanel);
    root->addWidget(center);
    root->setStretchFactor(0, 0);
    root->setStretchFactor(1, 4);
    setCentralWidget(root);
    retranslateUi();
}

void MainWindow::addSelectedNode()
{
    QListWidgetItem *item = m_nodeList->currentItem();
    if (!item) {
        return;
    }
    const QString typeName = item->data(Qt::UserRole).toString();
    m_scene->addNode(typeName, QPointF(80 + m_graph.nodes().size() * 35, 80 + m_graph.nodes().size() * 25));
}

void MainWindow::onNodeSelected(const QString &nodeId)
{
    m_selectedNodeId = nodeId;
}

void MainWindow::runWorkflow()
{
    m_log->clear();
    const ExecutionReport report = m_executor.execute(m_graph);
    for (const QString &message : report.messages) {
        appendLog(message);
    }
    if (!report.ok) {
        showWarning(report.messages.join("\n"));
        return;
    }
    appendLog(tr("Workflow executed successfully."));
    updateNodePreviews();
}

void MainWindow::newWorkflow()
{
    m_graph.clear();
    m_executor.clear();
    m_scene->rebuild();
    updateNodePreviews();
    markDirty();
}

void MainWindow::openWorkflow()
{
    const QString path = QFileDialog::getOpenFileName(this, tr("Open Workflow"), QString(), tr("Workflow (*.json)"));
    if (path.isEmpty()) {
        return;
    }
    QString error;
    if (!WorkflowSerializer::loadFromFile(path, m_factory, &m_graph, &error)) {
        showWarning(error);
        return;
    }
    m_executor.clear();
    m_scene->rebuild();
    updateNodePreviews();
    appendLog(tr("Loaded %1").arg(path));
    m_dirty = false;
}

void MainWindow::saveWorkflow()
{
    const QString path = QFileDialog::getSaveFileName(this, tr("Save Workflow"), QString(), tr("Workflow (*.json)"));
    if (path.isEmpty()) {
        return;
    }
    QString error;
    if (!WorkflowSerializer::saveToFile(path, m_graph, &error)) {
        showWarning(error);
        return;
    }
    appendLog(tr("Saved %1").arg(path));
    m_dirty = false;
}

void MainWindow::showWarning(const QString &message)
{
    appendLog(tr("Error: %1").arg(message));
    QMessageBox::warning(this, "ImageNodeEditor", message);
}

void MainWindow::markDirty()
{
    m_dirty = true;
}

void MainWindow::switchToEnglish()
{
    LanguageManager::instance().setLanguage("en_US");
}

void MainWindow::switchToChinese()
{
    LanguageManager::instance().setLanguage("zh_CN");
}

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QMainWindow::changeEvent(event);
}

void MainWindow::retranslateUi()
{
    setWindowTitle("ImageNodeEditor");
    if (m_languageMenu) m_languageMenu->setTitle(tr("Language"));
    if (m_englishAction) m_englishAction->setText(tr("English"));
    if (m_chineseAction) m_chineseAction->setText(tr("Simplified Chinese"));
    if (m_workflowToolBar) m_workflowToolBar->setWindowTitle(tr("Workflow"));
    if (m_newAction) m_newAction->setText(tr("New"));
    if (m_openAction) m_openAction->setText(tr("Open"));
    if (m_saveAction) m_saveAction->setText(tr("Save"));
    if (m_runAction) m_runAction->setText(tr("Run"));
    if (m_languageButton) m_languageButton->setText(tr("Language"));
    if (m_nodeLibraryLabel) m_nodeLibraryLabel->setText(tr("Node Library"));

    populateNodeList();
    refreshDefaultNodeTitles();
    m_scene->rebuild();
    updateNodePreviews();
}

void MainWindow::populateNodeList()
{
    if (!m_nodeList) {
        return;
    }
    const QString selectedType = m_nodeList->currentItem() ? m_nodeList->currentItem()->data(Qt::UserRole).toString() : QString();
    m_nodeList->clear();
    for (const QString &type : m_factory.availableTypes()) {
        auto node = m_factory.create(type, "__preview__");
        const QString display = node ? node->title() : type;
        auto *item = new QListWidgetItem(display, m_nodeList);
        item->setData(Qt::UserRole, type);
        if (type == selectedType) {
            item->setSelected(true);
            m_nodeList->setCurrentItem(item);
        }
    }
}

void MainWindow::refreshDefaultNodeTitles()
{
    for (Node *node : m_graph.nodes()) {
        if (node->hasCustomTitle()) {
            continue;
        }
        auto localizedNode = m_factory.create(node->typeName(), "__title__");
        if (localizedNode) {
            node->setTitle(localizedNode->title(), false);
        }
    }
}

void MainWindow::updateNodePreviews()
{
    QHash<QString, QHash<QString, DataValue>> liveOutputs;
    QString topoError;
    const QList<QString> order = m_graph.topologicalOrder(&topoError);

    if (!topoError.isEmpty()) {
        for (Node *node : m_graph.nodes()) {
            if (node->supportsPreview()) {
                if (NodeItem *item = m_scene->nodeItem(node->id())) {
                    item->setPreviewImage(QImage());
                }
            }
        }
        return;
    }

    const QList<Edge> edges = m_graph.edges();
    for (Node *node : m_graph.nodes()) {
        if (node->supportsPreview()) {
            if (NodeItem *item = m_scene->nodeItem(node->id())) {
                item->setPreviewImage(QImage());
            }
        }
    }

    for (const QString &nodeId : order) {
        Node *node = m_graph.node(nodeId);
        if (!node) {
            continue;
        }

        QHash<QString, DataValue> inputs;
        bool missingInput = false;
        for (const Edge &edge : edges) {
            if (edge.toNode != nodeId) {
                continue;
            }
            const auto upstreamOutputs = liveOutputs.value(edge.fromNode);
            if (!upstreamOutputs.contains(edge.fromPort)) {
                missingInput = true;
                break;
            }
            inputs.insert(edge.toPort, upstreamOutputs.value(edge.fromPort));
        }
        for (const PortSpec &input : node->inputPorts()) {
            if (input.required && !inputs.contains(input.name)) {
                missingInput = true;
                break;
            }
        }
        if (missingInput) {
            continue;
        }

        const NodeResult result = node->preview(inputs);
        if (!result.ok) {
            continue;
        }
        liveOutputs.insert(nodeId, result.outputs);

        if (node->supportsPreview()) {
            NodeItem *item = m_scene->nodeItem(nodeId);
            if (item && result.outputs.contains("image")) {
                item->setPreviewImage(result.outputs.value("image").asImage());
            }
        }
    }
}

void MainWindow::appendLog(const QString &message)
{
    m_log->appendPlainText(message);
}
