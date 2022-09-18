/*
 * Copyright (c) 2021. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     Iceyer <me@iceyer.net>
 *
 * Maintainer: Iceyer <me@iceyer.net>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "repo.h"

#include <QString>
#include <QDir>

#include "module/util/sysinfo.h"
#include "module/package/info.h"

#include "repo_client.h"
#include "ostree_repo.h"

namespace linglong {
namespace repo {

void registerAllMetaType()
{
    qSerializeRegister<QStringMap>();
    qSerializeRegister<linglong::repo::InfoResponse>();
    qSerializeRegister<linglong::repo::Response>();
    qSerializeRegister<linglong::repo::RevPair>();
    qSerializeRegister<linglong::repo::UploadTaskRequest>();
}

} // namespace repo
} // namespace linglong
