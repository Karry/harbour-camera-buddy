import QtQuick 2.6
import Sailfish.Silica 1.0

ApplicationWindow
{
    id: appWindow

    initialPage: Qt.resolvedUrl("pages/CamerasPage.qml")
    cover: Qt.resolvedUrl("pages/Cover.qml")
    allowedOrientations: defaultAllowedOrientations
}
