/*
 * Copyright (c) 2022. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     Iceyer <me@iceyer.net>
 *
 * Maintainer: Iceyer <me@iceyer.net>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef LINGLONG_SRC_MODULE_UTIL_CONST_H_
#define LINGLONG_SRC_MODULE_UTIL_CONST_H_

namespace linglong {

const auto kDefaultRepo = "linglong";
// fix to modify after linglong-server
// const auto kDefaultChannel = "main";
const auto kDefaultChannel = "linglong";
const auto kDefaultVersion = "latest";
// const auto kDefaultModule = "binary";
const auto kDefaultModule = "runtime";

const auto kMainModule = kDefaultModule;

} // namespace linglong

#endif // LINGLONG_SRC_MODULE_UTIL_CONST_H_
