/*
 * Copyright (c) 2021. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     Iceyer <me@iceyer.net>
 *
 * Maintainer: Iceyer <me@iceyer.net>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "cli.h"

#include <iostream>

#include <QDebug>
#include <QDateTime>
#include <QCoreApplication>

#include "module/util/job/job_controller.h"

namespace linglong {
namespace cli {

QString formatRemaining(qint64 remaining)
{
    if (remaining < 0) {
        return "--s";
    }
    return QString("%1s").arg(remaining);
}

QString formatRate(qint64 rate)
{
    if (rate > 1024 * 1024) {
        return QString("%1 MB/s").arg(double(rate) / 1024.0 / 1024.0, 0, 'f', 2);
    }
    if (rate > 1024) {
        return QString("%1 KB/s").arg(double(rate) / 1024.0, 0, 'f', 2);
    }
    return QString("%1 B/s").arg(rate);
}

void Cli::onJobProgressChanged(quint32 progress, quint64 rate, quint64 averageRate, qint64 remaining,
                               const QString &message)
{
    auto remainingFormatStr = formatRemaining(remaining);
    auto averageRateFormatStr = formatRate(averageRate);
    //    qDebug() << "\n" << progress << averageRate << remaining << message << "\n\n";
    QString output = QString("[%1%][%2][remaining %3] %4")
                         .arg(progress)
                         .arg(averageRateFormatStr)
                         .arg(remainingFormatStr)
                         .arg(message);
    std::cout << "\r\33[K" << output.toStdString();
    std::cout.flush();
}

void Cli::onFinish(quint32, const QString &message)
{
    QString output = QString("%1").arg(message);
    std::cout << std::endl;
    std::cout << output.toStdString();
    std::cout << std::endl;
    std::cout.flush();
    // FIXME: use signal to quit
    qApp->exit(0);
}

int Cli::runJob(std::function<QDBusPendingReply<QString>()> funcCreateJob,
                std::function<QDBusInterface *(const QString &path)> funcCreateJobInterface, bool useSignal)
{
    auto dbusReply = funcCreateJob();
    dbusReply.waitForFinished();
    QString jobPath = dbusReply.value();

    if (dbusReply.isError()) {
        qDebug() << dbusReply.error().type() << dbusReply.error().name();

        qCritical().noquote() << dbusReply.error().message();
        return dbusReply.error().type();
    }

    // not an async task, just get result now
    if (jobPath.isEmpty()) {
        qDebug() << "no job path, job has finish";
        return 0;
    }

    QScopedPointer<QDBusInterface> jobInterface(funcCreateJobInterface(jobPath));
    if (useSignal) {
        connect(jobInterface.data(), SIGNAL(ProgressChanged(quint32, quint64, quint64, qint64, QString)), this,
                SLOT(onJobProgressChanged(quint32, quint64, quint64, qint64, QString)));
        connect(jobInterface.data(), SIGNAL(Finish(quint32, QString)), this, SLOT(onFinish(quint32, QString)));
        return qApp->exec();
    } else {
        while (jobInterface.data()->property("State").toUInt() != static_cast<quint32>(util::JobStateFinish)) {
            QThread::sleep(1);
        }
        auto statusCode = jobInterface.data()->property("StatusCode").toUInt();
        return statusCode;
    }
}

} // namespace cli
} // namespace linglong
