/*
 * Copyright (c) 2022. ${ORGANIZATION_NAME}. All rights reserved.
 *
 * Author:     Iceyer <me@iceyer.net>
 *
 * Maintainer: Iceyer <me@iceyer.net>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef LINGLONG_SRC_SERVICE_IMPL_SERVICE_DBUS_COMMON_H_
#define LINGLONG_SRC_SERVICE_IMPL_SERVICE_DBUS_COMMON_H_

#include <QDBusConnection>
namespace linglong {
namespace service {

inline QDBusConnection targetConnection()
{
    return QDBusConnection::systemBus();
}
} // namespace service
} // namespace linglong

#endif // LINGLONG_SRC_SERVICE_IMPL_SERVICE_DBUS_COMMON_H_
