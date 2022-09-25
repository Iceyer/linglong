/*
 * Copyright (c) 2021. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     Iceyer <me@iceyer.net>
 *
 * Maintainer: Iceyer <me@iceyer.net>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <gtest/gtest.h>

#include <mutex>

#include "qt_yaml.h"
#include "serialize_test.h"

inline void registerSerializeTestMetaType()
{
    static std::once_flag flag;
    std::call_once(flag, []() {
        qSerializeRegister<TestMount>();
        qSerializeRegister<TestPermission>();
        qSerializeRegister<TestApp>();

        qSerializeRegister<linglong::test::MountRule>();
        qSerializeRegister<linglong::test::Permission>();
        qSerializeRegister<linglong::test::App>();
    });
}

auto yamlData = R"MLS00(version: 1.0

package:
  source: "linglong/main:org.deepin.demo/2.2.32/x86_64/binary"

runtime:
  source: "linglong/main:org.deepin.Runtime/20.5.0/x86_64/binary"
)MLS00";

TEST(Serialize, YAML_STRING)
{
    linglong::runtime::registerAllOciMetaType();

    YAML::Node doc = YAML::Load(yamlData);
    auto app = formYaml<TestApp>(doc);

    EXPECT_EQ(app->package->source, "linglong/main:org.deepin.demo/2.2.32/x86_64/binary");
}

TEST(Serialize, YAML_NS)
{
    registerSerializeTestMetaType();

    auto path = "../../test/data/demo/app.yml";
    YAML::Node doc = YAML::LoadFile(path);

    auto app = formYaml<linglong::test::App>(doc);

    EXPECT_EQ(app->permissions->mounts.value(0)->type, "test_type");
}

TEST(Serialize, YAML)
{
    linglong::runtime::registerAllOciMetaType();
    registerSerializeTestMetaType();

    auto path = "../../test/data/demo/app.yml";
    YAML::Node doc = YAML::LoadFile(path);

    auto app = formYaml<TestApp>(doc);

    EXPECT_EQ(app->root->parent(), app);
    EXPECT_EQ(app->namespaces.value(0)->parent(), app);
    EXPECT_EQ(app->version, "1.12");
    EXPECT_EQ(app->mounts.length(), 3);
    EXPECT_EQ(app->root->readonly, false);
    EXPECT_EQ(app->root->path, "/run/user/1000/linglong/ab24ae64edff4ddfa8e6922eb29e2baf");

    EXPECT_EQ(app->permissions->mounts.value(0)->type, "test_type");

    app->deleteLater();

    TestApp app2;

    app2.root = new Root;
    app2.root->readonly = true;
    app2.version = "2";

    auto doc2 = toYaml<TestApp>(&app2);

    EXPECT_EQ(doc2["version"].as<QString>(), "2");
    EXPECT_EQ(doc2["root"]["readonly"].as<QString>(), "true");
    EXPECT_EQ(doc2["root"]["readonly"].as<bool>(), true);

    app2.root->deleteLater();
}