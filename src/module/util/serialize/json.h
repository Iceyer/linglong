/*
 * Copyright (c) 2020-2021. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     Iceyer <me@iceyer.net>
 *
 * Maintainer: Iceyer <me@iceyer.net>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QDebug>
#include <QFile>

#include "module/util/serialize/serialize.h"

namespace linglong {
namespace util {

template<typename T>
static T *loadJSON(const QString &filepath)
{
    QFile jsonFile(filepath);
    jsonFile.open(QIODevice::ReadOnly);
    auto json = QJsonDocument::fromJson(jsonFile.readAll());
    return fromVariant<T>(json.toVariant());
}

template<typename T>
static T *loadJSONString(const QString &jsonString)
{
    auto json = QJsonDocument::fromJson(jsonString.toLocal8Bit());
    return fromVariant<T>(json.toVariant());
}

template<typename LIST>
static LIST arrayFromJson(const QString &str)
{
    auto json = QJsonDocument::fromJson(str.toLocal8Bit());
    return fromVariantList<LIST>(json.toVariant());
}

template<typename T>
static T *loadJsonBytes(const QByteArray &jsonString)
{
    auto json = QJsonDocument::fromJson(jsonString);
    return fromVariant<T>(json.toVariant());
}

} // namespace util
} // namespace linglong
