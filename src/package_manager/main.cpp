/*
 * Copyright (c) 2020-2021. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     HuQinghong <huqinghong@uniontech.com>
 *
 * Maintainer: HuQinghong <huqinghong@uniontech.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <QCoreApplication>

#include "service/impl/register_meta_type.h"
#include "module/util/log/log_handler.h"
#include "module/repo/repo.h"
#include "module/dbus_ipc/dbus_package_manager_common.h"

#include "dbus_gen_package_manager_adaptor.h"
#include "dbus_gen_job_manager_adaptor.h"

using namespace linglong;

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setOrganizationName("deepin");
    // 安装消息处理函数
    LOG_HANDLER->installMessageHandler();

    linglong::package::registerAllMetaType();
    linglong::service::registerAllMetaType();
    linglong::repo::registerAllMetaType();

    QCommandLineParser parser;
    QCommandLineOption optBus("bus", "service bus address", "bus");
    optBus.setFlags(QCommandLineOption::HiddenFromHelp);

    parser.addOptions({optBus});
    parser.parse(QCoreApplication::arguments());

    PackageManagerAdaptor packageManagerAdaptor(package_manager::PackageManager::instance());
    JobManagerAdaptor jma(package_manager::JobManager::instance());

    QScopedPointer<QDBusServer> dbusServer;
    if (parser.isSet(optBus)) {
        auto busAddress = parser.value(optBus);
        qDebug() << "bus address" << busAddress;
        dbusServer.reset(new QDBusServer(busAddress));
        if (!dbusServer->isConnected()) {
            qCritical() << "dbusServer is not connected" << dbusServer->lastError();
            return -1;
        }
        QObject::connect(dbusServer.data(), &QDBusServer::newConnection, [](const QDBusConnection &conn) {
            // FIXME: work round to keep conn alive, but we finally need to free clientConn.
            auto clientConn = new QDBusConnection(conn);
            registerServiceAndObject(clientConn, "",
                                     {{DBusPackageManagerPath, package_manager::PackageManager::instance()},
                                      {DBusPackageManagerJobManagerPath, package_manager::JobManager::instance()}});
        });
    } else {
        QDBusConnection bus = QDBusConnection::systemBus();
        if (!registerServiceAndObject(&bus, DBusPackageManagerServiceName,
                                      {{DBusPackageManagerPath, package_manager::PackageManager::instance()},
                                       {DBusPackageManagerJobManagerPath, package_manager::JobManager::instance()}})) {
            return -1;
        }
    }
    return QCoreApplication::exec();
}
