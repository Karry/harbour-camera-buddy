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

#include <cassert>

class DownloadRunnable : public QRunnable {
private:
    DownloadModel* model;
    int itemIndex;
    QSharedPointer<CameraDevice> cameraPtr;
    DownloadItem item;

public:
    DownloadRunnable(DownloadModel* m, int index, QSharedPointer<CameraDevice> cam, const DownloadItem& itm)
        : model(m), itemIndex(index), cameraPtr(cam), item(itm) {
        setAutoDelete(true);
    }

    void run() override {
        // Copy member variables to local variables for lambda capture
        DownloadModel* modelPtr = model;
        int index = itemIndex;

        try {
            // Ensure directory exists
            QDir dir(QFileInfo(item.filePath).absolutePath());
            if (!dir.exists()) {
                dir.mkpath(".");
            }

            QFile file(item.filePath);
            if (file.exists()) {
                QTimer::singleShot(0, modelPtr, [modelPtr, index]() {
                    modelPtr->onDownloadItemFinished(index, false,
                        QString("File already exists"));
                });
                return;
            }

            // Write file
            if (!file.open(QIODevice::WriteOnly)) {
                QString errorMsg = file.errorString();
                QTimer::singleShot(0, modelPtr, [modelPtr, index, errorMsg]() {
                    modelPtr->onDownloadItemFinished(index, false,
                        QString("Failed to create file: %1").arg(errorMsg));
                });
                return;
            }

            if (!downloadPhotoData(
                cameraPtr->camera,
                cameraPtr->context,
                item.photo->folder,
                item.photo->name,
                item.photo->size,
                file)) {

                file.close();
                file.remove();
                QTimer::singleShot(0, modelPtr, [modelPtr, index]() {
                    modelPtr->onDownloadItemFinished(index, false, "Failed to download photo data");
                });
                return;
            }

            file.close();

            QTimer::singleShot(0, modelPtr, [modelPtr, index]() {
                modelPtr->onDownloadItemFinished(index, true, "");
            });


        } catch (const std::exception& e) {
            QString errorMsg = QString::fromUtf8(e.what());
            QTimer::singleShot(0, modelPtr, [modelPtr, index, errorMsg]() {
                modelPtr->onDownloadItemFinished(index, false, QString("Exception: %1").arg(errorMsg));
            });
        }
    }

private:
    bool downloadPhotoData(Camera* camera, GPContext* context, const QString& folder, const QString& filename, uint64_t file_size, QFile &target) {
        // try to read using gp_camera_file_read first to handle large files
        QByteArray buff(1024 * 1024, Qt::Initialization::Uninitialized); // 1 MB buffer
        int ret;
        uint64_t retrieved = 0;
        while (true) {
            uint64_t size = buff.size();
            ret = gp_camera_file_read(camera, folder.toUtf8().constData(), filename.toUtf8().constData(),
                GP_FILE_TYPE_NORMAL, retrieved, buff.data(), &size, context);

            if (ret==GP_ERROR_NOT_SUPPORTED) {
                qWarning() << "gp_camera_file_read not supported, falling back to gp_camera_file_get";
                break; // fallback to gp_camera_file_get
            }
            if (ret != GP_OK) {
                qWarning() << "gp_camera_file_read failed for" << folder + "/" + filename << ", gphoto2 error code:" << ret;
                return false;
            }
            if (size == 0) {
                if (retrieved != file_size) {
                    qWarning() << "gp_camera_file_read returned" << retrieved << "bytes for file" << folder + "/" + filename << "but expected" << file_size;
                } else {
                    qDebug() << "gp_camera_file_read returned" << retrieved << "bytes for file" << folder + "/" + filename;
                }
                return true; // completed
            }
            if (int64_t(size) != target.write(buff.constData(), size)) {
                qWarning() << "Failed to write complete file chunk";
                return false;
            }
            retrieved += size;

            // Progress if total known
            if (file_size > 0) {
                double progress = double(retrieved) / double(file_size);
                auto modelPtr = model;
                auto indexCopy = itemIndex;
                QTimer::singleShot(0, model, [modelPtr, indexCopy, progress]() {
                    modelPtr->onDownloadItemProgress(indexCopy, progress);
                });
            }
        }

        // fallback
        CameraFile *file = nullptr;
        ret = gp_file_new(&file);
        if (ret != GP_OK) {
            return false;
        }

        ret = gp_camera_file_get(camera, folder.toUtf8().constData(), filename.toUtf8().constData(),
                                GP_FILE_TYPE_NORMAL, file, context);

        QByteArray data;
        if (ret == GP_OK) {
            const char* fileData = nullptr;
            unsigned long fileSize = 0;
            if (gp_file_get_data_and_size(file, &fileData, &fileSize) == GP_OK) {
                data = QByteArray(fileData, fileSize);
                if (data.size() != target.write(data)) {
                    qWarning() << "Failed to write complete file";
                    return false;
                }
            } else {
                qWarning() << "Failed to get photo data and size for" << folder + "/" + filename;
                return false;
            }
        } else {
            qWarning() << "Failed to get photo file for" << folder + "/" + filename << ", gphoto2 error code:" << ret;
            return false;
        }

        gp_file_free(file);
        return true;
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
    case ThumbnailAvailableRole:
        return !item.photo->thumbnailAvailable;
    case ThumbnailBase64Role:
        return QString::fromUtf8(item.photo->thumbnailData.toBase64());
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
    roles[ThumbnailAvailableRole] = "thumbnailAvailable";
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

void DownloadModel::onDownloadItemProgress(int index, qreal progress) {
    if (index < 0 || index >= downloads.size()) {
        return;
    }

    DownloadItem &item = downloads[index];
    if (item.status == Downloading) {
        item.progress = progress;
        auto modelIndex = this->index(index);
        emit dataChanged(modelIndex, modelIndex, {ProgressRole});
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

