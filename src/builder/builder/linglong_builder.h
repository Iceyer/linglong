/*
 * Copyright (c) 2022. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     Iceyer <me@iceyer.net>
 *
 * Maintainer: Iceyer <me@iceyer.net>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "builder.h"
#include "project.h"
#include "module/package/bundle.h"
#include "module/util/status_code.h"

namespace linglong {
namespace builder {

class LinglongBuilder
    : public QObject
    , public Builder
{
    Q_OBJECT
public:
    util::Error create(const QString &projectName) override;

    util::Error build() override;

    util::Error exportBundle(const QString &outputFilepath, bool useLocalDir) override;

    util::Error push(const QString &ref) override;

    util::Error push(const QString &bundleFilePath, const QString &repoUrl, const QString &repoChannel,
                               bool force) override;

    util::Error run() override;

    util::Error initRepo();

    util::Error buildFlow(Project* project);
};

// TODO: remove later
class message : public Serialize
{
    Q_OBJECT;
    Q_SERIALIZE_CONSTRUCTOR(message)
public:
    Q_SERIALIZE_PROPERTY(QString, type);
    Q_SERIALIZE_PROPERTY(int, pid);
    Q_SERIALIZE_PROPERTY(QString, arg0);
    Q_SERIALIZE_PROPERTY(int, wstatus);
    Q_SERIALIZE_PROPERTY(QString, information);
};
} // namespace builder
} // namespace linglong