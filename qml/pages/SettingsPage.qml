import QtQuick 2.6
import Sailfish.Silica 1.0

Dialog {
    id: settingsDialog

    allowedOrientations: Orientation.All

    onAccepted: {
        settings.ptpIpAddress = ipAddressField.text
        cameraModel.refresh()
    }

    canAccept: ipAddressField.text.length > 0

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column
            width: parent.width
            spacing: Theme.paddingLarge

            DialogHeader {
                title: qsTr("Settings")
                acceptText: qsTr("Save")
                cancelText: qsTr("Cancel")
            }

            SectionHeader {
                text: qsTr("PTP/IP Camera")
            }

            TextField {
                id: ipAddressField
                width: parent.width
                label: qsTr("IP Address")
                placeholderText: qsTr("Enter IP address (e.g., 192.168.1.1)")
                text: settings.ptpIpAddress
                inputMethodHints: Qt.ImhPreferNumbers

                EnterKey.enabled: text.length > 0
                EnterKey.iconSource: "image://theme/icon-m-enter-accept"
                EnterKey.onClicked: {
                    ipAddressField.focus = false
                    settingsDialog.accept()
                }
                Component.onCompleted: {
                    ipAddressField.forceActiveFocus()
                }
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: qsTr("Enter the IP address of your PTP/IP compatible camera. The camera will be detected automatically after you accept the settings.")
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                wrapMode: Text.WordWrap
            }
        }
    }
}

