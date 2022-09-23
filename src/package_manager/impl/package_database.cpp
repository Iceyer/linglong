/*
 * Copyright (c) 2022. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     Iceyer <me@iceyer.net>
 *
 * Maintainer: Iceyer <me@iceyer.net>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "package_database.h"

#include "app_status.h"

namespace linglong {
namespace package_manager {

util::Error PackageDatabase::remove(const QString &username, const package::Ref &ref)
{
    // FIXME: now delete all user package
    linglong::util::deleteAppRecord(ref.appId, ref.version, ref.arch, ref.channel, ref.module, "");
    return NoError();
}

util::Error PackageDatabase::findPackage(const QString &username, const package::Ref &ref)
{
    auto version = ref.version;
    if (ref.version == kDefaultVersion) {
        version = "";
    }
    util::getAppInstalledStatus(ref.appId, version, ref.arch, ref.channel, ref.module, username);
    return util::Error();
}

bool PackageDatabase::isPackageInstalled(const QString &username, const package::Ref &ref)
{
    auto version = ref.version;
    if (ref.version == kDefaultVersion) {
        version = "";
    }
    return util::getAppInstalledStatus(ref.appId, version, ref.arch, ref.channel, ref.module, username);
}

} // namespace package_manager
} // namespace linglong