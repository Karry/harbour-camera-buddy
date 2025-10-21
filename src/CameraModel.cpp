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

#include "CameraModel.h"

#include <QDebug>
#include <QThread>
#include <QMutexLocker>
#include <QCoreApplication>

#include <gphoto2/gphoto2.h>

// CameraDevice implementation
CameraDevice::~CameraDevice() {
    cleanup();
}

QString CameraDevice::toString() const {
    return QString("Camera: %1 (%2) on %3").arg(name, model, port);
}

bool CameraDevice::initialize() {
    if (camera || !context) {
        return false;
    }

    int ret = gp_camera_new(&camera);
    if (ret < GP_OK) {
        qWarning() << "Failed to create camera object:" << gp_result_as_string(ret);
        return false;
    }

    // Set the camera model and port
    GPPortInfoList *portinfolist = nullptr;
    CameraAbilitiesList *abilities = nullptr;

    ret = gp_port_info_list_new(&portinfolist);
    if (ret < GP_OK) {
        qWarning() << "Failed to create port info list:" << gp_result_as_string(ret);
        gp_camera_free(camera);
        camera = nullptr;
        return false;
    }

    ret = gp_port_info_list_load(portinfolist);
    if (ret < GP_OK) {
        qWarning() << "Failed to load port info list:" << gp_result_as_string(ret);
        gp_port_info_list_free(portinfolist);
        gp_camera_free(camera);
        camera = nullptr;
        return false;
    }

    ret = gp_abilities_list_new(&abilities);
    if (ret < GP_OK) {
        qWarning() << "Failed to create abilities list:" << gp_result_as_string(ret);
        gp_port_info_list_free(portinfolist);
        gp_camera_free(camera);
        camera = nullptr;
        return false;
    }

    ret = gp_abilities_list_load(abilities, context);
    if (ret < GP_OK) {
        qWarning() << "Failed to load abilities list:" << gp_result_as_string(ret);
        gp_abilities_list_free(abilities);
        gp_port_info_list_free(portinfolist);
        gp_camera_free(camera);
        camera = nullptr;
        return false;
    }

    // Find and set camera abilities
    int modelIndex = gp_abilities_list_lookup_model(abilities, model.toUtf8().constData());
    if (modelIndex >= 0) {
        CameraAbilities cameraAbilities;
        ret = gp_abilities_list_get_abilities(abilities, modelIndex, &cameraAbilities);
        if (ret >= GP_OK) {
            ret = gp_camera_set_abilities(camera, cameraAbilities);
            if (ret < GP_OK) {
                qWarning() << "Failed to set camera abilities:" << gp_result_as_string(ret);
            }
        }
    }

    // Find and set port info
    int portIndex = gp_port_info_list_lookup_path(portinfolist, port.toUtf8().constData());
    if (portIndex >= 0) {
        GPPortInfo portInfo;
        ret = gp_port_info_list_get_info(portinfolist, portIndex, &portInfo);
        if (ret >= GP_OK) {
            ret = gp_camera_set_port_info(camera, portInfo);
            if (ret < GP_OK) {
                qWarning() << "Failed to set camera port info:" << gp_result_as_string(ret);
            }
        }
    }

    // Try to initialize the camera
    ret = gp_camera_init(camera, context);
    if (ret < GP_OK) {
        qWarning() << "Failed to initialize camera" << model << "on" << port << ":" << gp_result_as_string(ret);
        gp_abilities_list_free(abilities);
        gp_port_info_list_free(portinfolist);
        gp_camera_free(camera);
        camera = nullptr;
        return false;
    }

    connected = true;

    // Cleanup
    gp_abilities_list_free(abilities);
    gp_port_info_list_free(portinfolist);

    qDebug() << "Successfully initialized camera:" << toString();
    return true;
}

void CameraDevice::cleanup() {
    if (camera) {
        gp_camera_exit(camera, context);
        gp_camera_free(camera);
        camera = nullptr;
    }
    connected = false;
    busy = false;
}

bool CameraDevice::isValid() const {
    return camera != nullptr && connected;
}

// CameraModel implementation
CameraModel::CameraModel(QObject *parent)
    : QAbstractListModel(parent)
    , refreshTimer(new QTimer(this))
    , scanning(false)
    , gphoto2Initialized(false)
    , globalContext(nullptr)
{
    initializeGPhoto2();

    // Set up refresh timer for periodic camera detection
    refreshTimer->setSingleShot(false);
    refreshTimer->setInterval(5000); // Check every 5 seconds
    connect(refreshTimer, &QTimer::timeout, this, &CameraModel::onRefreshTimer);

    // Initial scan
    refresh();

    // Start periodic scanning
    refreshTimer->start();
}

CameraModel::~CameraModel() {
    refreshTimer->stop();

    // Cleanup all cameras
    for (auto &camera : cameras) {
        camera->cleanup();
    }
    cameras.clear();

    cleanupGPhoto2();
}

void CameraModel::initializeGPhoto2() {
    if (gphoto2Initialized) {
        return;
    }

    globalContext = gp_context_new();
    if (!globalContext) {
        qWarning() << "Failed to create GPhoto2 context";
        return;
    }

    // Set up error handling
    gp_context_set_error_func(globalContext, nullptr, nullptr);
    gp_context_set_message_func(globalContext, nullptr, nullptr);

    gphoto2Initialized = true;
    qDebug() << "GPhoto2 initialized successfully";
}

void CameraModel::cleanupGPhoto2() {
    if (globalContext) {
        gp_context_unref(globalContext);
        globalContext = nullptr;
    }
    gphoto2Initialized = false;
}

void CameraModel::refresh() {
    if (scanning) {
        return; // Already scanning
    }

    scanning = true;
    emit scanningChanged();

    qDebug() << "Scanning for cameras...";
    scanForCameras();

    scanning = false;
    emit scanningChanged();
}

void CameraModel::onRefreshTimer() {
    // Only auto-refresh if we're not currently scanning
    if (!scanning) {
        refresh();
    }
}

void CameraModel::scanForCameras() {
    QMutexLocker locker(&scanMutex);

    if (!gphoto2Initialized) {
        emit error("GPhoto2 not initialized");
        return;
    }

    QList<QSharedPointer<CameraDevice>> newCameras = listGPhoto2Cameras();

    // Compare with existing cameras and update the model
    bool changed = false;

    if (newCameras.size() != cameras.size()) {
        changed = true;
    } else {
        // Check if any camera changed
        for (int i = 0; i < newCameras.size(); ++i) {
            if (i >= cameras.size() ||
                newCameras[i]->name != cameras[i]->name ||
                newCameras[i]->port != cameras[i]->port ||
                newCameras[i]->model != cameras[i]->model) {
                changed = true;
                break;
            }
        }
    }

    if (changed) {
        beginResetModel();

        // Cleanup old cameras
        for (auto &camera : cameras) {
            camera->cleanup();
        }

        cameras = newCameras;
        endResetModel();

        emit countChanged();
        qDebug() << "Found" << cameras.size() << "cameras";

        for (const auto &camera : cameras) {
            qDebug() << "  " << camera->toString();
        }
    }
}

QList<QSharedPointer<CameraDevice>> CameraModel::listGPhoto2Cameras() {
    QList<QSharedPointer<CameraDevice>> result;

    if (!globalContext) {
        return result;
    }

    CameraList *cameraList = nullptr;
    int ret = gp_list_new(&cameraList);
    if (ret < GP_OK) {
        qWarning() << "Failed to create camera list:" << gp_result_as_string(ret);
        return result;
    }

    // Auto-detect cameras
    ret = gp_camera_autodetect(cameraList, globalContext);
    if (ret < GP_OK) {
        qWarning() << "Failed to auto-detect cameras:" << gp_result_as_string(ret);
        gp_list_free(cameraList);
        return result;
    }

    int count = gp_list_count(cameraList);
    qDebug() << "GPhoto2 detected" << count << "cameras";

    for (int i = 0; i < count; ++i) {
        const char *model = nullptr;
        const char *port = nullptr;

        ret = gp_list_get_name(cameraList, i, &model);
        if (ret < GP_OK) {
            qWarning() << "Failed to get camera model at index" << i << ":" << gp_result_as_string(ret);
            continue;
        }

        ret = gp_list_get_value(cameraList, i, &port);
        if (ret < GP_OK) {
            qWarning() << "Failed to get camera port at index" << i << ":" << gp_result_as_string(ret);
            continue;
        }

        auto camera = QSharedPointer<CameraDevice>::create();
        camera->model = QString::fromUtf8(model);
        camera->port = QString::fromUtf8(port);
        camera->name = QString("%1 (%2)").arg(camera->model, camera->port);
        camera->context = globalContext;

        // Try to initialize the camera to verify it's working
        if (camera->initialize()) {
            result.append(camera);
        } else {
            qWarning() << "Failed to initialize camera:" << camera->toString();
        }
    }

    gp_list_free(cameraList);
    return result;
}

int CameraModel::rowCount(const QModelIndex &parent) const {
    Q_UNUSED(parent)
    return cameras.size();
}

QVariant CameraModel::data(const QModelIndex &index, int role) const {
    if (index.row() < 0 || index.row() >= cameras.size()) {
        return QVariant();
    }

    const auto &camera = cameras[index.row()];

    switch (role) {
        case NameRole:
            return camera->name;
        case ModelRole:
            return camera->model;
        case PortRole:
            return camera->port;
        case SerialNumberRole:
            return camera->serialNumber;
        case ConnectedRole:
            return camera->connected;
        case BusyRole:
            return camera->busy;
        case CameraObjectRole:
            return QVariant::fromValue(camera);
        default:
            return QVariant();
    }
}

QHash<int, QByteArray> CameraModel::roleNames() const {
    QHash<int, QByteArray> roles = QAbstractItemModel::roleNames();

    roles[NameRole] = "name";
    roles[ModelRole] = "model";
    roles[PortRole] = "port";
    roles[SerialNumberRole] = "serialNumber";
    roles[ConnectedRole] = "connected";
    roles[BusyRole] = "busy";
    roles[CameraObjectRole] = "cameraObject";

    return roles;
}

Qt::ItemFlags CameraModel::flags(const QModelIndex &index) const {
    if (!index.isValid()) {
        return Qt::ItemIsEnabled;
    }

    return QAbstractItemModel::flags(index) | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QSharedPointer<CameraDevice> CameraModel::getCameraAt(int index) const {
    if (index < 0 || index >= cameras.size()) {
        return QSharedPointer<CameraDevice>();
    }
    return cameras[index];
}

QString CameraModel::getCameraName(int index) const {
    if (index < 0 || index >= cameras.size()) {
        return QString();
    }
    return cameras[index]->name;
}

bool CameraModel::isCameraConnected(int index) const {
    if (index < 0 || index >= cameras.size()) {
        return false;
    }
    return cameras[index]->connected;
}
