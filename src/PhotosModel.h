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
#include <QSharedPointer>
#include <QDateTime>
#include <QSize>
#include <QMutex>
#include <QTimer>

#include "CameraModel.h"
#include <gphoto2/gphoto2.h>

struct PhotoInfo {
    QString name;
    QString folder;
    QString fullPath;
    QDateTime dateTime;
    qint64 size;
    QString mimeType;
    QSize dimensions;
    bool selected;
    bool thumbnail_available;
    QByteArray thumbnailData;

    PhotoInfo() : size(0), selected(false), thumbnail_available(false) {}

    QString toString() const;
    QString sizeString() const;
};

Q_DECLARE_METATYPE(PhotoInfo*)
Q_DECLARE_METATYPE(QSharedPointer<PhotoInfo>)

class PhotosModel: public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(bool loading READ isLoading NOTIFY loadingChanged)
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(int selectedCount READ getSelectedCount NOTIFY selectedCountChanged)
    Q_PROPERTY(QString cameraName READ getCameraName NOTIFY cameraNameChanged)
    Q_PROPERTY(int cameraIndex READ getCameraIndex WRITE setCameraIndex NOTIFY cameraIndexChanged)
    Q_PROPERTY(CameraModel* cameraModel READ getCameraModel WRITE setCameraModel NOTIFY cameraModelChanged)

public slots:
    void refresh();
    void selectAll(bool select = true);
    void selectJpegs();
    void selectPhoto(int index, bool select = true);
    void toggleSelection(int index);
    void loadThumbnail(int index);

signals:
    void loadingChanged();
    void countChanged();
    void selectedCountChanged();
    void cameraNameChanged();
    void cameraIndexChanged();
    void cameraModelChanged();
    void thumbnailLoaded(int index);
    void error(const QString &message);

public:
    explicit PhotosModel(QObject *parent = nullptr);
    PhotosModel(const PhotosModel&) = delete;
    PhotosModel(PhotosModel &&) = delete;
    ~PhotosModel();
    PhotosModel& operator=(const PhotosModel&) = delete;
    PhotosModel& operator=(PhotosModel&&) = delete;

    enum Roles {
        NameRole = Qt::UserRole,
        FolderRole = Qt::UserRole + 1,
        FullPathRole = Qt::UserRole + 2,
        DateTimeRole = Qt::UserRole + 3,
        SizeRole = Qt::UserRole + 4,
        SizeStringRole = Qt::UserRole + 5,
        MimeTypeRole = Qt::UserRole + 6,
        DimensionsRole = Qt::UserRole + 7,
        SelectedRole = Qt::UserRole + 8,
        ThumbnailAvailableRole = Qt::UserRole + 9,
        ThumbnailDataRole = Qt::UserRole + 10,
        ThumbnailBase64Role = Qt::UserRole + 11
    };
    Q_ENUM(Roles)

    Q_INVOKABLE int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    Q_INVOKABLE QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    Q_INVOKABLE Qt::ItemFlags flags(const QModelIndex &index) const override;
    Q_INVOKABLE bool setData(const QModelIndex &index, const QVariant &value, int role) override;

    Q_INVOKABLE bool isLoading() const { return loading; }
    Q_INVOKABLE int getSelectedCount() const;
    Q_INVOKABLE QString getCameraName() const { return cameraName; }
    Q_INVOKABLE int getCameraIndex() const { return cameraIndex; }
    Q_INVOKABLE void setCameraIndex(int index);
    Q_INVOKABLE CameraModel* getCameraModel() const { return cameraModel; }
    Q_INVOKABLE void setCameraModel(CameraModel* model);
    Q_INVOKABLE QSharedPointer<PhotoInfo> getPhotoAt(int index) const;
    Q_INVOKABLE QVariantList getSelectedPhotos() const;
    Q_INVOKABLE QVariant getCamera() const;


private:
    void loadPhotosFromCamera();
    QList<QSharedPointer<PhotoInfo>> scanCameraFolders(Camera* camera, GPContext* context);
    QList<QSharedPointer<PhotoInfo>> scanFolder(Camera* camera, GPContext* context, const QString& folder);
    PhotoInfo getPhotoInfo(Camera* camera, GPContext* context, const QString& folder, const QString& filename);

    static QByteArray loadThumbnailData(Camera* camera, GPContext* context, const QString& folder, const QString& filename);

private:
    QList<QSharedPointer<PhotoInfo>> photos;
    CameraModel* cameraModel;
    int cameraIndex;
    QString cameraName;
    bool loading;
    QMutex loadMutex;
    QTimer* thumbnailTimer;
};
