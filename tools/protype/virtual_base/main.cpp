/*
 * Copyright (c) 2022. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     Iceyer <me@iceyer.net>
 *
 * Maintainer: Iceyer <me@iceyer.net>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QFile>
#include <QDebug>

int generateBaseTo(const QString &hostPrefix, const QString &hostPath, const QString &targetDirPath)
{
    Q_ASSERT(QDir::isAbsolutePath(hostPrefix));
    Q_ASSERT(QDir::isAbsolutePath(hostPath));
    Q_ASSERT(QDir::isAbsolutePath(targetDirPath));

    QDir rootDir(hostPath);
    QDir targetDir(targetDirPath + hostPath);

    // FIXME: skip soft link
    QDirIterator iter(rootDir.canonicalPath(), QDir::Files | QDir::NoDotAndDotDot, (QDirIterator::Subdirectories));
    while (iter.hasNext()) {
        iter.next();
        auto relativePath = rootDir.relativeFilePath(iter.filePath());

        auto virtualSourceDir = QDir(hostPrefix + hostPath);
        auto virtualSource = virtualSourceDir.filePath(relativePath);
        auto sourcePath = iter.filePath();
        sourcePath = virtualSource;

        auto targetPath = targetDir.filePath(relativePath);
        QFileInfo fi(targetPath);
        qDebug() << "mkpath" << fi.path() << "for" << targetPath;
        QDir(fi.path()).mkpath(".");
        qDebug() << "link" << targetPath << "-->" << sourcePath;
        QFile::link(sourcePath, targetPath);
    }
}

int main(int argc, char *argv[])
{
    generateBaseTo("/virtual-base", "/usr", "/persistent/linglong/virtual-base");
}