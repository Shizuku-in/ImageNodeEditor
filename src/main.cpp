#include "core/NodeFactory.h"
#include "core/WorkflowExecutor.h"
#include "core/WorkflowGraph.h"
#include "gui/MainWindow.h"
#include "i18n/LanguageManager.h"
#include "io/WorkflowSerializer.h"
#include "nodes/ImageNodes.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QFont>
#include <QGuiApplication>
#include <QSettings>
#include <QTextStream>

namespace {

QString requestedLanguage(const QCoreApplication &app)
{
    QCommandLineParser parser;
    parser.addOption(QCommandLineOption("lang", "Language code, for example en_US or zh_CN.", "locale"));
    parser.addOption(QCommandLineOption("workflow", "Workflow JSON path.", "workflow"));
    parser.addOption(QCommandLineOption("no-gui", "Run without the GUI."));
    parser.parse(app.arguments());

    const QString commandLineLanguage = parser.value("lang");
    if (!commandLineLanguage.isEmpty()) {
        return commandLineLanguage;
    }
    const QString savedLanguage = QSettings().value("language").toString();
    if (!savedLanguage.isEmpty()) {
        return savedLanguage;
    }
    return LanguageManager::instance().defaultLanguage();
}

bool hasNoGuiArgument(int argc, char *argv[])
{
    for (int i = 1; i < argc; ++i) {
        if (QString::fromLocal8Bit(argv[i]) == "--no-gui") {
            return true;
        }
    }
    return false;
}

int runCli(QCoreApplication &app)
{
    QCommandLineParser parser;
    parser.setApplicationDescription(QCoreApplication::translate("App", "ImageNodeEditor command line runner"));
    parser.addHelpOption();
    const QCommandLineOption workflowOption("workflow", QCoreApplication::translate("App", "Workflow JSON path."), "workflow");
    const QCommandLineOption noGuiOption("no-gui", QCoreApplication::translate("App", "Run without the GUI."));
    const QCommandLineOption langOption("lang", QCoreApplication::translate("App", "Language code, for example en_US or zh_CN."), "locale");
    parser.addOption(workflowOption);
    parser.addOption(noGuiOption);
    parser.addOption(langOption);
    parser.process(app);

    QTextStream out(stdout);
    QTextStream err(stderr);
    const QString workflowPath = parser.value(workflowOption);
    if (workflowPath.isEmpty()) {
        err << QCoreApplication::translate("App", "Missing --workflow workflow.json") << "\n";
        return 2;
    }

    NodeFactory factory;
    registerImageNodes(factory);

    WorkflowGraph graph;
    QString error;
    if (!WorkflowSerializer::loadFromFile(workflowPath, factory, &graph, &error)) {
        err << QCoreApplication::translate("App", "Failed to load workflow: %1").arg(error) << "\n";
        return 1;
    }

    WorkflowExecutor executor;
    const ExecutionReport report = executor.execute(graph);
    for (const QString &message : report.messages) {
        out << message << "\n";
    }
    if (!report.ok) {
        err << QCoreApplication::translate("App", "Workflow failed") << "\n";
        return 1;
    }
    out << QCoreApplication::translate("App", "Workflow executed successfully") << "\n";
    return 0;
}

} // namespace

int main(int argc, char *argv[])
{
    if (hasNoGuiArgument(argc, argv)) {
        QGuiApplication app(argc, argv);
        QGuiApplication::setOrganizationName("ImageNodeEditor");
        QGuiApplication::setApplicationName("ImageNodeEditor");
        QGuiApplication::setApplicationVersion("1.0");
        LanguageManager::instance().setLanguage(requestedLanguage(app));
        return runCli(app);
    }

    QApplication app(argc, argv);
    app.setFont(QFont("Microsoft YaHei UI"));
    QApplication::setOrganizationName("ImageNodeEditor");
    QApplication::setApplicationName("ImageNodeEditor");
    QApplication::setApplicationVersion("1.0");
    LanguageManager::instance().setLanguage(requestedLanguage(app));
    MainWindow window;
    window.show();
    return app.exec();
}
