/*
 * Copyright (c) 2021. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     Iceyer <me@iceyer.net>
 *
 * Maintainer: Iceyer <me@iceyer.net>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <QStringList>
#include <QString>
#include <QFileInfo>
#include <QProcess>
#include <QDir>
#include <QDirIterator>
#include <QDebug>
#include <QTemporaryFile>
#include <QSysInfo>
#include <QCryptographicHash>

#include "status_code.h"

namespace linglong {
namespace util {

QStringList getUserInfo();

QString jonsPath(const QStringList &component);

QString getUserFile(const QString &path);

QString ensureUserDir(const QStringList &relativeDirPathComponents);

bool ensureDir(const QString &path);

bool ensureParentDir(const QString &path);

/*!
 * 计算文件hash
 *
 * @param: path: 文件路径
 *
 * @param: method: hash算法
 *
 * @return QString: hash字符串
 */
QString fileHash(const QString &path, QCryptographicHash::Algorithm method);

/*!
 * 计算文件夹大小
 *
 * @param: srcPath: 文件夹路径
 *
 * @return quint64: byte 文件夹大小
 */
quint64 sizeOfDir(const QString &srcPath);

/*!
 * 创建一个pattern格式的随机文件
 *
 * @param pattern 文件格式
 *
 * @return 文件路径
 */
QString createProxySocket(const QString &pattern);

/*!
 * 判断文件是否存在
 * @param path
 * @return
 */
bool inline fileExists(const QString &path)
{
    QFileInfo fs(path);
    return fs.exists() && fs.isFile() ? true : false;
}

/*!
 * 判断目录是否存在
 * @param path
 * @return
 */
bool inline dirExists(const QString &path)
{
    QFileInfo fs(path);
    return fs.exists() && fs.isDir() ? true : false;
}

/*!
 * 创建目录
 * @param path 路径
 * @return bool
 */
bool inline createDir(const QString &path)
{
    auto val = QDir().exists(path);
    if (!val) {
        return QDir().mkpath(path);
    }
    return true;
}

/*!
 * 移除目录
 * @param path 绝对路径
 * @return bool
 */
bool removeDir(const QString &path);

/*!
 * 获取指定目录下的子目录
 * @param path 输入路径
 * @param subdir 探测子目录，默认false
 * @return QStringList
 */
QStringList inline listDirFolders(const QString &path, const bool subdir = false)
{
    QStringList parent;

    QDirIterator dirs(path, QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot,
                      subdir ? (QDirIterator::IteratorFlag)0x2 : (QDirIterator::IteratorFlag)0x0);

    while (dirs.hasNext()) {
        dirs.next();
        parent << dirs.filePath();
    }
    return parent;
}

/*!
 * 建立link
 * @param src 来源
 * @param dest 目标
 * @param override 默认覆盖
 * @return
 */
bool inline linkFile(const QString &src, const QString &dest, const bool override = true)
{
    // QFile::link(const QString &fileName, const QString &linkName)
    QFile::link(src, dest);
    return true;
}

/*!
 * 拷贝目录
 * @param src 来源
 * @param dst 目标
 * @return
 */
void inline copyDir(const QString &src, const QString &dst)
{
    QDir srcDir(src);
    QDir dstDir(dst);

    if (!dstDir.exists()) {
        dstDir.mkpath(".");
    }

    QFileInfoList list = srcDir.entryInfoList();

    foreach (QFileInfo info, list) {
        if (info.fileName() == "." || info.fileName() == "..") {
            continue;
        }
        if (info.isDir()) {
            // 穿件文件夹，递归调用
            copyDir(info.filePath(), dst + "/" + info.fileName());
            continue;
        }
        // 拷贝文件
        QFile file(info.filePath());
        file.copy(dst + "/" + info.fileName());
    }
}

/*!
 * 建立源目录下所有文件的链接到目标目录下并保持目录结构不变
 * @param src 来源
 * @param dst 目标目录
 * @return
 */
void inline linkDirFiles(const QString &src, const QString &dst)
{
    QDir srcDir(src);

    createDir(dst);

    QFileInfoList list = srcDir.entryInfoList();

    foreach (QFileInfo info, list) {
        if (info.fileName() == "." || info.fileName() == "..") {
            continue;
        }
        if (info.isDir()) {
            createDir(dst + "/" + info.fileName());
            // 穿越文件夹，递归调用
            linkDirFiles(info.filePath(), dst + "/" + info.fileName());
            continue;
        }
        // 链接文件
        linkFile(info.absoluteFilePath(), QDir(dst).absolutePath() + "/" + info.fileName());
    }
}

/*!
 * 从src目录获取相应文件名称，删除目标目录中的对应链接文件
 * @param src 来源
 * @param dst 目标目录
 * @return
 */
void removeDstDirLinkFiles(const QString &src, const QString &dst);

/*!
 * 判断是否是deepin系统或者其发型版
 * @return bool: true:是deepin系统产品
 */
bool isDeepinSysProduct();

/*!
 * 根据系统版本获取玲珑安装路径
 * @return QString 玲珑安装路径
 */
QString getLinglongRootPath();

} // namespace util
} // namespace linglong
