/*
 * Copyright (c) 2021. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     huqinghong@uniontech.com
 *
 * Maintainer: huqinghong@uniontech.com
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <gtest/gtest.h>
#include <thread>
#include <future>

#include <QDebug>

#include "module/dbus_ipc/dbus_package_manager_common.h"
#include "module/dbus_ipc/dbus_app_manager_common.h"
#include "module/flatpak/flatpak_manager.h"
#include "module/package/package.h"
#include "module/util/runner.h"
#include "module/util/log/log_handler.h"
#include "module/repo/repo.h"
#include "package_manager/impl/app_status.h"
#include "package_manager/impl/package_manager.h"
#include "service/impl/register_meta_type.h"
#include "service/impl/param_option.h"
#include "service/impl/reply.h"

#include "dbus_gen_app_manager_interface.h"
#include "dbus_gen_package_manager_interface.h"
#include "dbus_gen_package_manager_adaptor.h"
#include "dbus_gen_job_manager_adaptor.h"

#include "cli/cli.h"

using namespace linglong;

// void start_ll_service()
//{
//     setenv("DISPLAY", ":0", 1);
//     setenv("XAUTHORITY", "~/.Xauthority", 1);
//     // 需要进入到 test 目录进行 ll-test
//     system("../bin/ll-service &");
// }

// TODO: it seem no need static
int testAppArgc = 0;
char *testAppArgv = nullptr;
static QCoreApplication *testQApp = nullptr;
static auto testRepoUrl = QString("https://repo-dev.linglong.space");
static auto packageManagerAddress = QString("unix:path=/tmp/linglong_package_manager_socket");
static QDBusServer *dbusServer = nullptr;
static QDBusConnection *packageManagerDBusConnection = nullptr;

static void initDBusAdaptor()
{
    if (testQApp) {
        return;
    }

    linglong::service::registerAllMetaType();

    auto linglongRoot = qEnvironmentVariable("LINGLONG_ROOT");
    auto repo = linglongRoot + "/repo";
    if (!util::dirExists(repo)) {
        util::ensureDir(linglongRoot);
        runner::Runner("ostree", QStringList {"init", "--repo=" + repo, "--mode=bare-user-only"});
    }

    testQApp = new QCoreApplication(testAppArgc, &testAppArgv);

    new PackageManagerAdaptor(package_manager::PackageManager::instance());
    new JobManagerAdaptor(package_manager::JobManager::instance());
    dbusServer = new QDBusServer(packageManagerAddress);

    EXPECT_EQ(dbusServer->isConnected(), true);

    QObject::connect(dbusServer, &QDBusServer::newConnection, [&](const QDBusConnection &conn) {
        // FIXME: work round to keep conn alive, but we finally need to free clientConn.
        qDebug() << "newConnection" << packageManagerAddress << conn.isConnected();
        auto clientConn = new QDBusConnection(conn);
        registerServiceAndObject(clientConn, "",
                                 {{DBusPackageManagerPath, package_manager::PackageManager::instance()},
                                  {DBusPackageManagerJobManagerPath, package_manager::JobManager::instance()}});
    });

    packageManagerDBusConnection =
        new QDBusConnection(QDBusConnection::connectToPeer(packageManagerAddress, "ll-package-manager-2"));
}

void runManagerDBusTest(std::function<void(OrgDeepinLinglongPackageManagerInterface &pmInterface)> f)
{
    initDBusAdaptor();

    QScopedPointer<OrgDeepinLinglongPackageManagerInterface> pm(new OrgDeepinLinglongPackageManagerInterface(
        DBusPackageManagerServiceName, DBusPackageManagerPath, *packageManagerDBusConnection));

    QtConcurrent::run([&f, &pm]() {
        QThread::msleep(200);

        // FIXME: update repo config
        auto dbusReply = pm->ModifyRepo(testRepoUrl);
        dbusReply.waitForFinished();

        f(*pm);
        testQApp->exit(0);
    });

    testQApp->exec();
}

static int runCliJobTest(std::function<QDBusPendingReply<QString>()> funcCreateJob)
{
    QScopedPointer<cli::Cli> cli(new cli::Cli);
    return cli->runJob([&]() -> QDBusPendingReply<QString> { return funcCreateJob(); },
                       [&](const QString &path) -> QDBusInterface * {
                           return new QDBusInterface(DBusPackageManagerServiceName, path,
                                                     DBusPackageManagerJobInterface, *packageManagerDBusConnection,
                                                     nullptr);
                       },
                       false);
}

TEST(PackageManager, install01)
{
    runManagerDBusTest([](OrgDeepinLinglongPackageManagerInterface &pm) {
        auto ret = runCliJobTest([&]() {
            auto refStr = "org.deepin.music";
            return pm.Install(refStr, {});
        });
        EXPECT_EQ(ret, 0);
    });
}

TEST(PackageManager, install02)
{
    runManagerDBusTest([](OrgDeepinLinglongPackageManagerInterface &pm) {
        auto refStr = "org.deepin.calculator/5.7.16";

        QDBusPendingReply<QString> reply = pm.Uninstall(refStr, {});
        reply.waitForFinished();
        EXPECT_EQ(reply.error().isValid(), true);

        auto ret = runCliJobTest([&] { return pm.Install(refStr, {}); });
        EXPECT_EQ(ret, 0);

        reply = pm.Uninstall(refStr, {});
        reply.waitForFinished();
        EXPECT_EQ(reply.error().isValid(), false);
    });
}

// TEST(Package, update01)
//{
//     prepareDBusEnv();
//
//     OrgDeepinLinglongPackageManagerInterface pm("org.deepin.linglong.PackageManager",
//                                                 "/org/deepin/linglong/PackageManager", QDBusConnection::systemBus());
//
//     linglong::service::UninstallParamOption uninstallParam;
//     uninstallParam.appId = "org.deepin.calculator";
//     uninstallParam.version = "5.7.16.1";
//     QDBusPendingReply<linglong::service::Reply> dbusReply = pm.Uninstall(uninstallParam);
//     dbusReply.waitForFinished();
//
//     linglong::service::ParamOption parmOption;
//     parmOption.appId = "org.deepin.calculator";
//     dbusReply = pm.Update(parmOption);
//     dbusReply.waitForFinished();
//     linglong::service::Reply retReply = dbusReply.value();
//
//     bool expectRet = false;
//     bool connect = getConnectStatus();
//     if (!connect) {
//         expectRet = false;
//     }
//     if (expectRet) {
//         EXPECT_EQ(retReply.code, STATUS_CODE(kErrorPkgUpdateSuccess));
//     } else {
//         EXPECT_NE(retReply.code, STATUS_CODE(kErrorPkgUpdateSuccess));
//     }
// }

TEST(PackageManager, install03)
{
    // 重复安装
    runManagerDBusTest([](OrgDeepinLinglongPackageManagerInterface &pm) {
        auto ret = runCliJobTest([&]() {
            auto refStr = "org.deepin.calculator";
            return pm.Install(refStr, {});
        });
        EXPECT_EQ(ret, 0);
    });
}

//
// TEST(Package, install04)
//{
//     runPackageManagerDBusTest([](OrgDeepinLinglongPackageManagerInterface &pm) {
//         auto refStr = "";
//         QDBusPendingReply<linglong::service::Reply> dbusReply = pm.Install(refStr, {});
//         dbusReply.waitForFinished();
//         linglong::service::Reply retReply = dbusReply.value();
//
//         EXPECT_NE(retReply.code, STATUS_CODE(kPkgInstallSuccess));
//     });
// }
//
// TEST(Package, install05)
//{
//     runPackageManagerDBusTest([](OrgDeepinLinglongPackageManagerInterface &pm) {
//         auto refStr = "org.deepin.calculator//arm64";
//         QDBusPendingReply<linglong::service::Reply> dbusReply = pm.Install(refStr, {});
//         dbusReply.waitForFinished();
//         linglong::service::Reply retReply = dbusReply.value();
//
//         EXPECT_NE(retReply.code, STATUS_CODE(kPkgInstallSuccess));
//     });
// }
//
// TEST(Package, install06)
//{
//     runPackageManagerDBusTest([](OrgDeepinLinglongPackageManagerInterface &pm) {
//         auto refStr = "com.belmoussaoui.Decoder";
//         QVariantMap options;
//         options["repoPoint"] = "flatpak";
//         QDBusPendingReply<linglong::service::Reply> dbusReply = pm.Install(refStr, options);
//         dbusReply.waitForFinished();
//         linglong::service::Reply retReply = dbusReply.value();
//
//         EXPECT_NE(retReply.code, STATUS_CODE(kPkgInstalling));
//     });
// }
//
// TEST(Package, run01)
//{
//     prepareDBusEnv();
//
//     OrgDeepinLinglongAppManagerInterface pm(AppManagerDBusServiceName, AppManagerDBusPath,
//                                             QDBusConnection::sessionBus());
//
//     linglong::service::RunParamOption paramOption;
//     paramOption.appId = "com.belmoussaoui.Decoder";
//     paramOption.repoPoint = "flatpak";
//
//     QString runtimeRootPath = linglong::flatpak::FlatpakManager::instance()->getRuntimePath(paramOption.appId);
//     QString appPath = linglong::flatpak::FlatpakManager::instance()->getAppPath(paramOption.appId);
//     QStringList desktopPath =
//     linglong::flatpak::FlatpakManager::instance()->getAppDesktopFileList(paramOption.appId);
//
//     QDBusPendingReply<linglong::service::Reply> dbusReply = pm.Start(paramOption);
//     dbusReply.waitForFinished();
//     linglong::service::Reply retReply = dbusReply.value();
//
//     EXPECT_NE(retReply.code, STATUS_CODE(kFail));
//
//     // stop service
//     stop_ll_service();
// }
//
// TEST(Package, run02)
//{
//     // start service
//     std::thread startQdbus(start_ll_service);
//     startQdbus.detach();
//     std::this_thread::sleep_for(std::chrono::seconds(1));
//
//     OrgDeepinLinglongAppManagerInterface pm(AppManagerDBusServiceName, AppManagerDBusPath,
//                                             QDBusConnection::sessionBus());
//
//     linglong::service::RunParamOption paramOption;
//     paramOption.appId = "org.deepin.calculator";
//     QDBusPendingReply<linglong::service::Reply> dbusReply = pm.Start(paramOption);
//     dbusReply.waitForFinished();
//     linglong::service::Reply retReply = dbusReply.value();
//
//     EXPECT_NE(retReply.code, STATUS_CODE(kFail));
//
//     // stop service
//     stop_ll_service();
// }
//
// TEST(Package, run03)
//{
//     // start service
//     std::thread startQdbus(start_ll_service);
//     startQdbus.detach();
//     std::this_thread::sleep_for(std::chrono::seconds(1));
//
//     OrgDeepinLinglongAppManagerInterface pm(AppManagerDBusServiceName, AppManagerDBusPath,
//                                             QDBusConnection::sessionBus());
//     linglong::service::ExecParamOption paramOption;
//     paramOption.containerID = "org.deepin.test";
//     paramOption.cmd = "/bin/bash";
//     QDBusPendingReply<linglong::service::Reply> dbusReply = pm.Exec(paramOption);
//     dbusReply.waitForFinished();
//     linglong::service::Reply retReply = dbusReply.value();
//
//     EXPECT_NE(retReply.code, STATUS_CODE(kSuccess));
//
//     // stop service
//     stop_ll_service();
// }
//
// TEST(Package, query01)
//{
//     // start service
//     std::thread startQdbus(start_ll_service);
//     startQdbus.detach();
//     std::this_thread::sleep_for(std::chrono::seconds(1));
//
//     OrgDeepinLinglongPackageManagerInterface pm("org.deepin.linglong.PackageManager",
//                                                 "/org/deepin/linglong/PackageManager", QDBusConnection::systemBus());
//     // test app not in repo
//     linglong::service::QueryParamOption paramOption;
//     paramOption.appId = QString();
//
//     linglong::package::MetaInfoList retMsg;
//     QDBusPendingReply<linglong::service::QueryReply> dbusReply = pm.Query(paramOption);
//     dbusReply.waitForFinished();
//     linglong::service::QueryReply reply = dbusReply.value();
//     linglong::util::getMetaInfoListByJson(reply.result, retMsg);
//     bool ret = retMsg.size() == 0 ? true : false;
//     EXPECT_EQ(ret, true);
//     // stop service
//     stop_ll_service();
// }
//
// TEST(Package, query02)
//{
//     // start service
//     std::thread startQdbus(start_ll_service);
//     startQdbus.detach();
//     std::this_thread::sleep_for(std::chrono::seconds(1));
//
//     OrgDeepinLinglongPackageManagerInterface pm("org.deepin.linglong.PackageManager",
//                                                 "/org/deepin/linglong/PackageManager", QDBusConnection::systemBus());
//     // test app not in repo
//     linglong::service::QueryParamOption paramOption;
//     paramOption.appId = "org.test1";
//
//     linglong::package::MetaInfoList retMsg;
//     QDBusPendingReply<linglong::service::QueryReply> dbusReply = pm.Query(paramOption);
//     dbusReply.waitForFinished();
//     linglong::service::QueryReply reply = dbusReply.value();
//     linglong::util::getMetaInfoListByJson(reply.result, retMsg);
//     bool ret = retMsg.size() == 0 ? true : false;
//     EXPECT_EQ(ret, true);
//     // stop service
//     stop_ll_service();
// }
//
// TEST(Package, query03)
//{
//     // start service
//     std::thread startQdbus(start_ll_service);
//     startQdbus.detach();
//     std::this_thread::sleep_for(std::chrono::seconds(1));
//
//     OrgDeepinLinglongPackageManagerInterface pm("org.deepin.linglong.PackageManager",
//                                                 "/org/deepin/linglong/PackageManager", QDBusConnection::systemBus());
//     linglong::service::QueryParamOption paramOption;
//     paramOption.appId = "cal";
//
//     linglong::package::MetaInfoList retMsg;
//     QDBusPendingReply<linglong::service::QueryReply> dbusReply = pm.Query(paramOption);
//     dbusReply.waitForFinished();
//     linglong::service::QueryReply reply = dbusReply.value();
//     linglong::util::getMetaInfoListByJson(reply.result, retMsg);
//     bool ret = retMsg.size() == 0 ? false : true;
//     EXPECT_EQ(ret, true);
//     // stop service
//     stop_ll_service();
// }
//
// TEST(Package, query04)
//{
//     // start service
//     std::thread startQdbus(start_ll_service);
//     startQdbus.detach();
//     std::this_thread::sleep_for(std::chrono::seconds(1));
//
//     OrgDeepinLinglongPackageManagerInterface pm("org.deepin.linglong.PackageManager",
//                                                 "/org/deepin/linglong/PackageManager", QDBusConnection::systemBus());
//     linglong::service::QueryParamOption paramOption;
//     paramOption.appId = "com.360.browser-stable";
//
//     linglong::package::MetaInfoList retMsg;
//     QDBusPendingReply<linglong::service::QueryReply> dbusReply = pm.Query(paramOption);
//     dbusReply.waitForFinished();
//     linglong::service::QueryReply reply = dbusReply.value();
//     linglong::util::getMetaInfoListByJson(reply.result, retMsg);
//     bool ret = retMsg.size() == 0 ? false : true;
//     EXPECT_EQ(ret, true);
//     // stop service
//     stop_ll_service();
// }
//
// TEST(Package, query05)
//{
//     // start service
//     std::thread startQdbus(start_ll_service);
//     startQdbus.detach();
//     std::this_thread::sleep_for(std::chrono::seconds(1));
//
//     OrgDeepinLinglongPackageManagerInterface pm("org.deepin.linglong.PackageManager",
//                                                 "/org/deepin/linglong/PackageManager", QDBusConnection::systemBus());
//     // test flatpak app
//     linglong::service::QueryParamOption paramOption;
//     paramOption.appId = "com.belmoussaoui.Decoder";
//     paramOption.repoPoint = "flatpak";
//
//     linglong::package::MetaInfoList retMsg;
//     QDBusPendingReply<linglong::service::QueryReply> dbusReply = pm.Query(paramOption);
//     dbusReply.waitForFinished();
//     linglong::service::QueryReply reply = dbusReply.value();
//     linglong::util::getMetaInfoListByJson(reply.result, retMsg);
//     bool ret = retMsg.size() == 0 ? false : true;
//     EXPECT_EQ(ret, true);
//     // stop service
//     stop_ll_service();
// }
//
// TEST(Package, list01)
//{
//     // start service
//     std::thread startQdbus(start_ll_service);
//     startQdbus.detach();
//     std::this_thread::sleep_for(std::chrono::seconds(1));
//
//     OrgDeepinLinglongPackageManagerInterface pm("org.deepin.linglong.PackageManager",
//                                                 "/org/deepin/linglong/PackageManager", QDBusConnection::systemBus());
//     linglong::service::QueryParamOption paramOption;
//     paramOption.appId = "";
//
//     linglong::package::MetaInfoList retMsg;
//     QDBusPendingReply<linglong::service::QueryReply> dbusReply = pm.Query(paramOption);
//     dbusReply.waitForFinished();
//     linglong::service::QueryReply reply = dbusReply.value();
//     linglong::util::getMetaInfoListByJson(reply.result, retMsg);
//     bool ret = retMsg.size() == 0 ? true : false;
//     EXPECT_EQ(ret, true);
//     // stop service
//     stop_ll_service();
// }
//
// TEST(Package, list02)
//{
//     // start service
//     std::thread startQdbus(start_ll_service);
//     startQdbus.detach();
//     std::this_thread::sleep_for(std::chrono::seconds(1));
//
//     OrgDeepinLinglongPackageManagerInterface pm("org.deepin.linglong.PackageManager",
//                                                 "/org/deepin/linglong/PackageManager", QDBusConnection::systemBus());
//     linglong::service::QueryParamOption paramOption;
//     paramOption.appId = "installed";
//     linglong::package::MetaInfoList retMsg;
//     QDBusPendingReply<linglong::service::QueryReply> dbusReply = pm.Query(paramOption);
//     dbusReply.waitForFinished();
//     linglong::service::QueryReply reply = dbusReply.value();
//     linglong::util::getMetaInfoListByJson(reply.result, retMsg);
//     bool ret = retMsg.size() > 0 ? true : false;
//     EXPECT_EQ(ret, true);
//     // stop service
//     stop_ll_service();
// }
//
// TEST(Package, list03)
//{
//     // start service
//     std::thread startQdbus(start_ll_service);
//     startQdbus.detach();
//     std::this_thread::sleep_for(std::chrono::seconds(1));
//
//     OrgDeepinLinglongPackageManagerInterface pm("org.deepin.linglong.PackageManager",
//                                                 "/org/deepin/linglong/PackageManager", QDBusConnection::systemBus());
//     linglong::service::QueryParamOption paramOption;
//     paramOption.appId = "installed";
//     paramOption.repoPoint = "flatpak";
//     linglong::package::MetaInfoList retMsg;
//     QDBusPendingReply<linglong::service::QueryReply> dbusReply = pm.Query(paramOption);
//     dbusReply.waitForFinished();
//     linglong::service::QueryReply reply = dbusReply.value();
//     linglong::util::getMetaInfoListByJson(reply.result, retMsg);
//     bool ret = retMsg.size() > 0 ? true : false;
//     EXPECT_EQ(ret, true);
//     // stop service
//     stop_ll_service();
// }
//
// TEST(Package, ps01)
//{
//     // start service
//     std::thread startQdbus(start_ll_service);
//     startQdbus.detach();
//     std::this_thread::sleep_for(std::chrono::seconds(1));
//
//     OrgDeepinLinglongAppManagerInterface pm(AppManagerDBusServiceName, AppManagerDBusPath,
//                                             QDBusConnection::sessionBus());
//
//     linglong::service::RunParamOption paramOption;
//     paramOption.appId = "org.deepin.calculator";
//     linglong::package::MetaInfoList retMsg;
//     QDBusPendingReply<linglong::service::Reply> reply = pm.Start(paramOption);
//     reply.waitForFinished();
//
//     QDBusPendingReply<linglong::service::QueryReply> dbusReply = pm.List();
//     dbusReply.waitForFinished();
//
//     EXPECT_NE(dbusReply.value().code, STATUS_CODE(kFail));
//
//     // stop service
//     stop_ll_service();
// }
//
// TEST(Package, kill)
//{
//     // start service
//     std::thread startQdbus(start_ll_service);
//     startQdbus.detach();
//     std::this_thread::sleep_for(std::chrono::seconds(1));
//
//     OrgDeepinLinglongAppManagerInterface pm(AppManagerDBusServiceName, AppManagerDBusPath,
//                                             QDBusConnection::sessionBus());
//
//     QDBusPendingReply<linglong::service::Reply> dbusReply = pm.Stop("org.deepin.calculator");
//     dbusReply.waitForFinished();
//
//     EXPECT_NE(dbusReply.value().code, STATUS_CODE(kSuccess));
//     // stop service
//     stop_ll_service();
// }

// TEST(Package, uninstall01)
//{
//     // start service
//     std::thread startQdbus(start_ll_service);
//     startQdbus.detach();
//     std::this_thread::sleep_for(std::chrono::seconds(1));
//
//     OrgDeepinLinglongPackageManagerInterface pm("org.deepin.linglong.PackageManager",
//                                                 "/org/deepin/linglong/PackageManager", QDBusConnection::systemBus());
//     linglong::service::UninstallParamOption uninstallParam;
//     uninstallParam.appId = "org.deepin.calculator";
//     uninstallParam.version = "5.7.16.1";
//     QDBusPendingReply<linglong::service::Reply> dbusReply = pm.Uninstall(uninstallParam);
//     dbusReply.waitForFinished();
//
//     linglong::service::Reply retReply = dbusReply.value();
//     EXPECT_NE(retReply.code, STATUS_CODE(kPkgUninstallSuccess));
//
//     // stop service
//     stop_ll_service();
// }

// TEST(Package, uninstall02)
//{
//     // start service
//     std::thread startQdbus(start_ll_service);
//     startQdbus.detach();
//     std::this_thread::sleep_for(std::chrono::seconds(1));
//
//     OrgDeepinLinglongPackageManagerInterface pm("org.deepin.linglong.PackageManager",
//                                                 "/org/deepin/linglong/PackageManager", QDBusConnection::systemBus());
//     linglong::service::UninstallParamOption uninstallParam;
//     uninstallParam.appId = "org.deepin.calculator123";
//     uninstallParam.version = "5.7.16.1";
//     QDBusPendingReply<linglong::service::Reply> dbusReply = pm.Uninstall(uninstallParam);
//     dbusReply.waitForFinished();
//
//     linglong::service::Reply retReply = dbusReply.value();
//     EXPECT_NE(retReply.code, STATUS_CODE(kPkgUninstallSuccess));
//
//     // stop service
//     stop_ll_service();
// }

// TEST(Package, uninstall03)
//{
//     // start service
//     std::thread startQdbus(start_ll_service);
//     startQdbus.detach();
//     std::this_thread::sleep_for(std::chrono::seconds(1));
//
//     OrgDeepinLinglongPackageManagerInterface pm("org.deepin.linglong.PackageManager",
//                                                 "/org/deepin/linglong/PackageManager", QDBusConnection::systemBus());
//     linglong::service::UninstallParamOption uninstallParam;
//     uninstallParam.appId = "";
//     uninstallParam.version = "5.7.16.1";
//     QDBusPendingReply<linglong::service::Reply> dbusReply = pm.Uninstall(uninstallParam);
//     dbusReply.waitForFinished();
//
//     linglong::service::Reply retReply = dbusReply.value();
//     EXPECT_NE(retReply.code, STATUS_CODE(kPkgUninstallSuccess));
//
//     // stop service
//     stop_ll_service();
// }

// TEST(Package, uninstall04)
//{
//     // start service
//     std::thread startQdbus(start_ll_service);
//     startQdbus.detach();
//     std::this_thread::sleep_for(std::chrono::seconds(1));
//
//     OrgDeepinLinglongPackageManagerInterface pm("org.deepin.linglong.PackageManager",
//                                                 "/org/deepin/linglong/PackageManager", QDBusConnection::systemBus());
//     linglong::service::UninstallParamOption uninstallParam;
//     uninstallParam.appId = "com.belmoussaoui.Decoder";
//     uninstallParam.repoPoint = "flatpak";
//
//     QDBusPendingReply<linglong::service::Reply> dbusReply = pm.Uninstall(uninstallParam);
//     dbusReply.waitForFinished();
//
//     linglong::service::Reply retReply = dbusReply.value();
//     EXPECT_NE(retReply.code, STATUS_CODE(kPkgUninstallSuccess));
//
//     // stop service
//     stop_ll_service();
// }
