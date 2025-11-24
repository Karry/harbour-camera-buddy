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

import QtQuick 2.6
import Sailfish.Silica 1.0

import ".." // Global singleton

CoverBackground {
    id: cover

    Column {
        anchors.centerIn: parent
        width: parent.width - 2 * Theme.paddingLarge
        spacing: Theme.paddingMedium

        Label {
            id: label
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("Camera\nBuddy")
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: Theme.fontSizeLarge
        }

        // Download progress indicator
        Column {
            anchors.horizontalCenter: parent.horizontalCenter
            width: parent.width
            spacing: Theme.paddingSmall
            visible: Global.isDownloading || Global.downloadTotalCount > 0

            Image {
                anchors.horizontalCenter: parent.horizontalCenter
                source: "image://theme/icon-m-download"
                width: Theme.iconSizeMedium
                height: Theme.iconSizeMedium
            }

            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Downloading...")
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryColor
                visible: Global.isDownloading
            }

            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("%1 / %2").arg(Global.downloadCompletedCount).arg(Global.downloadTotalCount)
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryColor
            }

            ProgressBar {
                anchors.horizontalCenter: parent.horizontalCenter
                width: parent.width
                value: Global.downloadProgress
                minimumValue: 0
                maximumValue: 1
            }
        }
    }

}
