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

#include <QDebug>
#include <QDateTime>
#include <QCoreApplication>
#include <iostream>

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

void Cli::onFinish(quint32 , const QString &message)
{
    QString output = QString("%1").arg(message);
    std::cout << std::endl;
    std::cout << output.toStdString();
    std::cout.flush();
    // FIXME: use signal to quit
    qApp->exit(0);
}

} // namespace cli
} // namespace linglong