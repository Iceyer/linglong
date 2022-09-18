/*
 * Copyright (c) 2020-2021. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     Iceyer <me@iceyer.net>
 *
 * Maintainer: Iceyer <me@iceyer.net>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef LINGLONG_SRC_MODULE_PACKAGE_REF_H_
#define LINGLONG_SRC_MODULE_PACKAGE_REF_H_

#include <QString>

#include "module/util/const.h"

namespace linglong {
namespace package {

class Ref
{
public:
    explicit Ref(const QString &appId);

    Ref(const QString &remote, const QString &appId, const QString &version, const QString &arch)
        : repo(remote)
        , appId(appId)
        , version(version)
        , arch(arch)
    {
        channel = "";
        module = "";
    }

    Ref(const QString &remote, const QString &appId, const QString &version, const QString &arch, const QString &module)
        : repo(remote)
        , appId(appId)
        , version(version)
        , arch(arch)
        , module(module)
    {
    }

    Ref(const QString &remote, const QString &channel, const QString &appId, const QString &version,
        const QString &arch, const QString &module)
        : repo(remote)
        , channel(channel)
        , appId(appId)
        , version(version)
        , arch(arch)
        , module(module)
    {
        autoFill();
    }

    /*!
     * toOSTreeRef return
     * {repo}:/{channel}/{id}/{version}/{arch}/{module}
     * or
     * {channel}/{id}/{version}/{arch}/{module}
     * @return
     */
    QString toOSTreeRefString() const;

    /*!
     * toSpecString return {repo}/{channel}:{id}/{version}/{arch}/{module}
     * @return
     */
    QString toSpecString() const
    {
        return QString("%1/%2:%3/%4/%5/%6").arg(repo, channel, appId, version, arch, module);
    }

    Q_DECL_DEPRECATED QString toString() const
    {
        QString ref = repo.isEmpty() ? "" : repo + ":";
        QString channelRef = channel.isEmpty() ? "" : channel + "/";
        QString moduleRef = module.isEmpty() ? "" : "/" + module;

        return QString(ref + channelRef + "%1/%2/%3" + moduleRef).arg(appId, version, arch);
    }

    // FIXME: local().toString()?
    Q_DECL_DEPRECATED QString toLocalRefString() const { return QString("%1/%2/%3").arg(appId, version, arch); }

    Q_DECL_DEPRECATED QString toLocalFullRef() const
    {
        return QString("%1/%2/%3/%4").arg(appId, version, arch, module);
    }

    QString repo;
    QString channel;
    QString appId;
    QString version;
    QString arch;
    QString module;

private:
    void autoFill();
};

} // namespace package
} // namespace linglong

#endif /* LINGLONG_SRC_MODULE_PACKAGE_REF_H_ */
