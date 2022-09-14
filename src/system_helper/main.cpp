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

#include "system_helper.h"
#include "privilege/privilege_install_portal.h"

int main(int argc, char *argv[])
{
    using namespace linglong::system::helper;
    QCoreApplication app(argc, argv);

    qJsonRegister<linglong::system::helper::FilePortalRule>();
    qJsonRegister<linglong::system::helper::PortalRule>();

    SystemHelper systemHelper;
    SystemHelperAdaptor systemHelperAdaptor(&systemHelper);

    QDBusConnection dbus = QDBusConnection::systemBus();
    if (!dbus.registerService(linglong::SystemHelperDBusServiceName)) {
        qCritical() << "registerService failed" << dbus.lastError();
        return -1;
    }

    if (!dbus.registerObject(linglong::SystemHelperDBusPath, &systemHelper)) {
        qCritical() << "registerObject failed" << dbus.lastError();
        return -1;
    }

    return app.exec();
}