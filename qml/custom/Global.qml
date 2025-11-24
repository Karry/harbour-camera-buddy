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

pragma Singleton

import QtQuick 2.0

Item {
    // Download progress tracking
    property int downloadCompletedCount: 0
    property int downloadTotalCount: 0
    property bool isDownloading: false

    // Computed property for progress percentage
    readonly property real downloadProgress: downloadTotalCount > 0 ? downloadCompletedCount / downloadTotalCount : 0

    // Reset download progress
    function resetDownloadProgress() {
        downloadCompletedCount = 0
        downloadTotalCount = 0
        isDownloading = false
    }

    // Update download progress
    function updateDownloadProgress(completed, total) {
        downloadCompletedCount = completed
        downloadTotalCount = total
        isDownloading = total > 0 && completed < total
    }
}
