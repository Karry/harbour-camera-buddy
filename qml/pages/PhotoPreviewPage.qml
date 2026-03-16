/*
  Camera Buddy for SFOS
  Copyright (c) 2026 Lukas Karas <lukas.karas@centrum.cz>

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

Page {
    id: photoPreviewPage

    property string photoName: ""
    property string thumbnailBase64: ""
    property string sizeString: ""
    property date dateTime

    allowedOrientations: Orientation.All

    // Allow flick-back gesture
    backNavigation: true

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: parent.height

        PageHeader {
            id: header
            title: photoName
        }

        PinchArea {
            id: pinchArea
            anchors {
                top: header.bottom
                left: parent.left
                right: parent.right
                bottom: infoLabel.top
            }

            property real minScale: 1.0
            property real maxScale: 4.0
            property real currentScale: 1.0

            onPinchStarted: {
                currentScale = previewImage.scale
            }

            onPinchUpdated: {
                var newScale = currentScale * pinch.scale
                previewImage.scale = Math.max(minScale, Math.min(maxScale, newScale))
            }

            onPinchFinished: {
                // Snap back if too small
                if (previewImage.scale < 1.0) {
                    snapBackAnimation.start()
                }
            }

            NumberAnimation {
                id: snapBackAnimation
                target: previewImage
                property: "scale"
                to: 1.0
                duration: 200
                easing.type: Easing.InOutQuad
            }

            Flickable {
                id: imageFlickable
                anchors.fill: parent
                clip: true
                contentWidth: Math.max(width, previewImage.width * previewImage.scale)
                contentHeight: Math.max(height, previewImage.height * previewImage.scale)

                Image {
                    id: previewImage
                    anchors.centerIn: parent
                    source: thumbnailBase64.length > 0 ? ("data:image/jpeg;base64," + thumbnailBase64) : ""
                    fillMode: Image.PreserveAspectFit
                    width: imageFlickable.width
                    height: imageFlickable.height
                    transformOrigin: Item.Center
                    asynchronous: true

                    BusyIndicator {
                        anchors.centerIn: parent
                        running: previewImage.status === Image.Loading
                        size: BusyIndicatorSize.Large
                    }
                }
            }
        }

        Label {
            id: infoLabel
            anchors {
                bottom: parent.bottom
                left: parent.left
                right: parent.right
                margins: Theme.horizontalPageMargin
                bottomMargin: Theme.paddingLarge
            }
            horizontalAlignment: Text.AlignHCenter
            text: sizeString + " • " + Qt.formatDateTime(dateTime, "dd.MM.yyyy hh:mm")
            color: Theme.secondaryHighlightColor
            font.pixelSize: Theme.fontSizeSmall
        }
    }
}

