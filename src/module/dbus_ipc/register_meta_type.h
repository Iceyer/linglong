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

#include "mutex"

#include "module/util/serialize/json.h"
#include "module/package/package.h"

#include "reply.h"
#include "module/dbus_ipc/param_option.h"

namespace linglong {
namespace service {

inline void registerAllMetaType()
{
    static std::once_flag flag;
    std::call_once(flag, []() {
        qDBusRegisterMetaType<linglong::service::Reply>();
        qDBusRegisterMetaType<linglong::service::QueryReply>();
        qDBusRegisterMetaType<linglong::service::ParamOption>();
        qDBusRegisterMetaType<linglong::service::DownloadParamOption>();
        qDBusRegisterMetaType<linglong::service::InstallParamOption>();
        qDBusRegisterMetaType<linglong::service::QueryParamOption>();
        qDBusRegisterMetaType<linglong::service::UninstallParamOption>();
        qDBusRegisterMetaType<QStringMap>();
        qDBusRegisterMetaType<QVariantMapList>();
        qDBusRegisterMetaType<linglong::service::RunParamOption>();
        qDBusRegisterMetaType<linglong::service::ExecParamOption>();
    });
}

} // namespace service
} // namespace linglong
