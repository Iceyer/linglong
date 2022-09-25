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
#include "module/util/serialize/yaml.h"

namespace linglong {
namespace config {

static const char *const kConfigFileName = "config.yaml";
// TODO: check more path for different distribution
static const char *const kDefaultConfigFilePath = "/usr/share/linglong/config.yaml";

void registerAllMetatype()
{
    qSerializeRegister<linglong::config::Repo>();
    qSerializeRegister<linglong::config::Config>();
}

static QString filePath()
{
    return util::getLinglongRootPath() + "/" + kConfigFileName;
}

static Config *loadConfig()
{
    static std::once_flag flag;
    std::call_once(flag, []() { linglong::config::registerAllMetatype(); });

    auto configFilePath = filePath();
    if (!util::fileExists(configFilePath) && util::fileExists(kDefaultConfigFilePath)) {
        QFile configFile(kDefaultConfigFilePath);
        configFile.copy(configFilePath);
    }

    Config *cfg = nullptr;
    try {
        cfg = formYaml<config::Config>(YAML::LoadFile(filePath().toStdString()));
        return cfg;
    } catch (...) {
        qWarning() << "load" << filePath() << "failed" << cfg;
        cfg = new Config;
        cfg->onPostSerialize();
    }

    Q_ASSERT(cfg);
    Q_ASSERT(cfg->repos.contains(kDefaultRepo));

    return cfg;
}

Config::Config(QObject *parent)
    : Serialize(parent)
{
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
} // namespace linglong

linglong::config::Config &ConfigInstance()
{
    static QScopedPointer<linglong::config::Config> config(linglong::config::loadConfig());
    return *config;
}
