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

#include <harbour-camera-buddy/private/Version.h>
#include "CameraBuddy.h"
#include "CameraModel.h"

// SFOS
#include <sailfishapp/sailfishapp.h>

// Qt includes
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickView>
#include <QQmlContext>
#include <QtCore/QtGlobal>
#include <QSettings>
#include <QTextStream>
#include <QTranslator>
#include <QLocale>
#include <QDebug>

// GPhoto2 includes
#include <gphoto2/gphoto2.h>

#include <iostream>
#include <locale>
#include <sstream>

#ifndef CAMERA_BUDDY_VERSION_STRING
static_assert(false, "CAMERA_BUDDY_VERSION_STRING should be defined by build system");
#endif

std::string osPrettyName(){
  QSettings osRelease("/etc/os-release", QSettings::IniFormat);
  QVariant prettyName=osRelease.value("PRETTY_NAME");
  if (prettyName.isValid()){
    return prettyName.toString().toStdString();
  } else {
    return "Unknown OS";
  }
}

std::string versionStrings(){
  std::stringstream ss;
  ss << "harbour-camera-buddy"
     << " " << CAMERA_BUDDY_VERSION_STRING
     << " (Qt " << qVersion() << ", " << osPrettyName() << ")";

  return ss.str();
}

CameraBuddy::CameraBuddy(QObject *parent)
    : QObject(parent)
    , m_app(nullptr)
{
}

QString CameraBuddy::version()
{
    return {CAMERA_BUDDY_VERSION_STRING};
}

void CameraBuddy::setApp(QGuiApplication* app)
{
    m_app = app;
}

void CameraBuddy::initializeGPhoto2()
{
    // Initialize GPhoto2 library
    gp_log_add_func(GP_LOG_ERROR, nullptr, nullptr);
    gp_log_add_func(GP_LOG_DEBUG, nullptr, nullptr);
}

void CameraBuddy::setupTranslations()
{
    if (!m_app) return;

    // Install translator
    QTranslator translator;
    QLocale locale;
    if (translator.load(locale.name(), SailfishApp::pathTo("translations").toLocalFile())) {
        qDebug() << "Install translator for locale " << locale << "/" << locale.name();
        QGuiApplication::installTranslator(&translator);
    } else {
        qWarning() << "Can't load translator for locale" << locale << "/" << locale.name() <<
                   "(" << SailfishApp::pathTo("translations").toLocalFile() << ")";
    }
}

Q_DECL_EXPORT int main(int argc, char* argv[]) {
#ifdef Q_WS_X11
    QCoreApplication::setAttribute(Qt::AA_X11InitThreads);
#endif

    QGuiApplication *app = SailfishApp::application(argc, argv);

    QGuiApplication::setOrganizationDomain("camera-buddy.karry.cz");
    QGuiApplication::setOrganizationName("cz.karry.camera-buddy"); // needed for Sailjail
    QGuiApplication::setApplicationName("CameraBuddy");
    QGuiApplication::setApplicationVersion(CAMERA_BUDDY_VERSION_STRING);

    std::cout << "Starting " << versionStrings() << std::endl;

#ifdef QT_QML_DEBUG
    qWarning() << "Starting QML debugger on port 1234.";
#endif

    // Setup C++ locale
    try {
        std::locale::global(std::locale(""));
    } catch (const std::runtime_error& e) {
        std::cerr << "Cannot set locale: \"" << e.what() << "\"" << std::endl;
    }

    // Create main application object
    CameraBuddy cameraBuddy;
    cameraBuddy.setApp(app);

    // Setup translations
    cameraBuddy.setupTranslations();

    // Initialize GPhoto2
    cameraBuddy.initializeGPhoto2();

    // Register QML types
    qmlRegisterType<CameraModel>("harbour.camerabuddy", 1, 0, "CameraModel");

    // Create camera model instance
    CameraModel cameraModel;

    QScopedPointer<QQuickView> view(SailfishApp::createView());
    view->rootContext()->setContextProperty("cameraModel", &cameraModel);
    view->setSource(SailfishApp::pathTo("qml/main.qml"));
    view->show();

    return QGuiApplication::exec();
}

