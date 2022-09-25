/*
 * Copyright (c) 2022. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     Iceyer <me@iceyer.net>
 *
 * Maintainer: Iceyer <me@iceyer.net>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <gtest/gtest.h>

#include <QDir>
#include <QDebug>

#include "src/module/util/file.h"

TEST(Util, CreateProxySocket)
{
    QDir empty("");
    QString value;
    linglong::util::createProxySocket("session-bus-proxy-XXXXXX");

    auto hostAppHome = linglong::util::ensureUserDir({".linglong", "org.deepin.calculator", "home"});
    linglong::util::getUserFile(".config");

    auto userRuntimeDir = QString("/run/user/%1").arg(getuid());
    linglong::util::jonsPath({userRuntimeDir, "bus"});
    EXPECT_EQ(empty.exists(), true);
}

TEST(Util, Filesystem)
{
    // listDirFolders
    bool delete_dir = false;
    auto parent_path = "/tmp/deepin-linglong/layers";
    if (!linglong::util::dirExists(parent_path)) {
        delete_dir = true;
        linglong::util::createDir(QString(parent_path) + "/1.0");
        linglong::util::createDir(QString(parent_path) + "/2.0");
    }

    auto r1 = linglong::util::listDirFolders("/tmp/deepin-linglong");
    EXPECT_NE(r1.empty(), true);
    EXPECT_EQ(r1.first(), parent_path);

    auto r2 = linglong::util::listDirFolders("/tmp/deepin-linglong", true);
    EXPECT_NE(r2.empty(), true);
    EXPECT_GT(r2.size(), 1);

    auto r3 = linglong::util::listDirFolders("/tmp/deepin-linglong", false);
    EXPECT_NE(r3.empty(), true);
    EXPECT_EQ(r3.first(), parent_path);
    EXPECT_EQ(r3.size(), 1);

    if (delete_dir && linglong::util::dirExists(parent_path)) {
        delete_dir = false;
        linglong::util::removeDir(QString("/tmp/deepin-linglong"));
    }
}
