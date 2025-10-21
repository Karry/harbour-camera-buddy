import QtQuick 2.6
import Sailfish.Silica 1.0

CoverBackground {
    id: cover

    Label {
        id: label
        anchors.centerIn: parent
        text: qsTr("Camera\nBuddy")
        horizontalAlignment: Text.AlignHCenter
        font.pixelSize: Theme.fontSizeLarge
    }

    CoverActionList {
        id: coverAction

        CoverAction {
            iconSource: "image://theme/icon-cover-refresh"
            onTriggered: {
                // TODO: Quick camera detection
                console.log("Cover action: detect cameras")
            }
        }
    }
}
