#include <QNodeGraph/Lib/Graph/graph_json.h>

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonValue>
#include <QVariant>

#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <utility>

namespace QNodeGraph::Graph {

namespace {

Core::GraphError error(Core::GraphErrorCode code, const QString& message) {
    return Core::GraphError{code, message.toStdString()};
}

QString directionName(Core::PortDirection direction) {
    return direction == Core::PortDirection::Input ? QStringLiteral("input")
                                                    : QStringLiteral("output");
}

std::optional<Core::PortDirection> parseDirection(const QString& value) {
    if (value == QStringLiteral("input")) {
        return Core::PortDirection::Input;
    }
    if (value == QStringLiteral("output")) {
        return Core::PortDirection::Output;
    }
    return std::nullopt;
}

QString dataTypeName(Core::PortDataType dataType) {
    switch (dataType) {
    case Core::PortDataType::Any:
        return QStringLiteral("any");
    case Core::PortDataType::Image:
        return QStringLiteral("image");
    case Core::PortDataType::Number:
        return QStringLiteral("number");
    case Core::PortDataType::Text:
        return QStringLiteral("text");
    case Core::PortDataType::Boolean:
        return QStringLiteral("boolean");
    }
    return {};
}

std::optional<Core::PortDataType> parseDataType(const QString& value) {
    if (value == QStringLiteral("any")) {
        return Core::PortDataType::Any;
    }
    if (value == QStringLiteral("image")) {
        return Core::PortDataType::Image;
    }
    if (value == QStringLiteral("number")) {
        return Core::PortDataType::Number;
    }
    if (value == QStringLiteral("text")) {
        return Core::PortDataType::Text;
    }
    if (value == QStringLiteral("boolean")) {
        return Core::PortDataType::Boolean;
    }
    return std::nullopt;
}

QJsonValue idValue(std::uint64_t id) {
    return QJsonValue::fromVariant(
        QVariant::fromValue<qulonglong>(static_cast<qulonglong>(id)));
}

bool parseId(const QJsonValue& value, std::uint64_t& id) {
    bool ok = false;
    qulonglong parsed = 0;
    if (value.isString()) {
        parsed = value.toString().toULongLong(&ok);
    } else if (value.isDouble()) {
        const auto number = value.toDouble();
        if (number >= 0.0 && std::isfinite(number) &&
            number <= static_cast<double>(std::numeric_limits<qulonglong>::max())) {
            parsed = static_cast<qulonglong>(number);
            ok = std::floor(number) == number;
        }
    }
    if (ok) {
        id = static_cast<std::uint64_t>(parsed);
    }
    return ok;
}

bool parsePosition(const QJsonObject& object, Core::Point& position) {
    if (!object.contains(QStringLiteral("position"))) {
        return true;
    }
    const auto value = object.value(QStringLiteral("position"));
    if (!value.isObject()) {
        return false;
    }
    const auto point = value.toObject();
    if (!point.value(QStringLiteral("x")).isDouble() ||
        !point.value(QStringLiteral("y")).isDouble()) {
        return false;
    }
    position.x = point.value(QStringLiteral("x")).toDouble();
    position.y = point.value(QStringLiteral("y")).toDouble();
    return true;
}

QJsonObject propertyValue(const Core::PropertyValue& value) {
    QJsonObject object;
    std::visit(
        [&object](const auto& typed) {
            using Value = std::decay_t<decltype(typed)>;
            if constexpr (std::is_same_v<Value, bool>) {
                object.insert(QStringLiteral("type"), QStringLiteral("bool"));
                object.insert(QStringLiteral("value"), typed);
            } else if constexpr (std::is_same_v<Value, double>) {
                object.insert(QStringLiteral("type"), QStringLiteral("double"));
                object.insert(QStringLiteral("value"), typed);
            } else if constexpr (std::is_same_v<Value, std::int64_t>) {
                object.insert(QStringLiteral("type"), QStringLiteral("int64"));
                object.insert(QStringLiteral("value"),
                              QString::number(typed));
            } else if constexpr (std::is_same_v<Value, std::string>) {
                object.insert(QStringLiteral("type"), QStringLiteral("string"));
                object.insert(QStringLiteral("value"),
                              QString::fromUtf8(typed.data(),
                                                static_cast<qsizetype>(typed.size())));
            }
        },
        value);
    return object;
}

Core::GraphResult<Core::PropertyValue> parsePropertyValue(
    const QJsonObject& object) {
    const auto type = object.value(QStringLiteral("type"));
    const auto value = object.value(QStringLiteral("value"));
    if (!type.isString()) {
        return std::unexpected(error(Core::GraphErrorCode::InvalidDocument,
                                      "A property is missing its type."));
    }
    const auto typeName = type.toString();
    if (typeName == QStringLiteral("bool") && value.isBool()) {
        return value.toBool();
    }
    if (typeName == QStringLiteral("double") && value.isDouble()) {
        return value.toDouble();
    }
    if (typeName == QStringLiteral("int64")) {
        bool ok = false;
        const auto parsed = value.isString()
                                ? value.toString().toLongLong(&ok)
                                : value.toVariant().toLongLong(&ok);
        if (ok) {
            return static_cast<std::int64_t>(parsed);
        }
    }
    if (typeName == QStringLiteral("string") && value.isString()) {
        return value.toString().toStdString();
    }
    return std::unexpected(error(
        Core::GraphErrorCode::InvalidDocument,
        QStringLiteral("Property value does not match type %1.").arg(typeName)));
}

} // namespace

Core::GraphResult<QJsonObject> GraphJson::toJson(
    const Core::GraphDocument& document) {
    QJsonObject root;
    root.insert(QStringLiteral("schemaVersion"), kSchemaVersion);
    root.insert(QStringLiteral("name"), QString::fromStdString(document.name()));

    QJsonArray nodes;
    for (const auto nodeId : document.nodeIds()) {
        const auto* node = document.node(nodeId);
        if (node == nullptr) {
            continue;
        }
        QJsonObject object;
        object.insert(QStringLiteral("id"), idValue(node->id));
        object.insert(QStringLiteral("type"), QString::fromStdString(node->type));
        object.insert(QStringLiteral("name"), QString::fromStdString(node->name));
        object.insert(QStringLiteral("position"),
                      QJsonObject{{QStringLiteral("x"), node->position.x},
                                  {QStringLiteral("y"), node->position.y}});

        QJsonObject properties;
        for (const auto& [name, value] : node->properties) {
            properties.insert(QString::fromStdString(name), propertyValue(value));
        }
        object.insert(QStringLiteral("properties"), properties);

        QJsonArray ports;
        for (const auto portId : node->ports) {
            const auto* port = document.port(portId);
            if (port == nullptr) {
                continue;
            }
            ports.append(QJsonObject{
                {QStringLiteral("id"), idValue(port->id)},
                {QStringLiteral("name"), QString::fromStdString(port->name)},
                {QStringLiteral("direction"), directionName(port->direction)},
                {QStringLiteral("dataType"), dataTypeName(port->dataType)},
                {QStringLiteral("acceptsMultipleConnections"),
                 port->acceptsMultipleConnections}});
        }
        object.insert(QStringLiteral("ports"), ports);
        nodes.append(object);
    }
    root.insert(QStringLiteral("nodes"), nodes);

    QJsonArray connections;
    for (const auto& connection : document.connections()) {
        connections.append(QJsonObject{
            {QStringLiteral("outputPort"), idValue(connection.outputPort)},
            {QStringLiteral("inputPort"), idValue(connection.inputPort)}});
    }
    root.insert(QStringLiteral("connections"), connections);
    return root;
}

Core::GraphResult<QJsonObject> GraphJson::migrate(const QJsonObject& object) {
    const auto schemaVersion = object.value(QStringLiteral("schemaVersion"));
    if (!schemaVersion.isDouble()) {
        return std::unexpected(error(Core::GraphErrorCode::InvalidDocument,
                                      "The document has no schema version."));
    }
    const auto version = schemaVersion.toInt();
    if (version == kSchemaVersion) {
        return object;
    }
    if (version != 0) {
        return std::unexpected(error(
            Core::GraphErrorCode::UnsupportedSchemaVersion,
            QStringLiteral("Unsupported graph schema version %1.").arg(version)));
    }

    auto migrated = object;
    migrated.insert(QStringLiteral("schemaVersion"), kSchemaVersion);
    if (!migrated.value(QStringLiteral("name")).isString()) {
        migrated.insert(QStringLiteral("name"), QStringLiteral("Untitled"));
    }
    if (!migrated.value(QStringLiteral("nodes")).isArray()) {
        migrated.insert(QStringLiteral("nodes"), QJsonArray{});
    }
    if (!migrated.value(QStringLiteral("connections")).isArray()) {
        migrated.insert(QStringLiteral("connections"), QJsonArray{});
    }
    return migrated;
}

Core::GraphResult<Core::GraphDocument> GraphJson::fromJson(
    const QJsonObject& object) {
    const auto schemaVersion = object.value(QStringLiteral("schemaVersion"));
    if (!schemaVersion.isDouble()) {
        return std::unexpected(error(Core::GraphErrorCode::InvalidDocument,
                                      "The document has no schema version."));
    }
    if (schemaVersion.toInt() != kSchemaVersion) {
        return std::unexpected(error(
            Core::GraphErrorCode::UnsupportedSchemaVersion,
            QStringLiteral("Unsupported graph schema version %1.")
                .arg(schemaVersion.toInt())));
    }
    const auto nodesValue = object.value(QStringLiteral("nodes"));
    const auto connectionsValue = object.value(QStringLiteral("connections"));
    if (!nodesValue.isArray() || !connectionsValue.isArray()) {
        return std::unexpected(error(Core::GraphErrorCode::InvalidDocument,
                                      "The document nodes or connections are invalid."));
    }

    const auto name = object.value(QStringLiteral("name"));
    if (!name.isString()) {
        return std::unexpected(error(Core::GraphErrorCode::InvalidDocument,
                                      "The document name is invalid."));
    }
    Core::GraphDocument document(name.toString().toStdString());
    for (const auto& value : nodesValue.toArray()) {
        if (!value.isObject()) {
            return std::unexpected(error(Core::GraphErrorCode::InvalidDocument,
                                          "A node entry is not an object."));
        }
        const auto nodeObject = value.toObject();
        std::uint64_t nodeId = 0;
        const auto type = nodeObject.value(QStringLiteral("type"));
        const auto nodeName = nodeObject.value(QStringLiteral("name"));
        if (!parseId(nodeObject.value(QStringLiteral("id")), nodeId) ||
            !type.isString() || !nodeName.isString()) {
            return std::unexpected(error(
                Core::GraphErrorCode::InvalidDocument,
                "A node is missing a valid id, type, or name."));
        }
        const auto node = document.addNode(type.toString().toStdString(),
                                           nodeName.toString().toStdString(),
                                           nodeId);
        if (!node) {
            return std::unexpected(node.error());
        }

        Core::Point position;
        if (!parsePosition(nodeObject, position)) {
            return std::unexpected(error(Core::GraphErrorCode::InvalidDocument,
                                          "A node position is invalid."));
        }
        if (const auto result = document.setNodePosition(*node, position);
            !result) {
            return std::unexpected(result.error());
        }

        const auto propertiesValue = nodeObject.value(QStringLiteral("properties"));
        if (!propertiesValue.isUndefined() && !propertiesValue.isObject()) {
            return std::unexpected(error(Core::GraphErrorCode::InvalidDocument,
                                          "A node properties value is invalid."));
        }
        const auto properties = propertiesValue.toObject();
        for (auto iterator = properties.begin(); iterator != properties.end();
             ++iterator) {
            if (!iterator.value().isObject()) {
                return std::unexpected(error(
                    Core::GraphErrorCode::InvalidDocument,
                    "A node property entry is not an object."));
            }
            const auto property = parsePropertyValue(iterator.value().toObject());
            if (!property) {
                return std::unexpected(property.error());
            }
            const auto result = document.setProperty(
                *node, iterator.key().toStdString(), *property);
            if (!result) {
                return std::unexpected(result.error());
            }
        }

        const auto portsValue = nodeObject.value(QStringLiteral("ports"));
        if (!portsValue.isArray()) {
            return std::unexpected(error(Core::GraphErrorCode::InvalidDocument,
                                          "A node ports value is invalid."));
        }
        for (const auto& portValue : portsValue.toArray()) {
            if (!portValue.isObject()) {
                return std::unexpected(error(Core::GraphErrorCode::InvalidDocument,
                                              "A port entry is not an object."));
            }
            const auto portObject = portValue.toObject();
            std::uint64_t portId = 0;
            const auto portName = portObject.value(QStringLiteral("name"));
            const auto direction = parseDirection(
                portObject.value(QStringLiteral("direction")).toString());
            const auto dataType = parseDataType(
                portObject.value(QStringLiteral("dataType")).toString());
            if (!parseId(portObject.value(QStringLiteral("id")), portId) ||
                !portName.isString() || !direction.has_value() ||
                !dataType.has_value()) {
                return std::unexpected(error(Core::GraphErrorCode::InvalidDocument,
                                              "A port entry is invalid."));
            }
            const auto port = document.addPort(
                *node, portName.toString().toStdString(), *direction, *dataType,
                portObject.value(QStringLiteral("acceptsMultipleConnections"))
                    .toBool(false),
                portId);
            if (!port) {
                return std::unexpected(port.error());
            }
        }
    }

    for (const auto& value : connectionsValue.toArray()) {
        if (!value.isObject()) {
            return std::unexpected(error(Core::GraphErrorCode::InvalidDocument,
                                          "A connection entry is not an object."));
        }
        const auto connection = value.toObject();
        std::uint64_t outputPort = 0;
        std::uint64_t inputPort = 0;
        if (!parseId(connection.value(QStringLiteral("outputPort")), outputPort) ||
            !parseId(connection.value(QStringLiteral("inputPort")), inputPort)) {
            return std::unexpected(error(Core::GraphErrorCode::InvalidDocument,
                                          "A connection entry is invalid."));
        }
        const auto result = document.connect(outputPort, inputPort);
        if (!result) {
            return std::unexpected(result.error());
        }
    }
    return document;
}

Core::GraphResult<void> GraphJson::validateExternalAssets(
    const Core::GraphDocument& document, const QString& baseDirectory) {
    const QDir base(baseDirectory);
    for (const auto nodeId : document.nodeIds()) {
        const auto* node = document.node(nodeId);
        if (node == nullptr) {
            continue;
        }
        for (const auto& [name, value] : node->properties) {
            if (name != "path" && name != "assetPath") {
                continue;
            }
            if (!std::holds_alternative<std::string>(value)) {
                continue;
            }
            const auto path = QString::fromStdString(
                std::get<std::string>(value));
            if (path.isEmpty()) {
                continue;
            }
            const QFileInfo fileInfo(QDir::isAbsolutePath(path)
                                         ? path
                                         : base.filePath(path));
            if (!fileInfo.exists() || !fileInfo.isFile()) {
                return std::unexpected(error(
                    Core::GraphErrorCode::ExternalAssetMissing,
                    QStringLiteral("External asset is missing: %1")
                        .arg(fileInfo.filePath())));
            }
        }
    }
    return {};
}

Core::GraphResult<void> GraphJson::save(const Core::GraphDocument& document,
                                        const QString& filePath) {
    const auto object = toJson(document);
    if (!object) {
        return std::unexpected(object.error());
    }
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return std::unexpected(error(Core::GraphErrorCode::SerializationError,
                                      file.errorString()));
    }
    const auto bytes = QJsonDocument(*object).toJson(QJsonDocument::Indented);
    if (file.write(bytes) != bytes.size()) {
        return std::unexpected(error(Core::GraphErrorCode::SerializationError,
                                      file.errorString()));
    }
    return {};
}

Core::GraphResult<Core::GraphDocument> GraphJson::load(
    const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return std::unexpected(error(Core::GraphErrorCode::SerializationError,
                                      file.errorString()));
    }
    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (!document.isObject()) {
        return std::unexpected(error(
            Core::GraphErrorCode::SerializationError,
            QStringLiteral("Unable to parse graph JSON: %1")
                .arg(parseError.errorString())));
    }
    return fromJson(document.object());
}

} // namespace QNodeGraph::Graph
