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
#include <QThreadPool>

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

    // Clean up lists before initialization
    gp_abilities_list_free(abilities);
    gp_port_info_list_free(portinfolist);

    // Try to initialize the camera with USB-specific handling
    ret = gp_camera_init(camera, context);
    if (ret < GP_OK) {
        qWarning() << "Failed to initialize camera" << model << "on" << port << ":" << gp_result_as_string(ret);

        // If initialization failed with USB device claim error, try to reset the port
        if (ret == GP_ERROR_IO_USB_CLAIM) {
            qDebug() << "USB claim failed, attempting to reset camera connection...";

            // Exit and re-create camera object
            gp_camera_exit(camera, context);
            gp_camera_free(camera);

            // Wait a bit before retry
            QThread::msleep(1000);

            // Try again with new camera object
            ret = gp_camera_new(&camera);
            if (ret >= GP_OK) {
                // Re-apply settings
                if (modelIndex >= 0) {
                    CameraAbilitiesList *newAbilities = nullptr;
                    if (gp_abilities_list_new(&newAbilities) >= GP_OK) {
                        if (gp_abilities_list_load(newAbilities, context) >= GP_OK) {
                            CameraAbilities cameraAbilities;
                            if (gp_abilities_list_get_abilities(newAbilities, modelIndex, &cameraAbilities) >= GP_OK) {
                                gp_camera_set_abilities(camera, cameraAbilities);
                            }
                        }
                        gp_abilities_list_free(newAbilities);
                    }
                }

                if (portIndex >= 0) {
                    GPPortInfoList *newPortList = nullptr;
                    if (gp_port_info_list_new(&newPortList) >= GP_OK) {
                        if (gp_port_info_list_load(newPortList) >= GP_OK) {
                            GPPortInfo portInfo;
                            if (gp_port_info_list_get_info(newPortList, portIndex, &portInfo) >= GP_OK) {
                                gp_camera_set_port_info(camera, portInfo);
                            }
                        }
                        gp_port_info_list_free(newPortList);
                    }
                }

                // Second initialization attempt
                ret = gp_camera_init(camera, context);
            }
        }

        if (ret < GP_OK) {
            qWarning() << "Second initialization attempt also failed:" << gp_result_as_string(ret);
            gp_camera_free(camera);
            camera = nullptr;
            return false;
        }
    }

    connected = true;

    // Initialize the single-threaded pool for gphoto2 operations
    initializeThreadPool();

    qDebug() << "Successfully initialized camera:" << toString();
    return true;
}

void CameraDevice::cleanup() {
    // Wait for all gphoto2 operations to complete before cleanup
    if (threadPool) {
        threadPool->waitForDone();
        delete threadPool;
        threadPool = nullptr;
    }

    if (camera) {
        gp_camera_exit(camera, context);
        gp_camera_free(camera);
        camera = nullptr;
    }
    connected = false;
    busy = false;
}

void CameraDevice::initializeThreadPool() {
    if (!threadPool) {
        threadPool = new QThreadPool();
        // Set to single thread to serialize all gphoto2 operations for this camera
        threadPool->setMaxThreadCount(1);
        qDebug() << "Initialized single-threaded pool for camera:" << toString();
    }
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

    // First, detect available cameras without initializing them
    QList<QSharedPointer<CameraDevice>> detectedCameras = detectGPhoto2Cameras();

    // Compare with existing cameras and update the model
    bool changed = false;
    QList<QSharedPointer<CameraDevice>> newCameras;

    // Check each detected camera
    for (const auto &detected : detectedCameras) {
        QSharedPointer<CameraDevice> existingCamera;

        // Look for this camera in our existing list
        for (const auto &existing : cameras) {
            if (existing->model == detected->model && existing->port == detected->port) {
                existingCamera = existing;
                break;
            }
        }

        if (existingCamera) {
            // Camera already exists, keep the existing one (preserves connection state)
            newCameras.append(existingCamera);
            qDebug() << "Keeping existing camera:" << existingCamera->toString() << "connected:" << existingCamera->connected;
        } else {
            // New camera detected, try to initialize it
            qDebug() << "New camera detected, attempting to initialize:" << detected->toString();
            if (detected->initialize()) {
                qDebug() << "Successfully initialized new camera:" << detected->toString();
            } else {
                qWarning() << "Failed to initialize new camera:" << detected->toString();
                detected->connected = false;
            }
            newCameras.append(detected);
            changed = true;
        }
    }

    // Check if any existing cameras were removed
    if (newCameras.size() != cameras.size()) {
        changed = true;
    }

    if (changed) {
        beginResetModel();

        // Cleanup cameras that are no longer detected
        for (auto &camera : cameras) {
            bool found = false;
            for (const auto &newCamera : newCameras) {
                if (camera->model == newCamera->model && camera->port == newCamera->port) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                qDebug() << "Cleaning up removed camera:" << camera->toString();
                camera->cleanup();
            }
        }

        cameras = newCameras;
        endResetModel();

        emit countChanged();
        qDebug() << "Found" << cameras.size() << "cameras";

        for (const auto &camera : cameras) {
            qDebug() << "  " << camera->toString() << "connected:" << camera->connected;
        }
    } else {
        qDebug() << "No camera changes detected";
    }
}

QList<QSharedPointer<CameraDevice>> CameraModel::detectGPhoto2Cameras() {
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
        camera->connected = false; // Will be set to true only after successful initialization

        qDebug() << "Detected camera:" << camera->model << "on port:" << camera->port << "name:" << camera->name;

        result.append(camera);
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
    roles[ModelRole] = "cameraModel";
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
