/*
 * Copyright (c) 2022. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     Iceyer <me@iceyer.net>
 *
 * Maintainer: Iceyer <me@iceyer.net>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef LINGLONG_SRC_MODULE_DBUS_IPC_DBUS_COMMON_H_
#define LINGLONG_SRC_MODULE_DBUS_IPC_DBUS_COMMON_H_

#include <QVariantMap>

typedef QList<QVariantMap> QVariantMapList;

Q_DECLARE_METATYPE(QVariantMapList);

inline QVariant toVariant(const QVariantMapList &vml)
{
    QVariantList vl;
    for (auto const &pkg : vml) {
        vl.push_back(pkg);
    }
    return vl;
}

#endif // LINGLONG_SRC_MODULE_DBUS_IPC_DBUS_COMMON_H_
