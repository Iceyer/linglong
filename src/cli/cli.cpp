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
namespace linglong {
namespace cli {

void Cli::onJobProgressChange(quint32 progress)
{
    qDebug() << "current" << progress;
}

} // namespace cli
} // namespace linglong