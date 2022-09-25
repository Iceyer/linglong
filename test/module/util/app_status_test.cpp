/*
 * Copyright (c) 2022. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     huqinghong <huqinghong@uniontech.com>
 *
 * Maintainer: huqinghong <huqinghong@uniontech.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <gtest/gtest.h>

#include <QDebug>

#include "package_manager/impl/app_status.h"
#include "src/module/util/sysinfo.h"

using namespace linglong;

TEST(Util, Database)
{
    util::Connection connection;
    QSqlQuery sqlQuery = connection.execute("select * from installedAppInfo");
    EXPECT_EQ(sqlQuery.lastError().type(), QSqlError::NoError);
}

TEST(Util, GetInstalledAppInfo)
{
    package::registerAllMetaType();

    // 插入2条记录
    QString appDataJson = R"MLS({
	"appId": "org.deepin.calculator-GetInstalledAppInfo",
	"name": "deepin-calculator",
	"version": "5.7.16",
	"arch": "x86_64",
	"kind": "app",
	"runtime": "org.deepin.Runtime/20.5.0/x86_64",
	"uabUrl": "",
	"repoName": "repo",
	"description": "Calculator for UOS"
}
)MLS";

    util::checkInstalledAppDb();
    util::updateInstalledAppInfoDb();

    QScopedPointer<package::MetaInfo> appItem(util::loadJSONString<package::MetaInfo>(appDataJson));

    const QString userName = util::getUserName();

    package::MetaInfoList pkgList;
    auto result = util::getInstalledAppInfo(appItem->appId, "", appItem->arch, "", "", "", pkgList);
    EXPECT_EQ(result, false);

    package::MetaInfoList pkgList1;
    result = util::getInstalledAppInfo(appItem->appId, "", appItem->arch, "", "", "", pkgList1);
    EXPECT_EQ(result, false);

    QString ret = "";
    QString error = "";
    result = util::queryAllInstalledApp("deepin-linglong", ret, error);
    EXPECT_EQ(result, true);

    package::MetaInfoList appMetaInfoList;
    util::getMetaInfoListByJson(ret, appMetaInfoList);

    auto code = util::insertAppRecord(appItem.data(), "user", userName);
    EXPECT_EQ(code, 0);

    // FIXME: always return zero
    code = util::deleteAppRecord(appItem->appId, "1.0.0", util::hostArch(), "", "", userName);
    EXPECT_EQ(code, 0);

    code = util::deleteAppRecord(appItem->appId, appItem->version, util::hostArch(), "", "", userName);
    EXPECT_EQ(code, 0);
}