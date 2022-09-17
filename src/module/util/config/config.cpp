/*
 * Copyright (c) 2020. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     Iceyer <me@iceyer.net>
 *
 * Maintainer: Iceyer <me@iceyer.net>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "config.h"

#include <fstream>
#include <mutex>

#include "module/util/file.h"
#include "module/util/yaml.h"

namespace linglong {

const char *const kConfigFileName = "config.yaml";
// TODO: check more path for different distribution
const char *const kDefaultConfigFilePath = "/usr/share/linglong/config.yaml";

static QString filePath()
{
    return util::getLinglongRootPath() + "/" + kConfigFileName;
}

namespace config {

void registerAllMetatype()
{
    qJsonRegister<linglong::config::Repo>();
    qJsonRegister<linglong::config::Config>();
}

Config::Config(QObject *parent)
    : Serialize(parent)
{
    static std::once_flag flag;
    std::call_once(flag, []() { registerAllMetatype(); });

    auto configFilePath = filePath();
    if (!util::fileExists(configFilePath) && util::fileExists(kDefaultConfigFilePath)) {
        QFile configFile(kDefaultConfigFilePath);
        configFile.copy(configFilePath);
    }
}

void Config::onPostSerialize()
{
    // make sure default repo exist
    if (!repos.contains(kDefaultRepo)) {
        repos.insert(kDefaultRepo, new Repo(this));
    };
}

void Config::save()
{
    auto node = toYaml<Config>(this);
    std::ofstream configPathOut(filePath().toStdString());
    configPathOut << node;
    configPathOut.close();
}

} // namespace config

config::Config &ConfigInstance()
{
    static QScopedPointer<config::Config> config(formYaml<config::Config>(YAML::LoadFile(filePath().toStdString())));
    return *config;
}

} // namespace linglong
