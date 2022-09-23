/*
 * Copyright (c) 2022. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     Iceyer <me@iceyer.net>
 *
 * Maintainer: Iceyer <me@iceyer.net>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef LINGLONG_SRC_PACKAGE_MANAGER_IMPL_PACKAGE_DATABASE_H_
#define LINGLONG_SRC_PACKAGE_MANAGER_IMPL_PACKAGE_DATABASE_H_

#include "module/util/result.h"
#include "module/package/ref.h"

namespace linglong {
namespace package_manager {

class PackageDatabase
{
public:
    bool isPackageInstalled(const QString &username, const package::Ref &ref);
    util::Error findPackage(const QString &username, const package::Ref &ref);
    util::Error remove(const QString &username, const package::Ref &ref);
};

} // namespace package_manager
} // namespace linglong

#endif // LINGLONG_SRC_PACKAGE_MANAGER_IMPL_PACKAGE_DATABASE_H_
