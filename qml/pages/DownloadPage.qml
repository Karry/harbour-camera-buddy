/*
  Camera Buddy for SFOS
  Copyright (c) 2025 Lukas Karas <lukas.karas@centrum.cz>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.camera.buddy 1.0

Page {
    id: downloadPage

    property string downloadPath: ""
    property var selectedPhotos: []
    property var camera: null
    property alias downloadModel: downloadModel

    allowedOrientations: Orientation.All

    DownloadModel {
        id: downloadModel
        downloadPath: downloadPage.downloadPath
        
        onDownloadFinished: {
            console.log("All downloads finished")
        }
        
        onDownloadProgress: {
            console.log("Download progress:", current, "/", total)
        }
        
        onDownloadError: {
            console.log("Download error:", message)
        }
    }

    Component.onCompleted: {
        if (camera) {
            downloadModel.setCamera(camera)
        }

        if (selectedPhotos.length > 0) {
            downloadModel.startDownload(selectedPhotos)
        }
    }

    SilicaListView {
        id: listView
        anchors.fill: parent
        model: downloadModel

        header: PageHeader {
            title: qsTr("Downloading Photos")
        }

        VerticalScrollDecorator {}

        delegate: ListItem {
            id: listItem
            width: listView.width
            height: Theme.itemSizeLarge

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
                        source: model.thumbnailBase64 ? ("data:image/jpeg;base64," + model.thumbnailBase64) : ""
                        fillMode: Image.PreserveAspectCrop
                        asynchronous: true
                    }

                    // Status overlay
                    Rectangle {
                        anchors.fill: parent
                        color: {
                            switch (model.status) {
                                case 0: return "transparent" // Pending
                                case 1: return Qt.rgba(0, 0, 1, 0.3) // Downloading
                                case 2: return Qt.rgba(0, 1, 0, 0.3) // Completed
                                case 3: return Qt.rgba(1, 0, 0, 0.3) // Error
                                default: return "transparent"
                            }
                        }

                        Image {
                            anchors.centerIn: parent
                            source: {
                                switch (model.status) {
                                    case 1: return "" // Downloading - show progress
                                    case 2: return "image://theme/icon-s-accept"
                                    case 3: return "image://theme/icon-s-clear"
                                    default: return ""
                                }
                            }
                            width: Theme.iconSizeSmall
                            height: Theme.iconSizeSmall
                        }

                        BusyIndicator {
                            anchors.centerIn: parent
                            running: model.status === 1
                            size: BusyIndicatorSize.Small
                        }
                    }
                }

                Column {
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width - Theme.itemSizeSmall - Theme.paddingMedium

                    Label {
                        text: model.fileName
                        color: Theme.primaryColor
                        truncationMode: TruncationMode.Fade
                        width: parent.width
                    }

                    Label {
                        text: {
                            switch (model.status) {
                                case 0: return qsTr("Pending...")
                                case 1: return qsTr("Downloading... %1%").arg(Math.round(model.progress * 100))
                                case 2: return qsTr("Completed")
                                case 3: return qsTr("Error: %1").arg(model.errorMessage)
                                default: return ""
                            }
                        }
                        color: Theme.secondaryColor
                        font.pixelSize: Theme.fontSizeExtraSmall
                        truncationMode: TruncationMode.Fade
                        width: parent.width
                    }

                    ProgressBar {
                        width: parent.width
                        visible: model.status === 1
                        value: model.progress
                        minimumValue: 0
                        maximumValue: 1
                    }
                }
            }
        }

        ViewPlaceholder {
            enabled: listView.count === 0
            text: qsTr("No downloads")
            hintText: qsTr("No photos selected for download")
        }
    }

    // Overall progress
    Rectangle {
        anchors.bottom: parent.bottom
        width: parent.width
        height: Theme.itemSizeSmall
        color: Theme.highlightBackgroundColor
        opacity: 0.9
        visible: downloadModel.totalCount > 0

        Column {
            anchors.centerIn: parent
            width: parent.width - 2 * Theme.horizontalPageMargin

            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Overall Progress: %1 / %2").arg(downloadModel.completedCount).arg(downloadModel.totalCount)
                color: Theme.primaryColor
                font.pixelSize: Theme.fontSizeSmall
            }

            ProgressBar {
                anchors.horizontalCenter: parent.horizontalCenter
                width: parent.width
                value: downloadModel.totalCount > 0 ? downloadModel.completedCount / downloadModel.totalCount : 0
                minimumValue: 0
                maximumValue: 1
            }
        }
    }
}
