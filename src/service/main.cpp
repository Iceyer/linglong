/*
 * Copyright (c) 2020-2021. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     Iceyer <me@iceyer.net>
 *
 * Maintainer: Iceyer <me@iceyer.net>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <QCoreApplication>
#include <qloggingcategory.h>

#include "dbus_gen_app_manager_adaptor.h"
#include "module/dbus_ipc/dbus_app_manager_common.h"
#include "module/runtime/app.h"
#include "module/runtime/runtime.h"
#include "module/util/log/log_handler.h"

#include "impl/register_meta_type.h"

int main(int argc, char *argv[])
{
    using namespace linglong;

    QCoreApplication app(argc, argv);
    QCoreApplication::setOrganizationName("deepin");

    // 安装消息处理函数
    LOG_HANDLER->installMessageHandler();

    linglong::runtime::registerAllMetaType();
    linglong::package::registerAllMetaType();
    linglong::service::registerAllMetaType();

    QCommandLineParser parser;
    QCommandLineOption optBus("bus", "service bus address", "bus");
    optBus.setFlags(QCommandLineOption::HiddenFromHelp);

    parser.addOptions({optBus});
    parser.parse(QCoreApplication::arguments());

    AppManagerAdaptor appManagerAdaptor(service::AppManager::instance());

    QScopedPointer<QDBusServer> dbusServer;

    if (parser.isSet(optBus)) {
        auto busAddress = parser.value(optBus);
        qDebug() << "service address:" << busAddress;
        dbusServer.reset(new QDBusServer(busAddress));
        if (!dbusServer->isConnected()) {
            qCritical() << "dbusServer is not connected" << dbusServer->lastError();
            return -1;
        }
        QObject::connect(dbusServer.data(), &QDBusServer::newConnection, [](const QDBusConnection &conn) {
            // FIXME: work round to keep conn alive, but we finally need to free clientConn.
            auto clientConn = new QDBusConnection(conn);
            registerServiceAndObject(clientConn, "", {{AppManagerDBusPath, service::AppManager::instance()}});
        });
    } else {
        auto bus = QDBusConnection::sessionBus();
        if (!registerServiceAndObject(&bus, AppManagerDBusServiceName,
                                      {{AppManagerDBusPath, service::AppManager::instance()}})) {
            return -1;
        }
        app.connect(&app, &QCoreApplication::aboutToQuit, &app, [&] {
            bus.unregisterObject(AppManagerDBusPath);
            bus.unregisterService(AppManagerDBusServiceName);
        });
    }

    return QCoreApplication::exec();
}
