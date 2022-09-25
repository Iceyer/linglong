/*
 * Copyright (c) 2021. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     Iceyer <me@iceyer.net>
 *
 * Maintainer: Iceyer <me@iceyer.net>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "file.h"

#include <sys/stat.h>

#include <QDir>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>

namespace linglong {
namespace util {

QString getLinglongRootPath()
{
    if (qEnvironmentVariableIsSet("LINGLONG_ROOT")) {
        return qEnvironmentVariable("LINGLONG_ROOT");
    }

    auto sysProductVersion = QSysInfo::productVersion().toDouble();
    if (!isDeepinSysProduct()) {
        return QString("/var/lib/linglong");
    }

    // v20系统
    if (20 <= sysProductVersion && 23 > sysProductVersion) {
        return QString("/data/linglong");
    }

    // v23系统
    if (23 <= sysProductVersion) {
        return QString("/persistent/linglong");
    }
    return QString();
}

QString jonsPath(const QStringList &component)
{
    return QDir::toNativeSeparators(component.join(QDir::separator()));
}

QString getUserFile(const QString &path)
{
    auto dirPath = QStandardPaths::standardLocations(QStandardPaths::HomeLocation).at(0);
    if (!path.isEmpty()) {
        dirPath += "/" + path;
    }
    return dirPath;
}
QString ensureUserDir(const QString &relativeDirPath)
{
    QStringList dirPathComponents = {
        QStandardPaths::standardLocations(QStandardPaths::HomeLocation).at(0),
        relativeDirPath,
    };
    auto dirPath = QDir::cleanPath(dirPathComponents.join(QDir::separator()));
    QDir(dirPath).mkpath(".");
    return dirPath;
}

QString ensureUserDir(const QStringList &relativeDirPathComponents)
{
    auto relativeFilepath = QDir::cleanPath(relativeDirPathComponents.join(QDir::separator().toLatin1()));
    return ensureUserDir(relativeFilepath);
}

bool ensureDir(const QString &path)
{
    QDir dir(path);
    dir.mkpath(".");
    return true;
}

QString createProxySocket(const QString &pattern)
{
    auto userRuntimeDir = QString("/run/user/%1/").arg(getuid());
    QString socketDir = userRuntimeDir + ".dbus-proxy/";
    bool ret = util::createDir(socketDir);
    if (!ret) {
        qCritical() << "createProxySocket pattern:" << pattern << " failed";
        return "";
    }
    QTemporaryFile tmpFile(socketDir + pattern);
    tmpFile.setAutoRemove(false);
    if (!tmpFile.open()) {
        qCritical() << "create " << socketDir + pattern << " failed";
        return "";
    }
    tmpFile.close();
    return tmpFile.fileName();
}

QStringList getUserInfo()
{
    auto filePath = util::getUserFile(".linglong/.user.json");
    QStringList userInfo;

    QTextStream qin(stdin, QIODevice::ReadOnly);

    QFile file(filePath);

    if (file.exists()) {
        file.open(QIODevice::ReadOnly);

        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        QJsonObject infoObj = doc.object();

        userInfo.append(infoObj["username"].toString());
        userInfo.append(infoObj["password"].toString());

        file.close();
    } else {
        QString name;
        QString password;

        qInfo() << "please enter ldap account: ";
        qin >> name;
        qInfo() << "please enter password: ";
        qin >> password;

        userInfo.append(name);
        userInfo.append(password);
    }

    return userInfo;
}

quint64 sizeOfDir(const QString &srcPath)
{
    QDir srcDir(srcPath);
    quint64 size = 0;
    QFileInfoList list = srcDir.entryInfoList();

    for (auto info : list) {
        if (info.fileName() == "." || info.fileName() == "..") {
            continue;
        }
        if (info.isSymLink()) {
            // FIXME: https://bugreports.qt.io/browse/QTBUG-50301
            struct stat symlinkStat;
            lstat(info.absoluteFilePath().toLocal8Bit(), &symlinkStat);
            size += symlinkStat.st_size;
        } else if (info.isDir()) {
            // 一个文件夹大小为 4K
            size += 4 * 1024;
            size += sizeOfDir(QStringList {srcPath, info.fileName()}.join("/"));
        } else {
            size += info.size();
        }
    }

    return size;
}

QString fileHash(const QString &path, QCryptographicHash::Algorithm method)
{
    QFile sourceFile(path);
    qint64 fileSize = sourceFile.size();
    const qint64 bufferSize = 2 * 1024 * 1024;

    if (sourceFile.open(QIODevice::ReadOnly)) {
        char buffer[bufferSize];
        int bytesRead;
        int readSize = qMin(fileSize, bufferSize);

        QCryptographicHash hash(method);

        while (readSize > 0 && (bytesRead = sourceFile.read(buffer, readSize)) > 0) {
            fileSize -= bytesRead;
            hash.addData(buffer, bytesRead);
            readSize = qMin(fileSize, bufferSize);
        }

        sourceFile.close();
        return QString(hash.result().toHex());
    }

    return QString();
}

/*!
 * ensure parent dir of path exist
 * @param path
 * @return
 */
bool ensureParentDir(const QString &path)
{
    auto parentPath = QFileInfo(path).path();
    qDebug() << "ensureDir" << parentPath;
    return ensureDir(parentPath);
}

bool isDeepinSysProduct()
{
    auto sysType = QSysInfo::productType();
    if ("uos" == sysType || "Deepin" == sysType) {
        return true;
    }
    return false;
}

bool removeDir(const QString &path)
{
    // if path is empty, the QDir will remove pwd, we do not except that.
    if (path.isEmpty()) {
        return false;
    }

    QDir dir(path);
    if (dir.exists()) {
        return dir.removeRecursively();
    }

    return true;
}

void removeDstDirLinkFiles(const QString &src, const QString &dst)
{
    if (!dirExists(dst)) {
        return;
    }

    QDir srcDir(src);

    QFileInfoList list = srcDir.entryInfoList();

    foreach (QFileInfo info, list) {
        if (info.fileName() == "." || info.fileName() == "..") {
            continue;
        }
        if (info.isDir()) {
            // 穿越文件夹，递归调用
            removeDstDirLinkFiles(info.filePath(), dst + "/" + info.fileName());
            continue;
        }
        // 删除链接文件
        if (fileExists(QDir(dst).absolutePath() + "/" + info.fileName())) {
            QFile(QDir(dst).absolutePath() + "/" + info.fileName()).remove();
        }
    }
}

} // namespace util
} // namespace linglong