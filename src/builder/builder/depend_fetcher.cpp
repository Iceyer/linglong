/*
 * Copyright (c) 2022. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     Iceyer <me@iceyer.net>
 *
 * Maintainer: Iceyer <me@iceyer.net>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <QDir>
#include "project.h"

#include "module/repo/ostree_repo.h"

#include "builder_config.h"
#include "depend_fetcher.h"
#include "project.h"

namespace linglong {
namespace builder {

class DependFetcherPrivate
{
public:
    explicit DependFetcherPrivate(const BuildDepend &bd, Project *parent)
        : ref(fuzzyRef(&bd)), project(parent), buildDepend(&bd), dependType(bd.type)
    {
    }
    //TODO: dependType should be removed, buildDepend include it
    package::Ref ref;
    Project *project;
    const BuildDepend *buildDepend;
    QString dependType;
};
DependFetcher::DependFetcher(const BuildDepend &bd, Project *parent)
    : QObject(parent)
    , dd_ptr(new DependFetcherPrivate(bd, parent))
{
}

DependFetcher::~DependFetcher() = default;

linglong::util::Error DependFetcher::fetch(const QString &subPath, const QString &targetPath)
{
    repo::OSTreeRepo ostree(BuilderConfig::instance()->repoPath());
    // depends with source > depends from remote > depends from local
    auto dependRef = package::Ref("", dd_ptr->ref.appId, dd_ptr->ref.version, dd_ptr->ref.arch);

    if (!dd_ptr->buildDepend->source) {
        dependRef = package::Ref("", "linglong", dd_ptr->ref.appId, dd_ptr->ref.version, dd_ptr->ref.arch, "");
        if ("latest" == dd_ptr->ref.version) {
            dependRef = ostree.remoteLatestRef(dependRef);
        }

        qInfo() << QString("fetching dependency: %1 %2").arg(dependRef.appId).arg(dependRef.version);

        auto ret = ostree.pullAll(dependRef, true);
        if (!ret.success()) {
            return NewError(ret, -1, "pull " + dependRef.toString() + " failed");
        }
    }

    QDir targetParentDir(targetPath);
    targetParentDir.cdUp();
    targetParentDir.mkpath(".");

   
    auto ret = ostree.checkoutAll(dependRef, subPath, targetPath);

    if (!ret.success()) {
        return NewError(ret, -1, QString("ostree checkout %1 failed").arg(dependRef.toLocalRefString()));
    }
    // for app,lib. if the dependType match runtime, should be submitted together.
    if (dd_ptr->dependType == kDependTypeRuntime) {
        auto targetInstallPath = dd_ptr->project->config().cacheAbsoluteFilePath(
            {"overlayfs", "up", dd_ptr->project->config().targetInstallPath("")});

        ret = ostree.checkoutAll(dependRef, subPath, targetInstallPath);
    }

    return WrapError(ret, QString("ostree checkout %1 with subpath '%2' to %3")
                              .arg(dependRef.toLocalRefString())
                              .arg(subPath)
                              .arg(targetPath));
}

} // namespace builder
} // namespace linglong
