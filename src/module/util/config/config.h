/*
 * Copyright (c) 2020. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     Iceyer <me@iceyer.net>
 *
 * Maintainer: Iceyer <me@iceyer.net>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef LINGLONG_SRC_MODULE_UTIL_CONFIG_CONFIG_H_
#define LINGLONG_SRC_MODULE_UTIL_CONFIG_CONFIG_H_

#include "module/util/serialize/serialize.h"
#include "module/util/serialize/json.h"
#include "module/util/const.h"

namespace linglong {
namespace config {

class Repo : public Serialize
{
    Q_OBJECT;
    Q_SERIALIZE_CONSTRUCTOR(Repo)
    Q_SERIALIZE_PROPERTY(QString, endpoint);
};
Q_SERIALIZE_DECLARE_TYPE(Repo)

class Config : public Serialize
{
    Q_OBJECT;
    Q_SERIALIZE_PROPERTY(linglong::config::RepoStrMap, repos)
public:
    explicit Config(QObject *parent = nullptr);
    virtual void onPostSerialize() override;
    void save();
};
Q_SERIALIZE_DECLARE_TYPE(Config)

void registerAllMetatype();

} // namespace config
} // namespace linglong

linglong::config::Config &ConfigInstance();

Q_SERIALIZE_DECLARE_METATYPE_NM(linglong::config, Repo)
Q_SERIALIZE_DECLARE_METATYPE_NM(linglong::config, Config)

#endif // LINGLONG_SRC_MODULE_UTIL_CONFIG_CONFIG_H_