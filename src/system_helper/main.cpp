/*
 * Copyright (c) 2022. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     Iceyer <me@iceyer.net>
 *
 * Maintainer: Iceyer <me@iceyer.net>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <QDebug>
#include <QCoreApplication>
#include <QDBusConnection>

#include "module/dbus_ipc/dbus_system_helper_common.h"
#include "dbus_gen_system_helper_adaptor.h"
#include "module/util/log_handler.h"

#include "system_helper.h"
#include "privilege/privilege_install_portal.h"

bool registerServiceAndOjbect(QDBusConnection conn, linglong::system::helper::SystemHelper &systemHelper,
                              bool peerToPeer = false)
{
    if (!conn.isConnected()) {
        qCritical() << "bad connection" << conn.lastError();
        return false;
    }
    if (!peerToPeer) {
        if (!conn.registerService(linglong::SystemHelperDBusServiceName)) {
            qCritical() << "registerService failed" << conn.lastError();
            return false;
        }
    }

    if (!conn.registerObject(linglong::SystemHelperDBusPath, &systemHelper)) {
        qCritical() << "registerObject failed" << conn.lastError();
        return false;
    }
    return true;
}

int main(int argc, char *argv[])
{
    using namespace linglong::system::helper;
    QCoreApplication app(argc, argv);

    LOG_HANDLER->installMessageHandler();

    qSerializeRegister<linglong::system::helper::FilePortalRule>();
    qSerializeRegister<linglong::system::helper::PortalRule>();

    SystemHelper systemHelper;
    SystemHelperAdaptor systemHelperAdaptor(&systemHelper);

    QCommandLineParser parser;
    QCommandLineOption optBus("bus", "service bus address", "bus");
    optBus.setFlags(QCommandLineOption::HiddenFromHelp);

    parser.addOptions({optBus});
    parser.parse(QCoreApplication::arguments());

    QScopedPointer<QDBusServer> dbusServer;
    if (parser.isSet(optBus)) {
        auto busAddress = parser.value(optBus);
        dbusServer.reset(new QDBusServer(busAddress));
        if (!dbusServer->isConnected()) {
            qCritical() << "dbusServer is not connected" << dbusServer->lastError();
            return -1;
        }
        QObject::connect(dbusServer.data(), &QDBusServer::newConnection, [&systemHelper](const QDBusConnection &conn) {
            // FIXME: work round to keep conn alive, but we finally need to free clientConn.
            auto clientConn = new QDBusConnection(conn);
            registerServiceAndOjbect(*clientConn, systemHelper, true);
        });
    } else {
        QDBusConnection dbus = QDBusConnection::systemBus();
        if (!registerServiceAndOjbect(dbus, systemHelper)) {
            return -1;
        }
    }

    return app.exec();
}
