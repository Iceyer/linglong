/*
 * Copyright (c) 2021. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     Iceyer <me@iceyer.net>
 *
 * Maintainer: Iceyer <me@iceyer.net>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QObject>

#include "module/util/result.h"

namespace linglong {
namespace builder {

class Builder
{
public:
    virtual util::Error create(const QString &projectName) = 0;

    virtual util::Error build() = 0;

    virtual util::Error exportBundle(const QString &outputFilepath, bool useLocalDir) = 0;

    virtual util::Error push(const QString &ref) = 0;

    virtual util::Error push(const QString &bundleFilePath, const QString &repoUrl,
                                       const QString &repoChannel, bool force) = 0;

    virtual util::Error run() = 0;
};

void registerAllMetaType();

} // namespace builder
} // namespace linglong
