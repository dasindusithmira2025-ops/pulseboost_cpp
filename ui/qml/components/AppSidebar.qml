import QtQuick 2.15
import "../layout"

Sidebar {
    id: root
    property string versionText: "Version 2.5.0 Pro"
    signal navigate(string pageId)
    onScreenSelected: root.navigate(screenId)
}
