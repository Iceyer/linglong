/*
 * Copyright (c) 2020-2021. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     Iceyer <me@iceyer.net>
 *
 * Maintainer: Iceyer <me@iceyer.net>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef LINGLONG_SRC_MODULE_PACKAGE_BUNDLE_H_
#define LINGLONG_SRC_MODULE_PACKAGE_BUNDLE_H_

#include <elf.h>
#include <sys/stat.h>
#include <unistd.h>

#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <cstring>

#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QDir>

#include "module/util/result.h"
#include "module/util/file.h"
#include "module/util/status_code.h"
#include "module/util/http/httpclient.h"
#include "info.h"

namespace linglong {
namespace package {

class BundlePrivate;

/*!
 * Create Bundle format file, An Bundle contains loader, and it's squashfs.
 */
class Bundle : public QObject
{
    Q_OBJECT

public:
    explicit Bundle(QObject *parent = nullptr);
    ~Bundle();

    /**
     * Load Bundle from path, create parent if not exist
     * @param path
     * @return
     */
    util::Error load(const QString &path);

    /**
     * Save Bundle to path, create parent if not exist
     * @param path
     * @return
     */
    util::Error save(const QString &path);

    /**
     * make Bundle
     * @param dataPath : data path
     * @param outputFilePath : output file path
     * @return Result
     */
    util::Error make(const QString &dataPath, const QString &outputFilePath);

    /**
     * push Bundle
     * @param uabFilePath uab file path
     * @param repoUrl remote repo url
     * @param force  force to push
     * @return Result
     */
    util::Error push(const QString &bundleFilePath, const QString &repoUrl, const QString &repoChannel, bool force);

private:
    QScopedPointer<BundlePrivate> dd_ptr;
    Q_DECLARE_PRIVATE_D(qGetPtrHelper(dd_ptr), Bundle)
};

} // namespace package
} // namespace linglong

#endif /* LINGLONG_SRC_MODULE_PACKAGE_BUNDLE_H_ */
