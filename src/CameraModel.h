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

#pragma once

#include <QObject>
#include <QAbstractListModel>
#include <QTimer>
#include <QSharedPointer>
#include <QMutex>
#include <QThreadPool>

#include <gphoto2/gphoto2.h>

struct CameraDevice {
    QString name;
    QString model;
    QString port;
    QString serialNumber;
    bool connected;
    bool busy;
    Camera* camera;
    GPContext* context;
    QThreadPool* threadPool;

    CameraDevice() : connected(false), busy(false), camera(nullptr), context(nullptr), threadPool(nullptr) {}
    ~CameraDevice();

    QString toString() const;
    bool initialize();
    void cleanup();
    bool isValid() const;
    void initializeThreadPool();
};

Q_DECLARE_METATYPE(CameraDevice*)
Q_DECLARE_METATYPE(QSharedPointer<CameraDevice>)

class CameraModel: public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(bool scanning READ isScanning NOTIFY scanningChanged)
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public slots:
    void refresh();
    void onRefreshTimer();

signals:
    void scanningChanged();
    void countChanged();
    void cameraConnected(int index);
    void cameraDisconnected(int index);
    void error(const QString &message);

public:
    explicit CameraModel(QObject *parent = nullptr);
    CameraModel(const CameraModel&) = delete;
    CameraModel(CameraModel &&) = delete;
    ~CameraModel();
    CameraModel& operator=(const CameraModel&) = delete;
    CameraModel& operator=(CameraModel&&) = delete;

    enum Roles {
        NameRole = Qt::UserRole,
        ModelRole = Qt::UserRole + 1,
        PortRole = Qt::UserRole + 2,
        SerialNumberRole = Qt::UserRole + 3,
        ConnectedRole = Qt::UserRole + 4,
        BusyRole = Qt::UserRole + 5,
        CameraObjectRole = Qt::UserRole + 6
    };
    Q_ENUM(Roles)

    Q_INVOKABLE int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    Q_INVOKABLE QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    Q_INVOKABLE Qt::ItemFlags flags(const QModelIndex &index) const override;

    Q_INVOKABLE bool isScanning() const { return scanning; }
    Q_INVOKABLE QSharedPointer<CameraDevice> getCameraAt(int index) const;
    Q_INVOKABLE QString getCameraName(int index) const;
    Q_INVOKABLE bool isCameraConnected(int index) const;

private:
    void scanForCameras();
    QList<QSharedPointer<CameraDevice>> detectGPhoto2Cameras();
    void initializeGPhoto2();
    void cleanupGPhoto2();

private:
    QList<QSharedPointer<CameraDevice>> cameras;
    QTimer *refreshTimer;
    bool scanning;
    bool gphoto2Initialized;
    QMutex scanMutex;
    GPContext* globalContext;
};
