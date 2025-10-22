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

#include "PhotosModel.h"
#include <QDebug>
#include <QThread>
#include <QMutexLocker>
#include <QStandardPaths>
#include <QThreadPool>
#include <QRunnable>

QString PhotoInfo::toString() const {
    return QString("%1/%2 (%3, %4)")
        .arg(folder)
        .arg(name)
        .arg(sizeString())
        .arg(dateTime.toString());
}

QString PhotoInfo::sizeString() const {
    if (size < 1024) {
        return QString("%1 B").arg(size);
    } else if (size < 1024 * 1024) {
        return QString("%1 KB").arg(size / 1024);
    } else {
        return QString("%1 MB").arg(size / (1024 * 1024));
    }
}

PhotosModel::PhotosModel(QObject *parent)
    : QAbstractListModel(parent)
    , cameraModel(nullptr)
    , cameraIndex(-1)
    , loading(false)
    , thumbnailTimer(new QTimer(this))
{
    thumbnailTimer->setSingleShot(true);
    thumbnailTimer->setInterval(100); // Small delay for thumbnail loading

    connect(thumbnailTimer, &QTimer::timeout, this, [this]() {
        // Process thumbnail loading queue if needed
    });
}

PhotosModel::~PhotosModel() {
    photos.clear();
}

int PhotosModel::rowCount(const QModelIndex &parent) const {
    Q_UNUSED(parent)
    return photos.size();
}

QVariant PhotosModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() >= photos.size()) {
        return QVariant();
    }

    auto photo = photos.at(index.row());
    if (!photo) {
        return QVariant();
    }

    switch (role) {
    case NameRole:
        return photo->name;
    case FolderRole:
        return photo->folder;
    case FullPathRole:
        return photo->fullPath;
    case DateTimeRole:
        return photo->dateTime;
    case SizeRole:
        return photo->size;
    case SizeStringRole:
        return photo->sizeString();
    case MimeTypeRole:
        return photo->mimeType;
    case DimensionsRole:
        return QSize(photo->dimensions.width(), photo->dimensions.height());
    case SelectedRole:
        return photo->selected;
    case ThumbnailAvailableRole:
        return photo->thumbnail_available;
    case ThumbnailDataRole:
        return photo->thumbnailData;
    case ThumbnailBase64Role:
        return QString::fromUtf8(photo->thumbnailData.toBase64());
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> PhotosModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name";
    roles[FolderRole] = "folder";
    roles[FullPathRole] = "fullPath";
    roles[DateTimeRole] = "dateTime";
    roles[SizeRole] = "size";
    roles[SizeStringRole] = "sizeString";
    roles[MimeTypeRole] = "mimeType";
    roles[DimensionsRole] = "dimensions";
    roles[SelectedRole] = "selected";
    roles[ThumbnailAvailableRole] = "thumbnailAvailable";
    roles[ThumbnailDataRole] = "thumbnailData";
    roles[ThumbnailBase64Role] = "thumbnailBase64";
    return roles;
}

Qt::ItemFlags PhotosModel::flags(const QModelIndex &index) const {
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

bool PhotosModel::setData(const QModelIndex &index, const QVariant &value, int role) {
    if (!index.isValid() || index.row() >= photos.size()) {
        return false;
    }

    auto photo = photos.at(index.row());
    if (!photo) {
        return false;
    }

    if (role == SelectedRole) {
        if (photo->selected == value.toBool()) {
            return true;
        }
        photo->selected = value.toBool();
        emit dataChanged(index, index, {SelectedRole});
        emit selectedCountChanged();
        return true;
    }

    return false;
}

void PhotosModel::setCameraModel(CameraModel* model) {
    if (cameraModel == model) {
        return;
    }

    cameraModel = model;
    refresh();
    emit cameraModelChanged();
}

void PhotosModel::setCameraIndex(int index) {
    if (cameraIndex == index) {
        return;
    }

    cameraIndex = index;

    if (cameraModel && index >= 0 && index < cameraModel->rowCount()) {
        cameraName = cameraModel->getCameraName(index);
    } else {
        cameraName.clear();
    }

    emit cameraIndexChanged();
    emit cameraNameChanged();

    refresh();
}

void PhotosModel::refresh() {
    qDebug() << "PhotosModel::refresh() called";
    qDebug() << "  loading:" << loading;
    qDebug() << "  cameraIndex:" << cameraIndex;
    qDebug() << "  cameraModel:" << (cameraModel ? "valid" : "null");

    if (loading || cameraIndex < 0 || !cameraModel) {
        qDebug() << "PhotosModel::refresh() - Skipping: loading=" << loading << ", cameraIndex=" << cameraIndex << ", cameraModel=" << (cameraModel ? "valid" : "null");
        return;
    }

    qDebug() << "PhotosModel::refresh() - Setting loading state and clearing photos";
    loading = true;
    emit loadingChanged();

    // Clear existing photos
    beginResetModel();
    photos.clear();
    endResetModel();
    emit countChanged();
    emit selectedCountChanged();

    qDebug() << "PhotosModel::refresh() - Starting background photo loading thread";

    // Get camera to use its dedicated thread pool
    if (!cameraModel || cameraIndex < 0) {
        qDebug() << "PhotosModel::refresh() - Invalid camera model or index";
        return;
    }

    auto camera = cameraModel->getCameraAt(cameraIndex);
    if (!camera || !camera->threadPool) {
        qDebug() << "PhotosModel::refresh() - Camera not available or thread pool not initialized";
        return;
    }

    // Load photos in camera's dedicated single-threaded pool
    class LoadPhotosRunnable : public QRunnable {
    private:
        PhotosModel* model;
    public:
        LoadPhotosRunnable(PhotosModel* m) : model(m) { setAutoDelete(true); }
        void run() override { model->loadPhotosFromCamera(); }
    };
    camera->threadPool->start(new LoadPhotosRunnable(this));
}

void PhotosModel::loadPhotosFromCamera() {
    qDebug() << "PhotosModel::loadPhotosFromCamera() - Background thread started";
    QMutexLocker locker(&loadMutex);

    qDebug() << "  cameraModel:" << (cameraModel ? "valid" : "null");
    qDebug() << "  cameraIndex:" << cameraIndex;

    if (!cameraModel || cameraIndex < 0) {
        qDebug() << "PhotosModel::loadPhotosFromCamera() - Invalid camera model or index";
        loading = false;
        emit loadingChanged();
        return;
    }

    qDebug() << "PhotosModel::loadPhotosFromCamera() - Getting camera at index" << cameraIndex;
    auto camera = cameraModel->getCameraAt(cameraIndex);
    if (!camera || !camera->camera || !camera->context) {
        qDebug() << "PhotosModel::loadPhotosFromCamera() - Camera not available or not connected";
        qDebug() << "  camera pointer:" << (camera ? "valid" : "null");
        if (camera) {
            qDebug() << "  camera->camera:" << (camera->camera ? "valid" : "null");
            qDebug() << "  camera->context:" << (camera->context ? "valid" : "null");
        }
        loading = false;
        emit loadingChanged();
        emit error(QObject::tr("Camera not available or not connected"));
        return;
    }

    qDebug() << "PhotosModel::loadPhotosFromCamera() - Camera is valid, scanning for photos";
    try {
        auto newPhotos = scanCameraFolders(camera->camera, camera->context);
        qDebug() << "PhotosModel::loadPhotosFromCamera() - Found" << newPhotos.size() << "photos";

        // Update model on main thread
        QTimer::singleShot(0, this, [this, newPhotos]() {
            qDebug() << "PhotosModel::loadPhotosFromCamera() - Updating UI with" << newPhotos.size() << "photos";
            beginResetModel();
            photos = newPhotos;
            endResetModel();

            loading = false;
            emit loadingChanged();
            emit countChanged();
            emit selectedCountChanged();
            qDebug() << "PhotosModel::loadPhotosFromCamera() - Photo loading completed successfully";
        });

    } catch (const std::exception& e) {
        qDebug() << "PhotosModel::loadPhotosFromCamera() - Exception occurred:" << e.what();
        loading = false;
        emit loadingChanged();
        emit error(QString("Failed to load photos: %1").arg(e.what()));
    }
}

QList<QSharedPointer<PhotoInfo>> PhotosModel::scanCameraFolders(Camera* camera, GPContext* context) {
    qDebug() << "PhotosModel::scanCameraFolders() - Starting folder scan";

    // Debug camera capabilities
    CameraAbilities abilities;
    int ret = gp_camera_get_abilities(camera, &abilities);
    if (ret == GP_OK) {
        qDebug() << "PhotosModel::scanCameraFolders() - Camera model:" << abilities.model;
        qDebug() << "PhotosModel::scanCameraFolders() - Camera library:" << abilities.library;
        qDebug() << "PhotosModel::scanCameraFolders() - Camera device type:" << abilities.device_type;
        qDebug() << "PhotosModel::scanCameraFolders() - Camera operations:" << abilities.operations;
        qDebug() << "PhotosModel::scanCameraFolders() - Camera file operations:" << abilities.file_operations;
        qDebug() << "PhotosModel::scanCameraFolders() - Camera folder operations:" << abilities.folder_operations;
    } else {
        qDebug() << "PhotosModel::scanCameraFolders() - Failed to get camera abilities, error:" << ret;
    }

    // Try to get camera summary for more detailed information
    CameraText summary;
    ret = gp_camera_get_summary(camera, &summary, context);
    if (ret == GP_OK) {
        qDebug() << "PhotosModel::scanCameraFolders() - Camera summary:" << summary.text;
    } else {
        qDebug() << "PhotosModel::scanCameraFolders() - Failed to get camera summary, error:" << ret;
    }

    // Try to get camera about text
    CameraText about;
    ret = gp_camera_get_about(camera, &about, context);
    if (ret == GP_OK) {
        qDebug() << "PhotosModel::scanCameraFolders() - Camera about:" << about.text;
    } else {
        qDebug() << "PhotosModel::scanCameraFolders() - Failed to get camera about, error:" << ret;
    }

    QList<QSharedPointer<PhotoInfo>> allPhotos;

    // Start with root folder "/"
    QStringList foldersToScan = {"/"};
    QStringList processedFolders;

    qDebug() << "PhotosModel::scanCameraFolders() - Initial folders to scan:" << foldersToScan;

    while (!foldersToScan.isEmpty()) {
        QString currentFolder = foldersToScan.takeFirst();
        qDebug() << "PhotosModel::scanCameraFolders() - Processing folder:" << currentFolder;

        if (processedFolders.contains(currentFolder)) {
            qDebug() << "PhotosModel::scanCameraFolders() - Folder already processed, skipping:" << currentFolder;
            continue;
        }
        processedFolders.append(currentFolder);

        // Scan current folder for photos
        qDebug() << "PhotosModel::scanCameraFolders() - Scanning folder for photos:" << currentFolder;
        auto folderPhotos = scanFolder(camera, context, currentFolder);
        qDebug() << "PhotosModel::scanCameraFolders() - Found" << folderPhotos.size() << "photos in folder:" << currentFolder;
        allPhotos.append(folderPhotos);

        // Get subfolders
        qDebug() << "PhotosModel::scanCameraFolders() - Looking for subfolders in:" << currentFolder;
        CameraList *folderList = nullptr;
        int ret = gp_list_new(&folderList);
        if (ret == GP_OK) {
            qDebug() << "PhotosModel::scanCameraFolders() - Created folder list successfully, calling gp_camera_folder_list_folders";
            ret = gp_camera_folder_list_folders(camera, currentFolder.toUtf8().constData(), folderList, context);
            qDebug() << "PhotosModel::scanCameraFolders() - gp_camera_folder_list_folders returned:" << ret;

            if (ret == GP_OK) {
                int folderCount = gp_list_count(folderList);
                qDebug() << "PhotosModel::scanCameraFolders() - Found" << folderCount << "subfolders in:" << currentFolder;

                // Log all found subfolders
                for (int i = 0; i < folderCount; i++) {
                    const char* folderName = nullptr;
                    if (gp_list_get_name(folderList, i, &folderName) == GP_OK) {
                        QString subFolder = currentFolder;
                        if (!subFolder.endsWith("/")) {
                            subFolder += "/";
                        }
                        subFolder += QString::fromUtf8(folderName);
                        qDebug() << "PhotosModel::scanCameraFolders() - Found subfolder:" << subFolder;

                        if (!processedFolders.contains(subFolder)) {
                            foldersToScan.append(subFolder);
                            qDebug() << "PhotosModel::scanCameraFolders() - Added to scan queue:" << subFolder;
                        } else {
                            qDebug() << "PhotosModel::scanCameraFolders() - Subfolder already processed, skipping:" << subFolder;
                        }
                    } else {
                        qDebug() << "PhotosModel::scanCameraFolders() - Failed to get name for subfolder index:" << i;
                    }
                }
            } else {
                qDebug() << "PhotosModel::scanCameraFolders() - Failed to list folders in" << currentFolder << ", gphoto2 error code:" << ret;
                // Try to get human-readable error
                const char* errorMsg = gp_result_as_string(ret);
                qDebug() << "PhotosModel::scanCameraFolders() - Error message:" << errorMsg;
            }
            gp_list_free(folderList);
        } else {
            qDebug() << "PhotosModel::scanCameraFolders() - Failed to create folder list, gphoto2 error code:" << ret;
            const char* errorMsg = gp_result_as_string(ret);
            qDebug() << "PhotosModel::scanCameraFolders() - Error message:" << errorMsg;
        }
    }

    qDebug() << "PhotosModel::scanCameraFolders() - Folder scan completed, total photos found:" << allPhotos.size();
    return allPhotos;
}

QList<QSharedPointer<PhotoInfo>> PhotosModel::scanFolder(Camera* camera, GPContext* context, const QString& folder) {
    qDebug() << "PhotosModel::scanFolder() - Scanning folder:" << folder;
    QList<QSharedPointer<PhotoInfo>> photos;

    CameraList *fileList = nullptr;
    int ret = gp_list_new(&fileList);
    if (ret != GP_OK) {
        qDebug() << "PhotosModel::scanFolder() - Failed to create file list for folder:" << folder << ", error:" << ret;
        return photos;
    }

    ret = gp_camera_folder_list_files(camera, folder.toUtf8().constData(), fileList, context);
    if (ret != GP_OK) {
        qDebug() << "PhotosModel::scanFolder() - Failed to list files in folder:" << folder << ", error:" << ret;
        gp_list_free(fileList);
        return photos;
    }

    int fileCount = gp_list_count(fileList);
    qDebug() << "PhotosModel::scanFolder() - Found" << fileCount << "files in folder:" << folder;

    for (int i = 0; i < fileCount; i++) {
        const char* fileName = nullptr;
        if (gp_list_get_name(fileList, i, &fileName) == GP_OK) {
            QString name = QString::fromUtf8(fileName);
            qDebug() << "PhotosModel::scanFolder() - Processing file:" << name;

            // Filter for image files
            QString lowerName = name.toLower();
            if (lowerName.endsWith(".jpg") || lowerName.endsWith(".jpeg") ||
                lowerName.endsWith(".raw") || lowerName.endsWith(".cr2") ||
                lowerName.endsWith(".nef") || lowerName.endsWith(".arw") ||
                lowerName.endsWith(".dng") || lowerName.endsWith(".png") ||
                lowerName.endsWith(".tiff") || lowerName.endsWith(".tif")) {

                qDebug() << "PhotosModel::scanFolder() - File matches image filter, creating PhotoInfo for:" << name;
                auto photo = QSharedPointer<PhotoInfo>::create();
                photo->name = name;
                photo->folder = folder;
                photo->fullPath = folder + (folder.endsWith("/") ? "" : "/") + name;

                // Try to get additional info
                try {
                    qDebug() << "PhotosModel::scanFolder() - Getting detailed photo info for:" << name;
                    *photo = getPhotoInfo(camera, context, folder, name);
                    qDebug() << "PhotosModel::scanFolder() - Successfully got photo info for:" << name;
                } catch (const std::exception& e) {
                    qDebug() << "PhotosModel::scanFolder() - Failed to get detailed photo info for:" << name << ", error:" << e.what();
                    // Keep basic info if detailed info fails
                } catch (...) {
                    qDebug() << "PhotosModel::scanFolder() - Failed to get detailed photo info for:" << name << ", unknown error";
                    // Keep basic info if detailed info fails
                }

                photos.append(photo);
                qDebug() << "PhotosModel::scanFolder() - Added photo to list:" << name;
            } else {
                qDebug() << "PhotosModel::scanFolder() - File does not match image filter, skipping:" << name;
            }
        } else {
            qDebug() << "PhotosModel::scanFolder() - Failed to get filename for index:" << i;
        }
    }

    gp_list_free(fileList);
    qDebug() << "PhotosModel::scanFolder() - Completed scanning folder:" << folder << ", found" << photos.size() << "photos";
    return photos;
}

PhotoInfo PhotosModel::getPhotoInfo(Camera* camera, GPContext* context, const QString& folder, const QString& filename) {
    PhotoInfo info;
    info.name = filename;
    info.folder = folder;
    info.fullPath = folder + (folder.endsWith("/") ? "" : "/") + filename;

    // Try to get file info
    CameraFileInfo fileInfo;
    int ret = gp_camera_file_get_info(camera, folder.toUtf8().constData(), filename.toUtf8().constData(), &fileInfo, context);
    if (ret == GP_OK) {
        if (fileInfo.file.fields & GP_FILE_INFO_SIZE) {
            info.size = fileInfo.file.size;
        }
        if (fileInfo.file.fields & GP_FILE_INFO_MTIME) {
            info.dateTime = QDateTime::fromTime_t(fileInfo.file.mtime);
        }
        if (fileInfo.file.fields & GP_FILE_INFO_TYPE) {
            info.mimeType = QString::fromUtf8(fileInfo.file.type);
        }
        if (fileInfo.file.fields & GP_FILE_INFO_WIDTH && fileInfo.file.fields & GP_FILE_INFO_HEIGHT) {
            info.dimensions = QSize(fileInfo.file.width, fileInfo.file.height);
        }
    }

    return info;
}

void PhotosModel::selectAll(bool select) {
    if (photos.isEmpty()) {
        return;
    }

    for (auto& photo : photos) {
        photo->selected = select;
    }

    emit dataChanged(index(0), index(photos.size() - 1), {SelectedRole});
    emit selectedCountChanged();
}

void PhotosModel::selectJpegs() {
    if (photos.isEmpty()) {
        return;
    }

    for (auto& photo : photos) {
        QString lowerName = photo->name.toLower();
        if (lowerName.endsWith(".jpg") || lowerName.endsWith(".jpeg")) {
            photo->selected = true;
        } else {
            photo->selected = false;
        }
    }

    emit dataChanged(index(0), index(photos.size() - 1), {SelectedRole});
    emit selectedCountChanged();
}

void PhotosModel::selectPhoto(int index, bool select) {
    if (index < 0 || index >= photos.size()) {
        return;
    }
    if (photos[index]->selected == select) {
        return;
    }

    photos[index]->selected = select;

    auto modelIndex = this->index(index);
    emit dataChanged(modelIndex, modelIndex, {SelectedRole});
    emit selectedCountChanged();
}

void PhotosModel::toggleSelection(int index) {
    if (index < 0 || index >= photos.size()) {
        return;
    }

    selectPhoto(index, !photos[index]->selected);
}

int PhotosModel::getSelectedCount() const {
    int count = 0;
    for (const auto& photo : photos) {
        if (photo->selected) {
            count++;
        }
    }
    return count;
}

QSharedPointer<PhotoInfo> PhotosModel::getPhotoAt(int index) const {
    if (index < 0 || index >= photos.size()) {
        return QSharedPointer<PhotoInfo>();
    }
    return photos[index];
}

QList<QSharedPointer<PhotoInfo>> PhotosModel::getSelectedPhotos() const {
    QList<QSharedPointer<PhotoInfo>> selected;
    for (const auto& photo : photos) {
        if (photo->selected) {
            selected.append(photo);
        }
    }
    return selected;
}

void PhotosModel::loadThumbnail(int index) {
    if (index < 0 || index >= photos.size() || !cameraModel || cameraIndex < 0) {
        return;
    }

    auto photo = photos[index];
    if (photo->thumbnail_available) {
        return; // Already loaded
    }

    auto camera = cameraModel->getCameraAt(cameraIndex);
    if (!camera || !camera->camera || !camera->context) {
        return;
    }

    // Load thumbnail in background
    class LoadThumbnailRunnable : public QRunnable {
    private:
        PhotosModel* model;
        int index;
        QSharedPointer<PhotoInfo> photo;
        QSharedPointer<CameraDevice> camera;
    public:
        LoadThumbnailRunnable(PhotosModel* m, int idx, QSharedPointer<PhotoInfo> p, QSharedPointer<CameraDevice> c)
            : model(m), index(idx), photo(p), camera(c) { setAutoDelete(true); }

        void run() override {
            try {
                QByteArray thumbnailData = model->loadThumbnailData(camera->camera, camera->context, photo->folder, photo->name);

                auto photoCopy = photo; // Capture shared pointer
                auto modelCopy = model;
                auto indexCopy = index;

                QTimer::singleShot(0, model, [photoCopy, modelCopy, indexCopy, thumbnailData]() {
                    photoCopy->thumbnailData = thumbnailData;
                    photoCopy->thumbnail_available = !thumbnailData.isEmpty();

                    auto modelIndex = modelCopy->index(indexCopy);
                    emit modelCopy->dataChanged(modelIndex, modelIndex,
                        {ThumbnailAvailableRole, ThumbnailDataRole, ThumbnailBase64Role});
                    emit modelCopy->thumbnailLoaded(indexCopy);
                    qDebug() << "Thumbnail loaded for photo at index" << indexCopy; //  << "base64:" << QString::fromUtf8(thumbnailData.toBase64());
                });

            } catch (...) {
                // Thumbnail loading failed, but don't emit error for this
                qWarning() << "Failed to load thumbnail for photo at index" << index;
            }
        }
    };

    qDebug() << "Loading thumbnail for photo at index" << index << ":" << photo->fullPath;

    // Use camera's dedicated single-threaded pool for gphoto2 operations
    if (!camera->threadPool) {
        qWarning() << "Camera thread pool not initialized for thumbnail loading";
        return;
    }

    camera->threadPool->start(new LoadThumbnailRunnable(this, index, photo, camera));
}

QByteArray PhotosModel::loadThumbnailData(Camera* camera, GPContext* context, const QString& folder, const QString& filename) {
    CameraFile *file = nullptr;
    int ret = gp_file_new(&file);
    if (ret != GP_OK) {
        return QByteArray();
    }

    ret = gp_camera_file_get(camera, folder.toUtf8().constData(), filename.toUtf8().constData(),
                            GP_FILE_TYPE_PREVIEW, file, context);

    QByteArray data;
    if (ret == GP_OK) {
        const char* fileData = nullptr;
        unsigned long fileSize = 0;
        if (gp_file_get_data_and_size(file, &fileData, &fileSize) == GP_OK) {
            data = QByteArray(fileData, fileSize);
        } else {
            qWarning() << "Failed to get thumbnail data and size for" << folder + "/" + filename;
        }
    } else {
        qWarning() << "Failed to get thumbnail file for" << folder + "/" + filename << ", gphoto2 error code:" << ret;
    }

    gp_file_free(file);
    return data;
}
