#pragma once

#include "core/NodeFactory.h"
#include "core/WorkflowExecutor.h"
#include "core/WorkflowGraph.h"

#include <QHash>
#include <QMainWindow>
#include <QStringList>

class QAction;
class QMenu;
class QToolBar;
class QToolButton;
class QTimer;
class GraphScene;
class QLabel;
class QListWidget;
class QPlainTextEdit;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void addSelectedNode();
    void onNodeSelected(const QString &nodeId);
    void runWorkflow();
    void newWorkflow();
    void openWorkflow();
    void saveWorkflow();
    void showWarning(const QString &message);
    void markDirty();
    void switchToEnglish();
    void switchToChinese();
    void retranslateUi();

protected:
    void changeEvent(QEvent *event) override;

private:
    void setupUi();
    void populateNodeList();
    void refreshDefaultNodeTitles();
    void scheduleNodePreviews();
    void updateNodePreviews();
    void clearPreviewCache();
    void clearPreviewState(const QString &nodeId);
    QString previewSignature(Node *node, const QStringList &inputSignatures) const;
    void appendLog(const QString &message);

    NodeFactory m_factory;
    WorkflowGraph m_graph;
    WorkflowExecutor m_executor;
    QString m_selectedNodeId;
    bool m_dirty = false;
    QHash<QString, QHash<QString, DataValue>> m_previewOutputs;
    QHash<QString, QString> m_previewSignatures;

    QMenu *m_languageMenu = nullptr;
    QAction *m_englishAction = nullptr;
    QAction *m_chineseAction = nullptr;
    QToolBar *m_workflowToolBar = nullptr;
    QAction *m_newAction = nullptr;
    QAction *m_openAction = nullptr;
    QAction *m_saveAction = nullptr;
    QAction *m_runAction = nullptr;
    QToolButton *m_languageButton = nullptr;
    QLabel *m_nodeLibraryLabel = nullptr;
    QListWidget *m_nodeList = nullptr;
    GraphScene *m_scene = nullptr;
    QTimer *m_previewTimer = nullptr;
    QPlainTextEdit *m_log = nullptr;
};
