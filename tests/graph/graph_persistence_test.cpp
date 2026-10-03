#include <QNodeGraph/Lib/Graph/graph_json.h>
#include <QNodeGraph/Lib/Graph/node_registry.h>

#include <QJsonObject>
#include <QFile>
#include <QDir>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <cstdint>
#include <string>

namespace {

QNodeGraph::Core::GraphDocument createImageGraph() {
    QNodeGraph::Core::GraphDocument document("Image graph");
    const auto source = document.addNode("load_image", "Load", 7);
    const auto filter = document.addNode("grayscale", "Gray", 9);
    const auto output = document.addPort(
        *source, "image", QNodeGraph::Core::PortDirection::Output,
        QNodeGraph::Core::PortDataType::Image, false, 11);
    const auto input = document.addPort(
        *filter, "image", QNodeGraph::Core::PortDirection::Input,
        QNodeGraph::Core::PortDataType::Image, false, 12);
    document.setNodePosition(*source, {32.5, -18.0});
    document.setNodePosition(*filter, {360.0, 42.0});
    document.setProperty(*source, "path", std::string("assets/input.png"));
    document.setProperty(*filter, "enabled", true);
    document.setProperty(*filter, "strength", 0.75);
    document.setProperty(*filter, "iterations", std::int64_t{3});
    document.connect(*output, *input);
    return document;
}

} // namespace

class GraphPersistenceTest final : public QObject {
    Q_OBJECT

private slots:
    void roundTripsDocumentAndProperties();
    void savesAndLoadsJsonFile();
    void rejectsUnsupportedSchemaVersion();
    void reportsMissingJsonFile();
    void registersAndInstantiatesBuiltInImageNodes();
    void migratesInitialSchemaAndReportsMissingExternalAssets();
};

void GraphPersistenceTest::roundTripsDocumentAndProperties() {
    const auto original = createImageGraph();
    const auto encoded = QNodeGraph::Graph::GraphJson::toJson(original);
    QVERIFY(encoded.has_value());
    QCOMPARE(encoded->value(QStringLiteral("schemaVersion")).toInt(), 1);

    const auto restored = QNodeGraph::Graph::GraphJson::fromJson(*encoded);
    QVERIFY(restored.has_value());
    QCOMPARE(restored->name(), std::string("Image graph"));
    QCOMPARE(restored->nodeCount(), std::size_t{2});
    QCOMPARE(restored->connectionCount(), std::size_t{1});
    QCOMPARE(restored->node(7)->position.x, 32.5);
    QCOMPARE(restored->node(9)->position.y, 42.0);
    QCOMPARE(std::get<std::string>(restored->node(7)->properties.at("path")),
             std::string("assets/input.png"));
    QCOMPARE(std::get<bool>(restored->node(9)->properties.at("enabled")),
             true);
    QCOMPARE(std::get<double>(restored->node(9)->properties.at("strength")),
             0.75);
    QCOMPARE(std::get<std::int64_t>(
                 restored->node(9)->properties.at("iterations")),
             std::int64_t{3});
}

void GraphPersistenceTest::savesAndLoadsJsonFile() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath(QStringLiteral("graph.qng.json"));
    const auto original = createImageGraph();
    QVERIFY(QNodeGraph::Graph::GraphJson::save(original, path).has_value());

    const auto loaded = QNodeGraph::Graph::GraphJson::load(path);
    QVERIFY(loaded.has_value());
    QCOMPARE(loaded->name(), std::string("Image graph"));
    QCOMPARE(loaded->connectionCount(), std::size_t{1});
}

void GraphPersistenceTest::rejectsUnsupportedSchemaVersion() {
    QJsonObject object;
    object.insert(QStringLiteral("schemaVersion"), 99);
    object.insert(QStringLiteral("name"), QStringLiteral("Future"));
    object.insert(QStringLiteral("nodes"), QJsonArray{});
    object.insert(QStringLiteral("connections"), QJsonArray{});

    const auto result = QNodeGraph::Graph::GraphJson::fromJson(object);
    QVERIFY(!result.has_value());
    QCOMPARE(result.error().code,
             QNodeGraph::Core::GraphErrorCode::UnsupportedSchemaVersion);
}

void GraphPersistenceTest::reportsMissingJsonFile() {
    const auto result = QNodeGraph::Graph::GraphJson::load(
        QStringLiteral("this-file-does-not-exist.qng.json"));
    QVERIFY(!result.has_value());
    QCOMPARE(result.error().code,
             QNodeGraph::Core::GraphErrorCode::SerializationError);
}

void GraphPersistenceTest::registersAndInstantiatesBuiltInImageNodes() {
    const auto registry = QNodeGraph::Graph::NodeRegistry::withBuiltins();
    QVERIFY(registry.find("load_image") != nullptr);
    QVERIFY(registry.find("grayscale") != nullptr);
    QVERIFY(registry.find("edge_detect") != nullptr);

    QNodeGraph::Core::GraphDocument document;
    const auto node = registry.createNode(document, "grayscale", "Gray",
                                           {120.0, 80.0});
    QVERIFY(node.has_value());
    QCOMPARE(document.node(*node)->type, std::string("grayscale"));
    QCOMPARE(document.node(*node)->ports.size(), std::size_t{2});
    QCOMPARE(document.node(*node)->position.x, 120.0);

    const auto unknown = registry.createNode(document, "missing", "Missing");
    QVERIFY(!unknown.has_value());
    QCOMPARE(unknown.error().code,
             QNodeGraph::Core::GraphErrorCode::UnknownNodeType);
}

void GraphPersistenceTest::migratesInitialSchemaAndReportsMissingExternalAssets() {
    QJsonObject legacy{
        {QStringLiteral("schemaVersion"), 0},
        {QStringLiteral("name"), QStringLiteral("Legacy")},
        {QStringLiteral("nodes"), QJsonArray{}},
        {QStringLiteral("connections"), QJsonArray{}}};
    const auto migrated = QNodeGraph::Graph::GraphJson::migrate(legacy);
    QVERIFY(migrated.has_value());
    QCOMPARE(migrated->value(QStringLiteral("schemaVersion")).toInt(), 1);
    QVERIFY(QNodeGraph::Graph::GraphJson::fromJson(*migrated).has_value());

    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto missing =
        QNodeGraph::Graph::GraphJson::validateExternalAssets(
            createImageGraph(), directory.path());
    QVERIFY(!missing.has_value());
    QCOMPARE(missing.error().code,
             QNodeGraph::Core::GraphErrorCode::ExternalAssetMissing);
}

QTEST_MAIN(GraphPersistenceTest)
#include "graph_persistence_test.moc"
