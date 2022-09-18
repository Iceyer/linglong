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

#include <QMetaProperty>
#include <QObject>
#include <QDBusArgument>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPointer>
#include <QDBusMetaType>
#include <QDebug>
#include <QFile>

#include "module/util/serialize/serialize.h"

// deprecated, use Q_SERIALIZE_CONSTRUCTOR
#define Q_JSON_CONSTRUCTOR(TYPE) Q_SERIALIZE_CONSTRUCTOR(TYPE)

// deprecated, use Q_SERIALIZE_PROPERTY
#define Q_JSON_PROPERTY(TYPE, PROP) Q_SERIALIZE_PROPERTY(TYPE, PROP)

// deprecated, use Q_SERIALIZE_ITEM_MEMBER
#define Q_JSON_ITEM_MEMBER(TYPE, PROP, MEMBER_NAME) Q_SERIALIZE_ITEM_MEMBER(TYPE, PROP, MEMBER_NAME)

// deprecated, use Q_JSON_ITEM_MEMBER_PTR
#define Q_JSON_PTR_PROPERTY(TYPE, PROP) Q_JSON_ITEM_MEMBER_PTR(TYPE, PROP, PROP)

// deprecated, use Q_SERIALIZE_ITEM_MEMBER_PTR
#define Q_JSON_ITEM_MEMBER_PTR(TYPE, PROP, MEMBER_NAME) Q_SERIALIZE_ITEM_MEMBER_PTR(TYPE, PROP, MEMBER_NAME)

// deprecated, use Q_SERIALIZE_DECLARE
#define D_SERIALIZE_DECLARE(TYPE) Q_SERIALIZE_DECLARE_TYPE(TYPE)

// deprecated, use Q_SERIALIZE_REGISTER_TYPE
#define D_SERIALIZE_REGISTER_TYPE_NM(NM, TYPE) Q_SERIALIZE_DECLARE_METATYPE_NM(NM::TYPE, NM::TYPE##List, NM::TYPE##StrMap)

// deprecated, use Q_SERIALIZE_DECLARE_TYPE_AND_METATYPE_NM
#define Q_JSON_DECLARE_PTR_METATYPE_NM(NM, TYPE) Q_SERIALIZE_DECLARE_TYPE_AND_METATYPE_NM(NM, TYPE)

// deprecated, use Q_SERIALIZE_DECLARE_TYPE_AND_METATYPE
#define Q_JSON_DECLARE_PTR_METATYPE(TYPE) Q_SERIALIZE_DECLARE_TYPE_AND_METATYPE(TYPE)

// deprecated, use qSerializeRegister
#define qJsonRegister qSerializeRegister

// TODO: move to serialize.h, and update all CMakeLists.txt
class Serialize : public QObject
{
    Q_OBJECT
protected:
    explicit Serialize(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

public:
    QStringList keys() const
    {
        QStringList jsonKeys;
        auto mo = this->metaObject();
        for (int i = mo->propertyOffset(); i < mo->propertyCount(); ++i) {
            jsonKeys.push_back(mo->property(i).name());
        }
        return jsonKeys;
    }

    template<typename T>
    static QJsonValue toJson(T *m)
    {
        return toVariant<T>(m).toJsonValue();
    }

    template<typename T>
    static QByteArray dump(T *m)
    {
        QJsonDocument doc(toJson(m).toObject());
        return doc.toJson();
    }

public:
    virtual void onPostSerialize() { }
};

typedef Serialize JsonSerialize;

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
