/*
 * Copyright (c) 2020-2021. Uniontech Software Ltd. All rights reserved.
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
#include <QStandardPaths>

#include "module/util/xdg.h"

TEST(Moduel_Util, Tool)
{
    QString env = "${HOME}/Desktop:${HOME}/Desktop";
    QRegExp exp("(\\$\\{.*\\})");
    exp.setMinimal(true);
    exp.indexIn(env);

    EXPECT_EQ(exp.capturedTexts().size(), 2);
    EXPECT_EQ(exp.capturedTexts().value(0), "${HOME}");

    auto envs = linglong::util::parseEnvKeyValue(env, ":");

    EXPECT_EQ(envs.first, envs.second);
    EXPECT_EQ(envs.first.contains("${HOME}"), false);
}

TEST(Moduel_Util, convertSpecialCharacters)
{
    QStringList args1 = {"/home/qwe/Video/test test/test.mp4"};
    QStringList args2 = {"/home/qwe/Video/test/测试 音乐.mp4"};

    auto retArgs1 = linglong::util::convertSpecialCharacters(args1);
    auto retArgs2 = linglong::util::convertSpecialCharacters(args2);

    EXPECT_EQ(retArgs1.value(0), "/home/qwe/Video/test\\ test/test.mp4");
    EXPECT_EQ(retArgs2.value(0), "/home/qwe/Video/test/测试\\ 音乐.mp4");

    auto retExec1 = linglong::util::parseExec(args1.value(0));
    auto retExec2 = linglong::util::parseExec(retArgs1.value(0));

    EXPECT_EQ(retExec1.size(), 2);
    EXPECT_EQ(retExec2.size(), 1);
}

TEST(Module_Util, Xdg01)
{

    auto r1 = linglong::util::getXdgDir("Desktop");
    EXPECT_EQ(r1.first, true);
    EXPECT_EQ(r1.second, QDir::homePath() + "/Desktop");

    auto r2 = linglong::util::getXdgDir("documentS");
    EXPECT_EQ(r2.first, true);
    EXPECT_EQ(r2.second, QDir::homePath() + "/Documents");

    auto r3 = linglong::util::getXdgDir("downloads");
    EXPECT_EQ(r3.first, true);
    EXPECT_EQ(r3.second, QDir::homePath() + "/Downloads");

    auto r4 = linglong::util::getXdgDir("music");
    EXPECT_EQ(r4.first, true);
    EXPECT_EQ(r4.second, QDir::homePath() + "/Music");

    auto r5 = linglong::util::getXdgDir("pictures");
    EXPECT_EQ(r5.first, true);
    EXPECT_EQ(r5.second, QDir::homePath() + "/Pictures");

    auto r5_2 = linglong::util::getXdgDir("picture");
    EXPECT_EQ(r5_2.first, true);
    EXPECT_EQ(r5_2.second, QDir::homePath() + "/Pictures");

    auto r6 = linglong::util::getXdgDir("home");
    EXPECT_EQ(r6.first, true);
    EXPECT_EQ(r6.second, QDir::homePath());

    auto r7 = linglong::util::getXdgDir("");
    EXPECT_EQ(r7.first, false);
    EXPECT_EQ(r7.second, "");

    auto r8 = linglong::util::getXdgDir("temp");
    EXPECT_EQ(r8.first, true);
    EXPECT_EQ(r8.second, QDir::tempPath());

    auto r9 = linglong::util::getXdgDir("runtime");
    EXPECT_EQ(r9.first, true);
    EXPECT_EQ(r9.second, QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation));

    auto r10 = linglong::util::getXdgDir("cxx");
    EXPECT_EQ(r10.first, false);
    EXPECT_EQ(r10.second, "");

    auto r11 = linglong::util::getXdgDir("public_share");
    EXPECT_EQ(r11.first, true);
    EXPECT_EQ(r11.second, QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/.Public");
}

TEST(Module_Util, Xdg02)
{
    QString r1 = "XDG_DESKTOP_DIR";
    auto r1XdgPath = linglong::util::getPathInXdgUserConfig(r1);
    EXPECT_EQ(r1XdgPath, QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));

    QString r2 = "XDG_DOCUMENTS_DIR";
    auto r2XdgPath = linglong::util::getPathInXdgUserConfig(r2);
    EXPECT_EQ(r2XdgPath, QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));

    QString r3 = "XXXX";
    auto r3XdgPath = linglong::util::getPathInXdgUserConfig(r3);
    EXPECT_EQ(r3XdgPath, "");

    QString r4 = "XDG_PUBLICSHARE_DIR";
    auto r4XdgPath = linglong::util::getPathInXdgUserConfig(r4);
    EXPECT_EQ(r4XdgPath, QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/.Public");
}