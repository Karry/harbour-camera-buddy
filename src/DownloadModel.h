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

#include <PhotosModel.h>
#include <CameraModel.h>

enum DownloadStatus {
    Pending = 0,
    Downloading = 1,
    Completed = 2,
    Error = 3
};

struct DownloadItem {
    QSharedPointer<PhotoInfo> photo;
    QString fileName;
    QString filePath;
    QString thumbnailBase64;
    DownloadStatus status;
    qreal progress;
    QString errorMessage;

    DownloadItem() : status(Pending), progress(0.0) {}
};

class DownloadModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(QString downloadPath READ downloadPath WRITE setDownloadPath NOTIFY downloadPathChanged)
    Q_PROPERTY(int totalCount READ totalCount NOTIFY totalCountChanged)
    Q_PROPERTY(int completedCount READ completedCount NOTIFY completedCountChanged)
    Q_PROPERTY(int pendingCount READ pendingCount NOTIFY pendingCountChanged)
    Q_PROPERTY(bool isDownloading READ isDownloading NOTIFY isDownloadingChanged)

public:
    explicit DownloadModel(QObject *parent = nullptr);
    ~DownloadModel();

    enum Roles {
        FileNameRole = Qt::UserRole,
        FilePathRole = Qt::UserRole + 1,
        ThumbnailBase64Role = Qt::UserRole + 2,
        StatusRole = Qt::UserRole + 3,
        ProgressRole = Qt::UserRole + 4,
        ErrorMessageRole = Qt::UserRole + 5
    };
    Q_ENUM(Roles)

    // QAbstractListModel interface
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Properties
    QString downloadPath() const { return downloadPath_; }
    void setDownloadPath(const QString &path);

    int totalCount() const { return downloads.size(); }
    int completedCount() const;
    int pendingCount() const;
    bool isDownloading() const { return isDownloading_; }

    // Methods
    Q_INVOKABLE void startDownload(const QVariantList &selectedPhotos);
    Q_INVOKABLE void cancelDownload();
    Q_INVOKABLE void retryFailed();
    Q_INVOKABLE void setCamera(const QVariant &cameraVariant);

signals:
    void downloadPathChanged();
    void totalCountChanged();
    void completedCountChanged();
    void pendingCountChanged();
    void isDownloadingChanged();
    void downloadFinished();
    void downloadProgress(int current, int total);
    void downloadError(const QString &message);

public slots:
    void onDownloadItemFinished(int index, bool success, const QString &errorMessage);
    void onDownloadItemProgress(int index, qreal progress);

private:
    void updateCounts();
    void processNextDownload();
    QString generateFileName(const QSharedPointer<PhotoInfo> &photo);

    QString downloadPath_;
    QList<DownloadItem> downloads;
    bool isDownloading_;
    QSharedPointer<CameraDevice> camera;
    QMutex mutex;

    int completedCount_;
    int pendingCount_;
};
