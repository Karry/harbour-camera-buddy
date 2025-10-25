/*
  Camera Buddy for SailfishOS
  Copyright (C) 2025  Lukáš Karas

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

#include "Settings.h"

Settings::Settings(QObject *parent)
    : QObject(parent)
    , settings("cz.karry.camera-buddy", "CameraBuddy")
{
    loadSettings();
}

QString Settings::ptpIpAddress() const {
    return ptpIpAddressStr;
}

void Settings::setPtpIpAddress(const QString &address) {
    if (ptpIpAddressStr != address) {
        ptpIpAddressStr = address;
        saveSettings();
        emit ptpIpAddressChanged();
    }
}

void Settings::loadSettings() {
    ptpIpAddressStr = settings.value("ptpIpAddress", "192.168.1.1").toString();
}

void Settings::saveSettings() {
    settings.setValue("ptpIpAddress", ptpIpAddressStr);
    settings.sync();
}

