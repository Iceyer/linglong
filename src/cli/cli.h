/*
 * Copyright (c) 2021. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     Iceyer <me@iceyer.net>
 *
 * Maintainer: Iceyer <me@iceyer.net>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef LINGLONG_SRC_CLI_CLI_H_
#define LINGLONG_SRC_CLI_CLI_H_

#include <QtCore/QObject>
#include <QtCore/QFile>
#include <QtDBus/QDBusInterface>

namespace linglong {
namespace cli {

/*!
 * cli need for legacy slot for dbus
 */
class Cli : public QObject
{
    Q_OBJECT

public Q_SLOTS:
    void onJobProgressChange(quint32);
};

}
}

#endif // LINGLONG_SRC_CLI_CLI_H_
