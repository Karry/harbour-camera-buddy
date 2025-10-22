import QtQuick 2.6
import Sailfish.Silica 1.0

Page {
    id: camerasPage

    allowedOrientations: Orientation.All

    SilicaFlickable {
        anchors.fill: parent

        PullDownMenu {
            MenuItem {
                text: qsTr("Refresh")
                onClicked: cameraModel.refresh()
            }
        }

        contentHeight: column.height

        Column {
            id: column
            width: camerasPage.width
            spacing: Theme.paddingLarge

            PageHeader {
                title: qsTr("Connected Cameras")
            }

            // Scanning indicator
            BusyIndicator {
                id: scanningIndicator
                anchors.horizontalCenter: parent.horizontalCenter
                running: cameraModel.scanning
                visible: running
                size: BusyIndicatorSize.Medium
            }

            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Scanning for cameras...")
                visible: cameraModel.scanning
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeMedium
            }

            // Camera list
            SilicaListView {
                id: cameraListView
                width: parent.width
                height: contentHeight
                visible: !cameraModel.scanning
                model: cameraModel

                delegate: ListItem {
                    id: cameraItem
                    contentHeight: Theme.itemSizeMedium

                    Rectangle {
                        anchors.fill: parent
                        color: model.connected ? Theme.rgba(Theme.highlightBackgroundColor, 0.1) : "transparent"
                        radius: Theme.paddingSmall
                    }

                    Column {
                        anchors {
                            left: parent.left
                            right: statusColumn.left
                            leftMargin: Theme.horizontalPageMargin
                            rightMargin: Theme.paddingMedium
                            verticalCenter: parent.verticalCenter
                        }

                        Label {
                            text: model.name || qsTr("Unknown Camera")
                            color: model.connected ? Theme.primaryColor : Theme.secondaryColor
                            font.pixelSize: Theme.fontSizeMedium
                            truncationMode: TruncationMode.Fade
                            width: parent.width
                        }

                        Label {
                            text: qsTr("Model: %1").arg(model.cameraModel || qsTr("Unknown"))
                            color: Theme.secondaryColor
                            font.pixelSize: Theme.fontSizeSmall
                            truncationMode: TruncationMode.Fade
                            width: parent.width
                            visible: (model.cameraModel || "").length > 0
                        }

                        Label {
                            text: qsTr("Port: %1").arg(model.port || qsTr("Unknown"))
                            color: Theme.secondaryColor
                            font.pixelSize: Theme.fontSizeSmall
                            truncationMode: TruncationMode.Fade
                            width: parent.width
                            visible: (model.port || "").length > 0
                        }

                        Label {
                            text: qsTr("Serial: %1").arg(model.serialNumber || qsTr("Unknown"))
                            color: Theme.secondaryColor
                            font.pixelSize: Theme.fontSizeSmall
                            truncationMode: TruncationMode.Fade
                            width: parent.width
                            visible: (model.serialNumber || "").length > 0
                        }
                    }

                    Column {
                        id: statusColumn
                        anchors {
                            right: parent.right
                            rightMargin: Theme.horizontalPageMargin
                            verticalCenter: parent.verticalCenter
                        }

                        Label {
                            text: model.connected ? qsTr("Connected") : qsTr("Disconnected")
                            color: model.connected ? Theme.highlightColor : Theme.secondaryColor
                            font.pixelSize: Theme.fontSizeSmall
                            horizontalAlignment: Text.AlignRight
                        }

                        BusyIndicator {
                            anchors.horizontalCenter: parent.horizontalCenter
                            running: model.busy
                            visible: running
                            size: BusyIndicatorSize.ExtraSmall
                        }
                    }

                    onClicked: {
                        if (model.connected && !model.busy) {
                            // Navigate to photos page for this camera
                            pageStack.push(Qt.resolvedUrl("PhotosPage.qml"), {
                                cameraIndex: index,
                                cameraName: model.name || qsTr("Unknown Camera")
                            })
                        }
                    }
                }

                // Empty state
                ViewPlaceholder {
                    enabled: cameraListView.count === 0 && !cameraModel.scanning
                    text: qsTr("No cameras found")
                    hintText: qsTr("Make sure your camera is connected via USB and turned on. Pull down to refresh.")
                }
            }

            // Camera count info
            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("%n camera(s) found", "", cameraModel.count)
                visible: cameraModel.count > 0 && !cameraModel.scanning
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
            }
        }
    }

    Component.onCompleted: {
        // Start initial scan when page loads
        cameraModel.refresh()
    }

    // Handle camera connection/disconnection signals
    Connections {
        target: cameraModel
        onCameraConnected: {
            console.log("Camera connected at index:", index)
        }
        onCameraDisconnected: {
            console.log("Camera disconnected at index:", index)
        }
        onError: {
            console.error("Camera model error:", message)
            // TODO: Show error notification to user
        }
    }
}
