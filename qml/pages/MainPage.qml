import QtQuick 2.6
import Sailfish.Silica 1.0

Page {
    id: mainPage

    allowedOrientations: Orientation.All

    SilicaFlickable {
        anchors.fill: parent

        PullDownMenu {
            MenuItem {
                text: qsTr("Settings")
                onClicked: pageStack.push(Qt.resolvedUrl("SettingsPage.qml"))
            }
        }

        contentHeight: column.height

        Column {
            id: column

            width: mainPage.width
            spacing: Theme.paddingLarge

            PageHeader {
                title: qsTr("Camera Buddy")
            }

            Label {
                x: Theme.horizontalPageMargin
                text: qsTr("Welcome to Camera Buddy!")
                color: Theme.secondaryHighlightColor
                font.pixelSize: Theme.fontSizeExtraLarge
                wrapMode: Text.WordWrap
                width: parent.width - 2 * Theme.horizontalPageMargin
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Detect Cameras")
                onClicked: {
                    // TODO: Implement camera detection
                    console.log("Camera detection clicked")
                }
            }

            SectionHeader {
                text: qsTr("Connected Cameras")
            }

            Label {
                x: Theme.horizontalPageMargin
                text: qsTr("No cameras detected")
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeMedium
                width: parent.width - 2 * Theme.horizontalPageMargin
            }
        }
    }
}
