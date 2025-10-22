import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.camera.buddy 1.0

Page {
    id: photosPage

    property int cameraIndex: -1
    property string cameraName: ""
    property alias photosModel: photosModel

    allowedOrientations: Orientation.All

    PhotosModel {
        id: photosModel
        cameraIndex: photosPage.cameraIndex
    }

    Component.onCompleted: {
        console.log("PhotosModel Component.onCompleted")
        console.log("  global cameraModel:", cameraModel)
        console.log("  photosPage.cameraIndex:", photosPage.cameraIndex)

        photosModel.cameraModel = cameraModel
    }

    SilicaListView {
        id: listView
        anchors.fill: parent
        model: photosModel

        header: PageHeader {
            title: qsTr("Photos") + " (" + photosModel.count + ")"
        }

        PullDownMenu {
            MenuItem {
                text: qsTr("Refresh")
                onClicked: photosModel.refresh()
            }
            MenuItem {
                text: qsTr("Select All")
                onClicked: photosModel.selectAll(true)
            }
            MenuItem {
                text: qsTr("Select JPEGs")
                onClicked: photosModel.selectJpegs()
            }
            MenuItem {
                text: qsTr("Deselect All")
                onClicked: photosModel.selectAll(false)
            }
        }

        PushUpMenu {
            visible: photosModel.selectedCount > 0
            MenuItem {
                text: qsTr("Download Selected") + " (" + photosModel.selectedCount + ")"
                onClicked: {
                    // TODO: Implement download functionality
                    console.log("Download selected photos")
                }
            }
        }

        delegate: ListItem {
            id: listItem
            width: listView.width
            height: Theme.itemSizeMedium

            onClicked: photosModel.toggleSelection(index)

            Row {
                anchors.fill: parent
                anchors.margins: Theme.paddingMedium
                spacing: Theme.paddingMedium

                Rectangle {
                    width: Theme.itemSizeSmall
                    height: Theme.itemSizeSmall
                    anchors.verticalCenter: parent.verticalCenter
                    color: Theme.secondaryColor

                    Image {
                        anchors.fill: parent
                        anchors.margins: 2
                        source: model.thumbnailAvailable ? ("data:image/jpeg;base64," + model.thumbnailBase64) : ""
                        fillMode: Image.PreserveAspectCrop
                        asynchronous: true

                        BusyIndicator {
                            anchors.centerIn: parent
                            running: !model.thumbnailAvailable
                            size: BusyIndicatorSize.Small
                        }

                        Component.onCompleted: {
                            if (!model.thumbnailAvailable) {
                                photosModel.loadThumbnail(index)
                            }
                        }
                    }
                }

                Column {
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width - Theme.itemSizeSmall - Theme.paddingMedium - checkbox.width - Theme.paddingMedium

                    Label {
                        text: model.name
                        color: listItem.highlighted ? Theme.highlightColor : Theme.primaryColor
                        truncationMode: TruncationMode.Fade
                        width: parent.width
                    }

                    Label {
                        text: model.sizeString + " • " + Qt.formatDateTime(model.dateTime, "dd.MM.yyyy hh:mm")
                        color: listItem.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                        font.pixelSize: Theme.fontSizeExtraSmall
                        truncationMode: TruncationMode.Fade
                        width: parent.width
                    }
                }

                Switch {
                    id: checkbox
                    anchors.verticalCenter: parent.verticalCenter
                    checked: model.selected
                    onClicked: photosModel.selectPhoto(index, checked)
                }
            }
        }

        ViewPlaceholder {
            enabled: listView.count === 0 && !photosModel.loading
            text: qsTr("No photos found")
            hintText: qsTr("Pull down to refresh")
        }

        BusyIndicator {
            anchors.centerIn: parent
            running: photosModel.loading
            size: BusyIndicatorSize.Large
        }
    }
}
