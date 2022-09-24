/*
 * Copyright (c) 2021. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     huqinghong <huqinghong@uniontech.com>
 *
 * Maintainer: huqinghong <huqinghong@uniontech.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <gtest/gtest.h>

#include <QtConcurrent/QtConcurrent>
#include "module/util/http/httpclient.h"
#include "src/module/util/file.h"
#include "src/module/util/config/config.h"

TEST(httpclient, test01)
{
    QStringList userInfo = {"linglong", "linglong"};

    QString configUrl = ConfigInstance().repos[linglong::kDefaultRepo]->endpoint;

    EXPECT_EQ (true, !configUrl.isEmpty());

    auto token = G_HTTPCLIENT->getToken(configUrl, userInfo);
    EXPECT_EQ (token.size() > 0, true);

    QtConcurrent::run([=]() {
        QString outMsg;
        G_HTTPCLIENT->queryRemoteApp("cal", "", "x86_64", outMsg);
        EXPECT_EQ (outMsg.size() > 0, true);
    });
}