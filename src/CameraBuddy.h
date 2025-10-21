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

#pragma once

#include <QGuiApplication>
#include <QObject>
#include <QLocale>

class CameraBuddy : public QObject
{
    Q_OBJECT

public:
    explicit CameraBuddy(QObject *parent = nullptr);
    ~CameraBuddy() = default;

    static QString version();

    void initializeGPhoto2();
    void setupTranslations();
    void setApp(QGuiApplication* app);

signals:


private:
    QGuiApplication* m_app;
};
