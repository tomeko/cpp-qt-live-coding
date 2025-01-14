#include "filewatcher.h"
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QLoggingCategory>
#include <QRegularExpression>

Q_LOGGING_CATEGORY(filewatcherCategory, "filewatcher");

FileWatcher::FileWatcher(QObject* parent)
    : QObject(parent)
    , m_fileUrl(QLatin1String(""))
    , m_enabled(true)
    , m_recursive(false)
{
    connect(this, &FileWatcher::fileUrlChanged, this, &FileWatcher::updateWatchedFile);
    connect(this, &FileWatcher::enabledChanged, this, &FileWatcher::updateWatchedFile);
    connect(this, &FileWatcher::recursiveChanged, this, &FileWatcher::updateWatchedFile);
    connect(this, &FileWatcher::nameFiltersChanged, this, &FileWatcher::updateWatchedFile);
    connect(&m_fileSystemWatcher, &QFileSystemWatcher::fileChanged, this, &FileWatcher::onWatchedFileChanged);
    connect(&m_fileSystemWatcher, &QFileSystemWatcher::directoryChanged, this, &FileWatcher::onWatchedDirectoryChanged);
}

QUrl FileWatcher::fileUrl() const
{
    return m_fileUrl;
}

bool FileWatcher::enabled() const
{
    return m_enabled;
}

bool FileWatcher::recursive() const
{
    return m_recursive;
}

QStringList FileWatcher::nameFilters() const
{
    return m_nameFilters;
}

void FileWatcher::setFileUrl(const QUrl& fileUrl)
{
    if (m_fileUrl == fileUrl) {
        return;
    }


    m_fileUrl = fileUrl;
    emit fileUrlChanged(m_fileUrl);
}

void FileWatcher::setEnabled(bool enabled)
{
    if (m_enabled == enabled) {
        return;
    }

    m_enabled = enabled;
    emit enabledChanged(m_enabled);
}

void FileWatcher::setRecursive(bool recursive)
{
    if (m_recursive == recursive) {
        return;
    }

    m_recursive = recursive;
    emit recursiveChanged(m_recursive);
}

void FileWatcher::setNameFilters(const QStringList& nameFilters)
{
    if (m_nameFilters == nameFilters) {
        return;
    }

    m_nameFilters = nameFilters;
    emit nameFiltersChanged(m_nameFilters);

}

bool FileWatcher::updateWatchedFile()
{
    const auto& files = m_fileSystemWatcher.files();
    if (files.length() > 0) {
        m_fileSystemWatcher.removePaths(files);
    }
    const auto& directories = m_fileSystemWatcher.directories();
    if (directories.length() > 0) {
        m_fileSystemWatcher.removePaths(directories);
    }

    if (!m_fileUrl.isValid() || !m_enabled) {
        return false;
    }

    if (!m_fileUrl.isLocalFile()) {
        qCWarning(filewatcherCategory) << "Can only watch local files";
        return false;
    }
    const auto& localFile = m_fileUrl.toLocalFile();
    if (localFile.isEmpty()) {
        return false;
    }

    if (m_recursive && QDir(localFile).exists()) {
        QSet<QString> newPaths;
        m_fileSystemWatcher.addPath(localFile);
        newPaths.insert(localFile);

        QDirIterator it(localFile, QDirIterator::Subdirectories | QDirIterator::FollowSymlinks);
        while (it.hasNext()) {
            const auto& file = it.next();
            const QString extension = it.fileInfo().completeSuffix();
            bool filtered = false;
            for (QString& filter : m_nameFilters)
            {
                QRegularExpression re(filter);
                auto m = re.match(extension);
                if (m.hasMatch())
                {
                    filtered = true;
                    break;
                }

            }

            if ((it.fileName() == QLatin1String("..")) || (it.fileName() == QLatin1String("."))
                || filtered) {
                continue;
            }
            m_fileSystemWatcher.addPath(file);
            newPaths.insert(it.filePath());
        }

        return newPaths != QSet<QString>(files.begin(), files.end()).unite(QSet<QString>(directories.begin(), directories.end()));
    } else if (QFile::exists(localFile)) {
        m_fileSystemWatcher.addPath(localFile);
    } else {
#ifdef QT_DEBUG
        qCWarning(filewatcherCategory) << "File to watch does not exist" << localFile;
#endif
    }
    return false;
}

void FileWatcher::onWatchedFileChanged()
{
    if (m_enabled) {
        emit fileChanged();
    }
}

void FileWatcher::onWatchedDirectoryChanged(const QString&)
{
    if (updateWatchedFile()) {
        onWatchedFileChanged(); // propagate event
    }
}
