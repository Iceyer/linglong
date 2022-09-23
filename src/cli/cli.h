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
#include <QtDBus/QDBusPendingReply>

namespace linglong {
namespace cli {

/*!
 * cli need for legacy slot for dbus
 */
class Cli : public QObject
{
    Q_OBJECT
public:
    int runJob(std::function<QDBusPendingReply<QString>()> funcCreateJob,
               std::function<QDBusInterface *(const QString &path)> funcCreateJobInterface, bool useSignal);

public Q_SLOTS:
    void onJobProgressChanged(quint32 progress, quint64 rate, quint64 averageRate, qint64 remaining,
                              const QString &message);
    void onFinish(quint32, const QString &);
};

} // namespace cli
} // namespace linglong

#endif // LINGLONG_SRC_CLI_CLI_H_
