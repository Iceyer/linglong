/*
 * Copyright (c) 2020-2021. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     Iceyer <me@iceyer.net>
 *
 * Maintainer: Iceyer <me@iceyer.net>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <gtest/gtest.h>

#include "src/module/package/info.h"

using namespace linglong;

TEST(Package, Ref)
{
    package::Ref ref("repo/channel:app/1.0/x86");
    EXPECT_EQ(ref.repo, "repo");
    EXPECT_EQ(ref.appId, "app");

    ref = package::Ref("channel:app/1.0/x86");
    EXPECT_EQ(ref.repo, kDefaultRepo);
    EXPECT_EQ(ref.appId, "app");

    ref = package::Ref("app/1.0/x86");
    EXPECT_EQ(ref.repo, kDefaultRepo);
    EXPECT_EQ(ref.channel, kDefaultChannel);
    EXPECT_EQ(ref.appId, "app");
    EXPECT_EQ(ref.version, "1.0");
    EXPECT_EQ(ref.arch, "x86");
}
