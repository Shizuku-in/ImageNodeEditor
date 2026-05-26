#include "core/NodeFactory.h"
#include "core/WorkflowGraph.h"
#include "imageops/ImageOps.h"
#include "io/WorkflowSerializer.h"
#include "nodes/ImageNodes.h"

#include <QtTest/QtTest>

class CoreTests : public QObject
{
    Q_OBJECT

private slots:
    void rejectsIncompatiblePorts()
    {
        NodeFactory factory;
        registerImageNodes(factory);
        WorkflowGraph graph;
        QVERIFY(graph.addNode(factory.create("ImageInput", "input")));
        QVERIFY(graph.addNode(factory.create("ThresholdMask", "mask")));
        QVERIFY(graph.addNode(factory.create("Display", "display")));

        QString error;
        QVERIFY(graph.addEdge({"input", "image", "mask", "image"}, &error));
        QVERIFY(!graph.addEdge({"mask", "mask", "display", "image"}, &error));
        QVERIFY(error.contains("Mask cannot connect to Image"));
    }

    void rejectsCycles()
    {
        NodeFactory factory;
        registerImageNodes(factory);
        WorkflowGraph graph;
        QVERIFY(graph.addNode(factory.create("Blur", "a")));
        QVERIFY(graph.addNode(factory.create("Sharpen", "b")));

        QString error;
        QVERIFY(graph.addEdge({"a", "image", "b", "image"}, &error));
        QVERIFY(!graph.addEdge({"b", "image", "a", "image"}, &error));
        QVERIFY(error.contains("cycle"));
    }

    void rejectsUnknownNodeInJson()
    {
        NodeFactory factory;
        registerImageNodes(factory);
        WorkflowGraph graph;
        QJsonObject root;
        QJsonArray nodes;
        nodes.append(QJsonObject{{"id", "n1"}, {"type", "NoSuchNode"}});
        root["nodes"] = nodes;
        root["edges"] = QJsonArray();

        QString error;
        QVERIFY(!WorkflowSerializer::fromJson(root, factory, &graph, &error));
        QVERIFY(error.contains("Unknown node type"));
    }

    void imageOpsBrightnessAndMask()
    {
        QImage image(2, 1, QImage::Format_ARGB32);
        image.setPixelColor(0, 0, QColor(100, 100, 100));
        image.setPixelColor(1, 0, QColor(200, 200, 200));

        const QImage bright = imageops::brightnessContrast(image, 20, 1.0);
        QCOMPARE(bright.pixelColor(0, 0).red(), 120);

        const QImage mask = imageops::thresholdMask(image, 150, false);
        QCOMPARE(qGray(mask.pixel(0, 0)), 0);
        QCOMPARE(qGray(mask.pixel(1, 0)), 255);
    }
};

QTEST_MAIN(CoreTests)

#include "core_tests.moc"
