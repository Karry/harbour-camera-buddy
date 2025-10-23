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

#include "DownloadModel.h"

#include <QDebug>
#include <QTimer>
#include <QDir>
#include <QFile>
#include <QRunnable>
#include <QThreadPool>
#include <QStandardPaths>

class DownloadRunnable : public QRunnable {
private:
    DownloadModel* model;
    int itemIndex;
    QSharedPointer<CameraDevice> camera;
    DownloadItem item;

public:
    DownloadRunnable(DownloadModel* m, int index, QSharedPointer<CameraDevice> cam, const DownloadItem& itm)
        : model(m), itemIndex(index), camera(cam), item(itm) {
        setAutoDelete(true);
    }

    void run() override {
        // Copy member variables to local variables for lambda capture
        DownloadModel* modelPtr = model;
        int index = itemIndex;

        try {
            QByteArray photoData = downloadPhotoData(camera->camera, camera->context,
                                                   item.photo->folder, item.photo->name);

            if (!photoData.isEmpty()) {
                // Ensure directory exists
                QDir dir(QFileInfo(item.filePath).absolutePath());
                if (!dir.exists()) {
                    dir.mkpath(".");
                }

                // Write file
                QFile file(item.filePath);
                if (file.open(QIODevice::WriteOnly)) {
                    qint64 written = file.write(photoData);
                    file.close();

                    if (written == photoData.size()) {
                        QTimer::singleShot(0, modelPtr, [modelPtr, index]() {
                            modelPtr->onDownloadItemFinished(index, true, "");
                        });
                        return;
                    } else {
                        QTimer::singleShot(0, modelPtr, [modelPtr, index]() {
                            modelPtr->onDownloadItemFinished(index, false, "Failed to write complete file");
                        });
                        return;
                    }
                } else {
                    QString errorMsg = file.errorString();
                    QTimer::singleShot(0, modelPtr, [modelPtr, index, errorMsg]() {
                        modelPtr->onDownloadItemFinished(index, false,
                            QString("Failed to create file: %1").arg(errorMsg));
                    });
                    return;
                }
            } else {
                QTimer::singleShot(0, modelPtr, [modelPtr, index]() {
                    modelPtr->onDownloadItemFinished(index, false, "Failed to download photo data");
                });
            }
        } catch (const std::exception& e) {
            QString errorMsg = QString::fromUtf8(e.what());
            QTimer::singleShot(0, modelPtr, [modelPtr, index, errorMsg]() {
                modelPtr->onDownloadItemFinished(index, false, QString("Exception: %1").arg(errorMsg));
            });
        }
    }

private:
    QByteArray downloadPhotoData(Camera* camera, GPContext* context, const QString& folder, const QString& filename) {
        CameraFile *file = nullptr;
        int ret = gp_file_new(&file);
        if (ret != GP_OK) {
            return QByteArray();
        }

        ret = gp_camera_file_get(camera, folder.toUtf8().constData(), filename.toUtf8().constData(),
                                GP_FILE_TYPE_NORMAL, file, context);

        QByteArray data;
        if (ret == GP_OK) {
            const char* fileData = nullptr;
            unsigned long fileSize = 0;
            if (gp_file_get_data_and_size(file, &fileData, &fileSize) == GP_OK) {
                data = QByteArray(fileData, fileSize);
            } else {
                qWarning() << "Failed to get photo data and size for" << folder + "/" + filename;
            }
        } else {
            qWarning() << "Failed to get photo file for" << folder + "/" + filename << ", gphoto2 error code:" << ret;
        }

        gp_file_free(file);
        return data;
    }
};

DownloadModel::DownloadModel(QObject *parent)
    : QAbstractListModel(parent)
    , isDownloading_(false)
    , completedCount_(0)
    , pendingCount_(0)
{
}

DownloadModel::~DownloadModel() {
    cancelDownload();
}

int DownloadModel::rowCount(const QModelIndex &parent) const {
    Q_UNUSED(parent)
    return downloads.size();
}

QVariant DownloadModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() >= downloads.size()) {
        return QVariant();
    }

    const DownloadItem &item = downloads.at(index.row());

    switch (role) {
    case FileNameRole:
        return item.fileName;
    case FilePathRole:
        return item.filePath;
    case ThumbnailBase64Role:
        return item.thumbnailBase64;
    case StatusRole:
        return static_cast<int>(item.status);
    case ProgressRole:
        return item.progress;
    case ErrorMessageRole:
        return item.errorMessage;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> DownloadModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[FileNameRole] = "fileName";
    roles[FilePathRole] = "filePath";
    roles[ThumbnailBase64Role] = "thumbnailBase64";
    roles[StatusRole] = "status";
    roles[ProgressRole] = "progress";
    roles[ErrorMessageRole] = "errorMessage";
    return roles;
}

void DownloadModel::setDownloadPath(const QString &path) {
    if (downloadPath_ != path) {
        downloadPath_ = path;
        emit downloadPathChanged();
    }
}

int DownloadModel::completedCount() const {
    return completedCount_;
}

int DownloadModel::pendingCount() const {
    return pendingCount_;
}

void DownloadModel::startDownload(const QVariantList &selectedPhotos) {
    if (isDownloading_) {
        return;
    }

    // Clear previous downloads
    beginResetModel();
    downloads.clear();
    endResetModel();

    if (selectedPhotos.isEmpty()) {
        return;
    }

    if (!camera) {
        emit downloadError("Camera not available for download");
        return;
    }

    if (downloadPath_.isEmpty()) {
        emit downloadError("Download path not set");
        return;
    }

    // Create download items from selected photos
    beginInsertRows(QModelIndex(), 0, selectedPhotos.size() - 1);

    for (const QVariant &photoVar : selectedPhotos) {
        // Each photo should be a QSharedPointer<PhotoInfo> wrapped in QVariant
        auto photo = photoVar.value<QSharedPointer<PhotoInfo>>();
        if (!photo) {
            qWarning() << "Invalid photo in selected list";
            continue;
        }

        DownloadItem item;
        item.photo = photo;
        item.fileName = generateFileName(photo);
        item.filePath = QDir(downloadPath_).filePath(item.fileName);
        item.thumbnailBase64 = photo->thumbnailData.toBase64();
        item.status = Pending;
        item.progress = 0.0;

        downloads.append(item);
    }

    endInsertRows();

    isDownloading_ = true;
    updateCounts();
    emit isDownloadingChanged();
    emit totalCountChanged();

    // Start downloading
    processNextDownload();
}

void DownloadModel::cancelDownload() {
    if (isDownloading_ && camera && camera->threadPool) {
        camera->threadPool->waitForDone();
        isDownloading_ = false;
        emit isDownloadingChanged();
    }
}

void DownloadModel::retryFailed() {
    if (isDownloading_) {
        return;
    }

    // Reset failed items to pending
    for (int i = 0; i < downloads.size(); ++i) {
        if (downloads[i].status == Error) {
            downloads[i].status = Pending;
            downloads[i].progress = 0.0;
            downloads[i].errorMessage.clear();

            auto modelIndex = index(i);
            emit dataChanged(modelIndex, modelIndex);
        }
    }

    updateCounts();

    if (pendingCount() > 0) {
        isDownloading_ = true;
        emit isDownloadingChanged();
        processNextDownload();
    }
}

void DownloadModel::onDownloadItemFinished(int index, bool success, const QString &errorMessage) {
    if (index < 0 || index >= downloads.size()) {
        return;
    }

    DownloadItem &item = downloads[index];

    if (success) {
        item.status = Completed;
        item.progress = 1.0;
        item.errorMessage.clear();
    } else {
        item.status = Error;
        item.errorMessage = errorMessage;
    }

    auto modelIndex = this->index(index);
    emit dataChanged(modelIndex, modelIndex);

    updateCounts();
    emit downloadProgress(completedCount(), totalCount());

    // Check if all downloads are finished
    if (pendingCount() == 0) {
        isDownloading_ = false;
        emit isDownloadingChanged();
        emit downloadFinished();
    } else {
        // Continue with next download
        processNextDownload();
    }
}

void DownloadModel::updateCounts() {
    int completed = 0;
    int pending = 0;

    for (const DownloadItem &item : downloads) {
        switch (item.status) {
        case Completed:
            completed++;
            break;
        case Pending:
        case Downloading:
            pending++;
            break;
        default:
            break;
        }
    }

    if (completedCount_ != completed) {
        completedCount_ = completed;
        emit completedCountChanged();
    }

    if (pendingCount_ != pending) {
        pendingCount_ = pending;
        emit pendingCountChanged();
    }
}

void DownloadModel::processNextDownload() {
    if (!isDownloading_ || !camera || !camera->threadPool) {
        return;
    }

    // Find next pending download
    for (int i = 0; i < downloads.size(); ++i) {
        if (downloads[i].status == Pending) {
            downloads[i].status = Downloading;

            auto modelIndex = index(i);
            emit dataChanged(modelIndex, modelIndex);

            // Use camera's dedicated thread pool for gphoto2 operations
            camera->threadPool->start(new DownloadRunnable(this, i, camera, downloads[i]));
            break;
        }
    }
}

QString DownloadModel::generateFileName(const QSharedPointer<PhotoInfo> &photo) {
    QString baseName = photo->name;

    // Add timestamp prefix if desired (optional)
    QString timestamp = photo->dateTime.toString("yyyyMMdd_hhmmss");

    // For now, just use the original filename
    // You could add timestamp or other prefixes here if needed
    return baseName;
}

void DownloadModel::setCamera(const QVariant &cameraVariant) {
    camera = cameraVariant.value<QSharedPointer<CameraDevice>>();
}

