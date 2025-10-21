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

            Label {
                x: Theme.horizontalPageMargin
                text: qsTr("Connect your DSLR camera via USB and download photos directly to your SailfishOS device.")
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeMedium
                wrapMode: Text.WordWrap
                width: parent.width - 2 * Theme.horizontalPageMargin
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("View Connected Cameras")
                onClicked: {
                    pageStack.push(Qt.resolvedUrl("CamerasPage.qml"))
                }
            }

            SectionHeader {
                text: qsTr("Quick Actions")
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Scan for Cameras")
                onClicked: {
                    pageStack.push(Qt.resolvedUrl("CamerasPage.qml"))
                    cameraModel.refresh()
                }
            }
        }
    }
}
