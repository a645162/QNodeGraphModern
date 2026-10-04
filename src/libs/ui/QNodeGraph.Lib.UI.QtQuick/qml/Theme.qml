pragma Singleton

import QtQuick

QtObject {
    id: theme

    readonly property int system: 0
    readonly property int light: 1
    readonly property int darkMode: 2

    property int mode: 0

    readonly property bool dark: mode === 2
                                 ? true
                                 : (mode === 1
                                    ? false
                                    : Application.styleHints.colorScheme ===
                                      Qt.Dark)

    // Surfaces
    readonly property color windowBg: dark ? "#161a1f" : "#eef1f5"
    readonly property color panelBg: dark ? "#1e242b" : "#ffffff"
    readonly property color panelBorder: dark ? "#2b333c" : "#d7dde5"
    readonly property color fieldBg: dark ? "#161a1f" : "#f4f6f9"
    readonly property color gridLine: dark ? "#232a32" : "#dfe4ea"
    readonly property color canvasBg: dark ? "#191d22" : "#f5f7fa"

    // Nodes
    readonly property color nodeBg: dark ? "#262d36" : "#ffffff"
    readonly property color nodeBgSelected: dark ? "#2f3b49" : "#e8f1fb"
    readonly property color nodeBorder: dark ? "#333c46" : "#d7dde5"
    readonly property color nodeBorderSelected: dark ? "#4f9ee8" : "#2f80d8"
    readonly property color nodeHeaderText: dark ? "#eef3f8" : "#1c2530"
    readonly property color nodeBodyText: dark ? "#b9c3ce" : "#4a5563"
    readonly property color nodeMutedText: dark ? "#7d8894" : "#8a95a3"

    // Accents / ports / wires
    readonly property color accent: dark ? "#4f9ee8" : "#2f80d8"
    readonly property color portInput: dark ? "#4f9ee8" : "#2f80d8"
    readonly property color portOutput: dark ? "#57b98c" : "#2f9e69"
    readonly property color portActive: "#f0b45f"
    readonly property color wire: dark ? "#7d9bb8" : "#9aa9ba"
    readonly property color wireActive: "#f0b45f"
    readonly property color error: dark ? "#ef9a9a" : "#d64545"

    // Text
    readonly property color textPrimary: dark ? "#eef3f8" : "#1c2530"
    readonly property color textSecondary: dark ? "#aab4bf" : "#5b6673"

    // Geometry
    readonly property real radius: 6
    readonly property real radiusSmall: 4
    readonly property real nodeRadius: 7
    readonly property real portRadius: 5
    readonly property real borderWidth: 1
    readonly property real spacing: 8
}
