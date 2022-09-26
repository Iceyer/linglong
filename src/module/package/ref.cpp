/*
 * Copyright (c) 2020-2021. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     Iceyer <me@iceyer.net>
 *
 * Maintainer: Iceyer <me@iceyer.net>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "ref.h"

#include <QStringList>
#include <QDebug>

#include "module/util/sysinfo.h"

namespace linglong {
namespace package {

/*!
 * deepin/channel:appId/version/arch -> appId/version -> appId
 * \param ref string
 */
Ref parseSpecRef(const QString &ref)
{
    QStringList slice = ref.split(":");
    QString remote = "";
    QString localId = ref;
    switch (slice.length()) {
    case 2:
        remote = slice.value(0);
        localId = slice.value(1);
        break;
    case 1:
        localId = slice.value(0);
        break;
    default:
        qCritical() << "invalid ref" << ref;
    }

    QString repo = kDefaultRepo;
    QString channel = kDefaultChannel;

    if (!remote.isEmpty()) {
        slice = remote.split("/");
        switch (slice.length()) {
        case 2:
            repo = slice.value(0);
            channel = slice.value(1);
            break;
        case 1:
            channel = slice.value(0);
            break;
        default:
            qCritical() << "invalid remote" << remote;
        }
    }

    QString id = localId;
    // keep empty
    QString version = kDefaultVersion;
    QString arch = util::hostArch();
    QString module = kDefaultModule;

    slice = localId.split("/");
    switch (slice.length()) {
    case 4:
        module = slice.value(3);
    case 3:
        arch = slice.value(2);
    case 2:
        version = slice.value(1);
    case 1:
        id = slice.value(0);
        break;
    default:
        qCritical() << "invalid local id" << localId;
    }

    return Ref {repo, channel, id, version, arch, module};
}

Ref::Ref(const QString &ref)
{
    *this = parseSpecRef(ref);
}

void Ref::autoFill()
{
    if (repo.isEmpty()) {
        repo = kDefaultRepo;
    }

    if (arch.isEmpty()) {
        arch = util::hostArch();
    }

    if (channel.isEmpty()) {
        channel = "linglong";
    }

    if (module.isEmpty()) {
        module = "runtime";
    }
}

QString Ref::toOSTreeRefString() const
{
    auto fixModule = module;
    auto fixRepo = repo;
    fixRepo = "repo";

    if (repo.isEmpty()) {
        // {channel}/{id}/{version}/{arch}/{module}
        return QString("%1/%2/%3/%4/%5").arg(channel, appId, version, arch, fixModule);
    }

    // {repo}:/{channel}/{id}/{version}/{arch}/{module}
    return QString("%1:%2/%3/%4/%5/%6").arg(fixRepo, channel, appId, version, arch, fixModule);
}

QString Ref::toOSTreeRefLocalString() const
{
    auto fixModule = module;

    if (channel.isEmpty()) {
        // {id}/{version}/{arch}/{module}
        return QString("%1/%2/%3/%4").arg(appId, version, arch, fixModule);
    }

    return QString("%1/%2/%3/%4/%5").arg(channel, appId, version, arch, fixModule);
}

} // namespace package
} // namespace linglong