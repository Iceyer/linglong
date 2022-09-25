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

#include "module/util/serialize/json.h"

#include <tuple>

namespace linglong {
namespace util {

/*!
 * Error is a stack trace error handler when function return
 */
class Error
{
public:
    Error() = default;
    Error(const Error &err)
    {
        msgMetaList = err.msgMetaList;
    }

    Error(const char *file, int line, const char *func, const Error &base, int code = 0, const QString &msg = "")
        : msgMetaList(base.msgMetaList)
    {
        msgMetaList.push_back(MessageMeta {
            .file = file,
            .line = line,
            .func = func,
            .code = code,
            .message = msg,
        });
    };

    Error(const char *file, int line, const char *func, int code = 0, const QString &msg = "")
    {
        msgMetaList.push_back(MessageMeta {
            .file = file,
            .line = line,
            .func = func,
            .code = code,
            .message = msg,
        });
    };

    Error(const char *file, int line, const char *func, const QString &msg, const Error &base)
        : msgMetaList(base.msgMetaList)
    {
        msgMetaList.push_back(MessageMeta {
            .file = file,
            .line = line,
            .func = func,
            .code = base.code(),
            .message = msg,
        });
    };

    Error(const char *file, int line, const char *func, int code, const QString &msg, const Error &base)
        : msgMetaList(base.msgMetaList)
    {
        msgMetaList.push_back(MessageMeta {
            .file = file,
            .line = line,
            .func = func,
            .code = code,
            .message = msg,
        });
    };

    int code() const
    {
        Q_ASSERT(msgMetaList.size());
        return msgMetaList.last().code;
    }

    bool success() const { return 0 == code(); }

    QString message() const
    {
        Q_ASSERT(msgMetaList.size());
        return msgMetaList.last().message;
    }

    Error &operator<<(const QString &msg)
    {
        Q_ASSERT(msgMetaList.size());
        msgMetaList.last().message = msg;
        return *this;
    }

    Error &operator<<(int code)
    {
        Q_ASSERT(msgMetaList.size());
        msgMetaList.last().code = code;
        return *this;
    }

    QString toJson() const
    {
        Q_ASSERT(msgMetaList.size());
        QJsonObject obj;
        obj["code"] = msgMetaList.last().code;
        obj["message"] = msgMetaList.last().message;
        return QJsonDocument(obj).toJson(QJsonDocument::Compact);
    }

    friend QDebug operator<<(QDebug d, const util::Error &result);

private:
    struct MessageMeta {
        const char *file;
        int line;
        const char *func;
        int code;
        QString message;
    };

    QList<MessageMeta> msgMetaList {};
};

inline QDebug operator<<(QDebug dbg, const Error &result)
{
    dbg << "\n";
    for (const auto &meta : result.msgMetaList) {
        dbg << QString(meta.file) + ":" + QString("%1").arg(meta.line) + ":" + QString(meta.func) << "\n";
        dbg << meta.code << meta.message << "\n";
    }
    return dbg;
}

} // namespace util
} // namespace linglong

#define NewError(...) linglong::util::Error(__FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)

#define NoError() linglong::util::Error(__FILE__, __LINE__, __FUNCTION__)

#define WrapError(base, ...) linglong::util::Error(__FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__, base)
