/*
 * Copyright (c) 2022. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     Iceyer <me@iceyer.net>
 *
 * Maintainer: Iceyer <me@iceyer.net>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef LINGLONG_SRC_MODULE_RUNTIME_RUNTIME_H_
#define LINGLONG_SRC_MODULE_RUNTIME_RUNTIME_H_

#include "oci.h"
#include "app.h"

namespace linglong {
namespace runtime {

// TODO: move to runtime.cpp
inline void registerAllMetaType()
{
    registerAllOciMetaType();

    static std::once_flag flag;
    std::call_once(flag, []() {
        qSerializeRegister<linglong::runtime::Layer>();
        qSerializeRegister<linglong::runtime::MountYaml>();
        qSerializeRegister<linglong::runtime::AppPermission>();
        qSerializeRegister<linglong::runtime::App>();
        qSerializeRegister<linglong::runtime::Container>();
    });
}

} // namespace runtime
} // namespace linglong

#endif // LINGLONG_SRC_MODULE_RUNTIME_RUNTIME_H_
