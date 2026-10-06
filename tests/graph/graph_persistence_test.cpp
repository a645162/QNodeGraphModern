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
    void loadsLegacyVersionThroughMigration();
    void persistsNodeSizeAndFixedFlag();
    void roundTripsExtendedPortDataTypes();
    void roundTripsMatrixAndColorProperties();
};

void GraphPersistenceTest::roundTripsDocumentAndProperties() {
    const auto original = createImageGraph();
    const auto encoded = QNodeGraph::Graph::GraphJson::toJson(original);
    QVERIFY(encoded.has_value());
    QCOMPARE(encoded->value(QStringLiteral("schemaVersion")).toInt(), 2);

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
    QCOMPARE(migrated->value(QStringLiteral("schemaVersion")).toInt(), 2);
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

void GraphPersistenceTest::loadsLegacyVersionThroughMigration() {
    // 旧版本文件（schemaVersion 0）必须先经 migrate 才能解析。此前 load()
    // 直接调用 fromJson()（严格校验 == kSchemaVersion），旧文件永远读不进来。
    QJsonObject legacy;
    legacy.insert(QStringLiteral("schemaVersion"), 0);
    legacy.insert(QStringLiteral("nodes"), QJsonArray{});
    legacy.insert(QStringLiteral("connections"), QJsonArray{});

    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath(QStringLiteral("legacy.json"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(QJsonDocument(legacy).toJson());
    file.close();

    const auto loaded = QNodeGraph::Graph::GraphJson::load(path);
    QVERIFY(loaded.has_value());
    // 迁移会补上缺失的 name。
    QCOMPARE(QString::fromStdString(loaded->name()), QStringLiteral("Untitled"));
    QCOMPARE(loaded->nodeCount(), std::size_t{0});
}

void GraphPersistenceTest::persistsNodeSizeAndFixedFlag() {
    // 节点尺寸与固定标记走 properties 通用袋，必须能往返。尺寸是四角 resize
    // 与连线几何的基础；fixed 决定节点可否删除。
    QNodeGraph::Core::GraphDocument document{"sizes"};
    const auto first = document.addNode("blur", "First");
    const auto second = document.addNode("image_preview", "Second");
    QVERIFY(first.has_value());
    QVERIFY(second.has_value());
    QVERIFY(document.setProperty(*first, "width", std::int64_t{400}).has_value());
    QVERIFY(document.setProperty(*first, "height", std::int64_t{240}).has_value());
    QVERIFY(document.setProperty(*second, "fixed", true).has_value());

    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath(QStringLiteral("sizes.json"));
    QVERIFY(QNodeGraph::Graph::GraphJson::save(document, path).has_value());

    const auto loaded = QNodeGraph::Graph::GraphJson::load(path);
    QVERIFY(loaded.has_value());
    const auto* loadedFirst = loaded->node(*first);
    QVERIFY(loadedFirst != nullptr);
    const auto& width = loadedFirst->properties.at("width");
    QCOMPARE(std::get<std::int64_t>(width), std::int64_t{400});
    const auto& height = loadedFirst->properties.at("height");
    QCOMPARE(std::get<std::int64_t>(height), std::int64_t{240});
    const auto* loadedSecond = loaded->node(*second);
    QVERIFY(loadedSecond != nullptr);
    QCOMPARE(std::get<bool>(loadedSecond->properties.at("fixed")), true);
}

void GraphPersistenceTest::roundTripsExtendedPortDataTypes() {
    QNodeGraph::Core::GraphDocument document{"typed"};
    const auto producer = document.addNode("producer", "Producer");
    const auto consumer = document.addNode("consumer", "Consumer");
    QVERIFY(producer.has_value() && consumer.has_value());
    const auto scalar = document.addPort(
        *producer, "value", QNodeGraph::Core::PortDirection::Output,
        QNodeGraph::Core::PortDataType::Scalar);
    const auto features = document.addPort(
        *producer, "features", QNodeGraph::Core::PortDirection::Output,
        QNodeGraph::Core::PortDataType::Features);
    const auto objects = document.addPort(
        *consumer, "objects", QNodeGraph::Core::PortDirection::Input,
        QNodeGraph::Core::PortDataType::Objects);
    QVERIFY(scalar.has_value() && features.has_value() && objects.has_value());

    const auto encoded = QNodeGraph::Graph::GraphJson::toJson(document);
    QVERIFY(encoded.has_value());
    QCOMPARE(encoded->value(QStringLiteral("schemaVersion")).toInt(), 2);
    const auto loaded = QNodeGraph::Graph::GraphJson::fromJson(*encoded);
    QVERIFY(loaded.has_value());
    QCOMPARE(loaded->port(*scalar)->dataType,
             QNodeGraph::Core::PortDataType::Scalar);
    QCOMPARE(loaded->port(*features)->dataType,
             QNodeGraph::Core::PortDataType::Features);
    QCOMPARE(loaded->port(*objects)->dataType,
             QNodeGraph::Core::PortDataType::Objects);

    QJsonObject versionOne = *encoded;
    versionOne.insert(QStringLiteral("schemaVersion"), 1);
    const auto migrated = QNodeGraph::Graph::GraphJson::migrate(versionOne);
    QVERIFY(migrated.has_value());
    QCOMPARE(migrated->value(QStringLiteral("schemaVersion")).toInt(), 2);
    QVERIFY(QNodeGraph::Graph::GraphJson::fromJson(*migrated).has_value());
}

void GraphPersistenceTest::roundTripsMatrixAndColorProperties() {
    QNodeGraph::Core::GraphDocument document{"matrix-color"};
    const auto node = document.addNode("op.convolution", "Convolution");
    QVERIFY(node.has_value());
    const std::string matrix = "-1,0,1;-2,0,2;-1,0,1";
    const std::string color = "#1a2b3c";
    QVERIFY(document.setProperty(*node, "kernel", matrix).has_value());
    QVERIFY(document.setProperty(*node, "borderValue", color).has_value());

    const auto encoded = QNodeGraph::Graph::GraphJson::toJson(document);
    QVERIFY(encoded.has_value());
    const auto loaded = QNodeGraph::Graph::GraphJson::fromJson(*encoded);
    QVERIFY(loaded.has_value());
    const auto* restored = loaded->node(*node);
    QVERIFY(restored != nullptr);
    QCOMPARE(std::get<std::string>(restored->properties.at("kernel")),
             matrix);
    QCOMPARE(std::get<std::string>(restored->properties.at("borderValue")),
             color);
}

QTEST_MAIN(GraphPersistenceTest)
#include "graph_persistence_test.moc"
