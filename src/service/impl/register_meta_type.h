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

#include "module/util/serialize/json.h"
#include "module/package/package.h"
#include "reply.h"
#include "param_option.h"

namespace linglong {
namespace service {

inline void registerAllMetaType()
{
    qDBusRegisterMetaType<linglong::service::Reply>();
    qDBusRegisterMetaType<linglong::service::QueryReply>();
    qDBusRegisterMetaType<linglong::service::ParamOption>();
    qDBusRegisterMetaType<linglong::service::DownloadParamOption>();
    qDBusRegisterMetaType<linglong::service::InstallParamOption>();
    qDBusRegisterMetaType<linglong::service::QueryParamOption>();
    qDBusRegisterMetaType<linglong::service::UninstallParamOption>();
    qDBusRegisterMetaType<QStringMap>();
    qDBusRegisterMetaType<linglong::service::RunParamOption>();
    qDBusRegisterMetaType<linglong::service::ExecParamOption>();
}

} // namespace service
} // namespace linglong
