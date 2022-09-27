/*
 * Copyright (c) 2020-2021. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     KeZhihuan <kezhihuan@uniontech.com>
 *
 * Maintainer: KeZhihuan <kezhihuan@uniontech.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "package_manager.h"
#include "package_manager_p.h"

#include <QDebug>
#include <QDBusInterface>
#include <QDBusReply>
#include <QJsonArray>

#include "dbus_gen_system_helper_interface.h"
#include "module/dbus_ipc/dbus_system_helper_common.h"
#include "module/dbus_ipc/dbus_package_manager_common.h"
#include "module/util/config/config.h"
#include "module/util/http/httpclient.h"
#include "module/util/sysinfo.h"
#include "module/util/runner.h"

#include "app_status.h"
#include "appinfo_cache.h"
#include "module/util/job/job.h"
#include "job_manager.h"

namespace linglong {
namespace package_manager {

namespace {
const char *const KeyInstallJobId = "jobId";
const char *const KeyInstallJobRef = "ref";
const char *const KeyInstallJobStageProgressBegin = "stageBeginPercent";
const char *const KeyInstallJobStageProgressEnd = "stageEndPercent";
} // namespace

package::Ref toRef(const InstallParamOption &installOption)
{
    QString appId = installOption.appId.trimmed();
    QString arch = installOption.arch.trimmed().toLower();
    QString version = installOption.version.trimmed();
    QString channel = installOption.channel.trimmed();
    QString module = installOption.appModule.trimmed();

    package::Ref ref("", channel, appId, version, arch, module);

    return ref;
}

void updateHostShare(const QString &sharePath)
{
    // 更新desktop database
    auto retRunner = runner::Runner("update-desktop-database", {sharePath + "/applications/"}, 1000 * 60 * 1);
    if (!retRunner) {
        qWarning() << "warning: update desktop database of " + sharePath + "/applications/ failed!";
    }

    // 更新mime type database
    if (linglong::util::dirExists(sharePath + "/mime/packages")) {
        auto retUpdateMime = runner::Runner("update-mime-database", {sharePath + "/mime/"}, 1000 * 60 * 1);
        if (!retUpdateMime) {
            qWarning() << "warning: update mime type database of " + sharePath + "/mime/ failed!";
        }
    }

    // 更新 glib-2.0/schemas
    if (linglong::util::dirExists(sharePath + "/glib-2.0/schemas")) {
        auto retUpdateSchemas =
            runner::Runner("glib-compile-schemas", {sharePath + "/glib-2.0/schemas"}, 1000 * 60 * 1);
        if (!retUpdateSchemas) {
            qWarning() << "warning: update schemas of " + sharePath + "/glib-2.0/schemas failed!";
        }
    }
}

QString getDBusCallerUsername(QDBusContext &ctx)
{
    QDBusReply<uint> dbusReply = ctx.connection().interface()->serviceUid(ctx.message().service());
    if (dbusReply.isValid()) {
        return util::getUserName(dbusReply.value());
    }
    // FIXME: must reject request
    qCritical() << "can not find caller uid";
    return "";
}

uid_t getDBusCallerUid(QDBusContext &ctx)
{
    if (!ctx.connection().interface()) {
        qCritical() << "can not get peer uid";
        return -1;
    }

    QDBusReply<uint> dbusReply = ctx.connection().interface()->serviceUid(ctx.message().service());

    if (dbusReply.isValid()) {
        return dbusReply.value();
    }
    // FIXME: must reject request
    qCritical() << "can not find caller uid";
    return -1;
}

std::unique_ptr<package::MetaInfo> takeLatestApp(const QString &appId, package::MetaInfoList &appList)
{
    int latestIndex = 0;
    QString curVersion = linglong::util::APP_MIN_VERSION;

    for (int index = 0; index < appList.size(); ++index) {
        auto item = appList.at(index);
        linglong::util::AppVersion dstVersion(curVersion);
        linglong::util::AppVersion iterVersion(item->version);
        if (appId == item->appId && iterVersion.isBigThan(dstVersion)) {
            curVersion = item->version;
            latestIndex = index;
        }
    }
    return std::unique_ptr<package::MetaInfo>(appList.takeAt(latestIndex).data());
}

PackageManagerPrivate::PackageManagerPrivate(PackageManager *parent)
    : entriesSharePath(linglong::util::getLinglongRootPath() + "/entries/share")
    , kAppInstallPath(linglong::util::getLinglongRootPath() + "/layers/")
    , kLocalRepoPath(linglong::util::getLinglongRootPath())
    , kRemoteRepoName("repo")
    , ostreeRepo(kLocalRepoPath)
    , systemHelperInterface(linglong::SystemHelperDBusServiceName, linglong::SystemHelperDBusPath,
                            []() -> QDBusConnection {
                                auto address = QString(getenv("LINGLONG_SYSTEM_HELPER_ADDRESS"));
                                if (address.length()) {
                                    return QDBusConnection::connectToPeer(address, "ll-package-manager");
                                }
                                return QDBusConnection::systemBus();
                            }())
    , q_ptr(parent)
{
}

/*
 * 从json字符串中提取软件包对应的JsonArray数据
 *
 * @param jsonString: 软件包对应的json字符串
 * @param jsonValue: 转换结果
 * @param err: 错误信息
 *
 * @return bool: true:成功 false:失败
 */
bool PackageManagerPrivate::getAppJsonArray(const QString &jsonString, QJsonValue &jsonValue, QString &err)
{
    QJsonParseError parseJsonErr;
    QJsonDocument document = QJsonDocument::fromJson(jsonString.toUtf8(), &parseJsonErr);
    if (QJsonParseError::NoError != parseJsonErr.error) {
        err = "parse server's json data failed, please check the network " + jsonString;
        return false;
    }

    QJsonObject jsonObject = document.object();
    if (jsonObject.size() == 0) {
        err = "query failed, receive data is empty";
        qCritical().noquote() << jsonString;
        return false;
    }

    if (!jsonObject.contains("code") || !jsonObject.contains("data")) {
        err = "query failed, receive data format err";
        qCritical().noquote() << jsonString;
        return false;
    }

    int code = jsonObject["code"].toInt();
    if (code != 0) {
        err = "app not found in repo";
        qCritical().noquote() << jsonString;
        return false;
    }

    jsonValue = jsonObject.value(QStringLiteral("data"));
    if (!jsonValue.isArray()) {
        err = "query failed, jsonString from server data format is not array";
        qCritical().noquote() << jsonString;
        return false;
    }
    return true;
}

/*
 * 将json字符串转化为软件包数据
 *
 * @param jsonString: 软件包对应的json字符串
 * @param appList: 转换结果
 * @param err: 错误信息
 *
 * @return bool: true:成功 false:失败
 */
bool PackageManagerPrivate::loadAppInfo(const QString &jsonString, linglong::package::MetaInfoList &appList,
                                        QString &err)
{
    QJsonValue arrayValue;
    auto ret = getAppJsonArray(jsonString, arrayValue, err);
    if (!ret) {
        qCritical().noquote() << jsonString;
        return false;
    }

    // 多个结果以json 数组形式返回
    QJsonArray arr = arrayValue.toArray();
    for (int i = 0; i < arr.size(); ++i) {
        QJsonObject dataObj = arr.at(i).toObject();
        const QString jsonString = QString(QJsonDocument(dataObj).toJson(QJsonDocument::Compact));
        // qInfo().noquote() << jsonString;
        QPointer<package::MetaInfo> appItem(util::loadJSONString<package::MetaInfo>(jsonString));
        appList.push_back(appItem);
    }
    return true;
}

/*
 * 从服务器查询指定包名/版本/架构的软件包数据
 *
 * @param pkgName: 软件包包名
 * @param pkgVer: 软件包版本号
 * @param pkgArch: 软件包对应的架构
 * @param appData: 查询结果
 * @param err: 错误信息
 *
 * @return bool: true:成功 false:失败
 */
bool PackageManagerPrivate::getAppInfoFromServer(const QString &pkgName, const QString &pkgVer, const QString &pkgArch,
                                                 QString &appData, QString &err)
{
    // FIXME: replace version latest to ""
    auto queryVer = (pkgVer == kDefaultVersion) ? "" : pkgVer;

    bool ret = G_HTTPCLIENT->queryRemoteApp(pkgName, queryVer, pkgArch, appData);
    if (!ret) {
        err = "getAppInfoFromServer err, " + appData + " ,please check the network";
        qCritical() << err;
        qCritical().noquote() << "receive from server:" << appData;
        return false;
    }

    return true;
}

/*
 * 安装应用时更新包括desktop文件在内的配置文件
 *
 * @param appId: 应用的appId
 * @param version: 应用的版本号
 * @param arch: 应用对应的架构
 */
void PackageManagerPrivate::addAppConfig(const QString &appId, const QString &version, const QString &arch)
{
    // 是否为多版本
    if (linglong::util::getAppInstalledStatus(appId, "", arch, "", "", "")) {
        linglong::package::MetaInfoList pkgList;
        // 查找当前已安装软件包的最高版本
        linglong::util::getInstalledAppInfo(appId, "", arch, "", "", "", pkgList);
        auto it = pkgList.at(0);
        linglong::util::AppVersion dstVersion(version);
        linglong::util::AppVersion curVersion(it->version);
        if (curVersion.isBigThan(dstVersion)) {
            return;
        }
        // 目标版本较已有版本高，先删除旧的desktop
        const QString installPath = kAppInstallPath + it->appId + "/" + it->version;
        // 删掉安装配置链接文件
        if (linglong::util::dirExists(installPath + "/" + arch + "/outputs/share")) {
            const QString appEntriesDirPath = installPath + "/" + arch + "/outputs/share";
            linglong::util::removeDstDirLinkFiles(appEntriesDirPath, entriesSharePath);
        } else {
            const QString appEntriesDirPath = installPath + "/" + arch + "/entries";
            linglong::util::removeDstDirLinkFiles(appEntriesDirPath, entriesSharePath);
        }
    }

    const QString savePath = kAppInstallPath + appId + "/" + version + "/" + arch;
    // 链接应用配置文件到系统配置目录
    if (linglong::util::dirExists(savePath + "/outputs/share")) {
        const QString appEntriesDirPath = savePath + "/outputs/share";
        linglong::util::linkDirFiles(appEntriesDirPath, entriesSharePath);
    } else {
        const QString appEntriesDirPath = savePath + "/entries";
        linglong::util::linkDirFiles(appEntriesDirPath, entriesSharePath);
    }
}

/*
 * 卸载应用时更新包括desktop文件在内的配置文件
 *
 * @param appId: 应用的appId
 * @param version: 应用的版本号
 * @param arch: 应用对应的架构
 */
void PackageManagerPrivate::delAppConfig(const QString &appId, const QString &version, const QString &arch)
{
    // 是否为多版本
    if (linglong::util::getAppInstalledStatus(appId, "", arch, "", "", "")) {
        linglong::package::MetaInfoList pkgList;
        // 查找当前已安装软件包的最高版本
        linglong::util::getInstalledAppInfo(appId, "", arch, "", "", "", pkgList);
        auto it = pkgList.at(0);
        linglong::util::AppVersion dstVersion(version);
        linglong::util::AppVersion curVersion(it->version);
        if (curVersion.isBigThan(dstVersion)) {
            return;
        }

        // 目标版本较已有版本高，先删除旧的desktop
        const QString installPath = kAppInstallPath + appId + "/" + version;
        // 删掉安装配置链接文件
        if (linglong::util::dirExists(installPath + "/" + arch + "/outputs/share")) {
            const QString appEntriesDirPath = installPath + "/" + arch + "/outputs/share";
            linglong::util::removeDstDirLinkFiles(appEntriesDirPath, entriesSharePath);
        } else {
            const QString appEntriesDirPath = installPath + "/" + arch + "/entries";
            linglong::util::removeDstDirLinkFiles(appEntriesDirPath, entriesSharePath);
        }

        const QString savePath = kAppInstallPath + appId + "/" + it->version + "/" + arch;
        // 链接应用配置文件到系统配置目录
        if (linglong::util::dirExists(savePath + "/outputs/share")) {
            const QString appEntriesDirPath = savePath + "/outputs/share";
            linglong::util::linkDirFiles(appEntriesDirPath, entriesSharePath);
        } else {
            const QString appEntriesDirPath = savePath + "/entries";
            linglong::util::linkDirFiles(appEntriesDirPath, entriesSharePath);
        }
        return;
    }
    // 目标版本较已有版本高，先删除旧的desktop
    const QString installPath = kAppInstallPath + appId + "/" + version;
    // 删掉安装配置链接文件
    if (linglong::util::dirExists(installPath + "/" + arch + "/outputs/share")) {
        const QString appEntriesDirPath = installPath + "/" + arch + "/outputs/share";
        linglong::util::removeDstDirLinkFiles(appEntriesDirPath, entriesSharePath);
    } else {
        const QString appEntriesDirPath = installPath + "/" + arch + "/entries";
        linglong::util::removeDstDirLinkFiles(appEntriesDirPath, entriesSharePath);
    }
}

std::tuple<util::Error, QVariantMapList> PackageManagerPrivate::query(const package::Ref &ref, bool noCache)
{
    qDebug() << "query" << ref.toSpecString() << "noCache:" << noCache;
    QString appId = ref.appId;
    bool ret = false;
    bool fromServer = false;
    QueryReply reply;
    package::MetaInfoList pkgList;
    QString arch = util::hostArch();
    QVariantMapList packages;
    QString appData = "";

    int status = STATUS_CODE(kFail);
    if (!noCache) {
        status = linglong::util::queryLocalCache(appId, appData);
    }
    // 缓存查不到从服务器查
    if (status != STATUS_CODE(kSuccess)) {
        qDebug() << "getAppInfoFromServer" << appId << arch;
        ret = getAppInfoFromServer(appId, "", arch, appData, reply.message);
        if (!ret) {
            reply.code = STATUS_CODE(kErrorPkgQueryFailed);
            qCritical() << "getAppInfoFromServer failed" << reply.message;
            return {NewError(reply.code, reply.message), packages};
        }
        fromServer = true;
    }
    QJsonValue jsonValue;
    ret = getAppJsonArray(appData, jsonValue, reply.message);
    if (!ret) {
        reply.code = STATUS_CODE(kErrorPkgQueryFailed);
        qCritical() << reply.message;
        return {NewError(reply.code, reply.message), packages};
    }
    // 若从服务器查询得到正确的数据则更新缓存
    if (fromServer) {
        linglong::util::updateCache(appId, appData);
    }

    auto jsonDoc = QJsonDocument(jsonValue.toArray());
    auto packageList = jsonDoc.toVariant().toList();
    for (auto const &package : packageList) {
        packages.push_back(package.toMap());
    }

    reply.message = "query " + appId + " success";

    return {NewError(0, reply.message), packages};
}

/*!
 * install progress from 0 to 100;
 *  stage: install package: 20 or 50 or 90, if now need install runtime, take 80 of the hole progress
 *         install_runtime: 0 or 30, if no need, is zero
 *         install_base: 0 or 40 percent, in deepin, this stage always skip
 *         post_install: fix 10 percent
 *         TODO: the first 3 stage could calc dynamic between 0~90
 * @param installParamOption
 * @param job
 * @return
 */
util::Error PackageManagerPrivate::install(const package::Ref &ref, util::Job *job)
{
    util::Error result(NoError());
    QString userName = linglong::util::getUserName();

    std::unique_ptr<package::MetaInfo> latestMetaInfo;
    std::tie(result, latestMetaInfo) = getLatestPackageMetaInfo(ref);

    if (!latestMetaInfo) {
        auto errorCode = STATUS_CODE(kPkgInstallFailed);
        auto errorMsg = QString("query package %1 failed").arg(ref.toSpecString());
        job->setProgress(100, 0, "package is already installed");
        job->setFinish(errorCode, errorMsg);
        return NewError(errorCode, errorMsg);
    }

    package::Ref targetRef = latestMetaInfo->ref();
    package::Ref runtimeRef(latestMetaInfo->runtime);
    // runtime 未锁版本号时获取最新版本runtime
    qDebug() << "install package runtime info:" << latestMetaInfo->runtime
             << ", package runtime version:" << runtimeRef.version;
    QStringList runtimeVersion = runtimeRef.version.split(".");
    std::unique_ptr<package::MetaInfo> installRuntimeInfo = nullptr;
    if (runtimeVersion.size() != 4) {
        runtimeRef.version = "";
    }
    std::tie(result, installRuntimeInfo) = getLatestPackageMetaInfo(runtimeRef);
    if (!installRuntimeInfo) {
        auto errorCode = STATUS_CODE(kPkgInstallFailed);
        auto errorMsg = QString("query package %1 failed").arg(runtimeRef.toSpecString());
        job->setProgress(100, 0, "query need install runtime info failed");
        job->setFinish(errorCode, errorMsg);
        return NewError(errorCode, errorMsg);
    }
    runtimeRef.version = installRuntimeInfo->version;
    qDebug() << "install package need install runtime version:" << installRuntimeInfo->version;

    package::Ref baseRef = getRuntimeBaseRef(runtimeRef);

    // FIXME: how to select channel
    runtimeRef.repo = "";
    runtimeRef.channel = targetRef.channel;
    baseRef.repo = "";
    baseRef.channel = targetRef.channel;

    auto needInstallRuntime = !isUserRuntimeInstalled(userName, runtimeRef);
    auto needInstallBase = !isUserBaseInstalled(userName, baseRef);

    // calc stage progress
    auto stagePackageProgressBegin = 0;
    auto stagePackageProgressEnd = 20;
    auto stageRuntimeProgressBegin = 20;
    auto stageRuntimeProgressEnd = 50;
    auto stageBaseProgressBegin = 50;
    auto stageBaseProgressEnd = 90;

    if (!needInstallBase) {
        stagePackageProgressEnd = 40;
        stageRuntimeProgressBegin = 40;
        stageRuntimeProgressEnd = 90;
        if (!needInstallRuntime) {
            stagePackageProgressEnd = 90;
        }
    } else {
        // TODO: do not care now
    }

    QVariantMap extraData;
    extraData[KeyInstallJobId] = job->id();
    extraData[KeyInstallJobRef] = targetRef.toSpecString();

    auto jobController = job->controller();
    job->connect(jobController, &util::JobController::progressChanged,
                 [job, &extraData](const QVariantMap &progressData) {
                     auto jobId = extraData[KeyInstallJobId].toString();
                     auto ref = extraData[KeyInstallJobRef].toString();

                     auto stageProgressBegin = extraData[KeyInstallJobStageProgressBegin].toInt();
                     auto stageProgressEnd = extraData[KeyInstallJobStageProgressEnd].toInt();

                     auto bytesTransferred = progressData["bytesTransferred"].toULongLong();
                     auto fetched = progressData["fetched"].toUInt();
                     auto requested = progressData["requested"].toUInt();
                     //                       auto startTime = extraData["startTime"].toULongLong();
                     auto elapsedTime = progressData["elapsedTime"].toULongLong();
                     auto status = progressData["state"].toString();
                     elapsedTime += 1;

                     auto averageRate = (elapsedTime == 0) ? 0 : bytesTransferred / elapsedTime;
                     auto rate = averageRate;
                     auto remaining = (fetched == 0) ? 0 : requested * elapsedTime / fetched;

                     auto stageProgress = fetched * 100 / requested;
                     auto progress = stageProgressBegin + stageProgress * (stageProgressEnd - stageProgressBegin) / 100;

                     if (job) {
                         job->setProgress(progress, rate, averageRate, remaining, ref);
                     } else {
                         qWarning() << "can not find job" << job;
                     }
                 });

    auto installPath = ostreeRepo.rootOfLayer(targetRef);
    qDebug() << "check package" << targetRef.toSpecString() << "is installed";
    if (!isUserAppInstalled(userName, targetRef)) {
        extraData[KeyInstallJobStageProgressBegin] = stagePackageProgressBegin;
        extraData[KeyInstallJobStageProgressEnd] = stagePackageProgressEnd;
        ostreeRepo.pull(targetRef, jobController);
        ostreeRepo.checkout(targetRef, "", installPath);
    } else {
        qDebug() << "package" << targetRef.toSpecString() << "installed";
        job->setProgress(100, 0, "package is already installed");
        job->setFinish(0, QString("install %1 success").arg(targetRef.toSpecString()));
        return NoError();
    }

    auto runtimeInstallPath = ostreeRepo.rootOfLayer(runtimeRef);
    qDebug() << "check runtime" << runtimeRef.toSpecString() << "is need install " << needInstallRuntime;
    if (needInstallRuntime) {
        // FIXME: update database
        // install runtime
        extraData[KeyInstallJobStageProgressBegin] = stageRuntimeProgressBegin;
        extraData[KeyInstallJobStageProgressEnd] = stageRuntimeProgressEnd;
        ostreeRepo.pull(runtimeRef, jobController);
        ostreeRepo.checkout(runtimeRef, "", runtimeInstallPath);
        auto ret = linglong::util::insertAppRecord(installRuntimeInfo.get(), "user", userName);
        qDebug() << "insertAppRecord" << runtimeRef.toString() << ret;
    } else {
        job->setProgress(stageRuntimeProgressEnd, "runtime " + runtimeRef.toSpecString() + " installed");
    }

    qDebug() << "check base" << baseRef.toSpecString() << "is installed";
    auto baseInstallPath = ostreeRepo.rootOfLayer(baseRef);
    if (needInstallBase) {
        // FIXME: update database
        // install base
        extraData[KeyInstallJobStageProgressBegin] = stageBaseProgressBegin;
        extraData[KeyInstallJobStageProgressEnd] = stageBaseProgressEnd;
        ostreeRepo.pull(baseRef, jobController);
        ostreeRepo.checkout(runtimeRef, "", baseInstallPath);
    } else {
        job->setProgress(stageBaseProgressEnd, "base ready");
    }

    job->setProgress(92, 10, "export files");
    // export files
    result = exportFiles(targetRef);
    if (!result.success()) {
        return WrapError(result, -2, "export files failed");
    }
    job->setProgress(95, 5, "export files...");

    // process install portal
    job->setProgress(98, 2, "process post install portal...");
    qDebug() << "call systemHelperInterface.RebuildInstallPortal" << installPath << ref.toSpecString();
    QDBusReply<void> reply = systemHelperInterface.RebuildInstallPortal(installPath, ref.toSpecString(), {});
    if (!reply.isValid()) {
        qCritical() << "process post install portal failed:" << reply.error();
    }

    // update database
    job->setProgress(99, 1, "update package database...");
    latestMetaInfo->kind = "app";
    auto ret = linglong::util::insertAppRecord(latestMetaInfo.get(), "user", userName);
    if (0 != ret) {
        job->setFinish(-2, QString("insertAppRecord failed"));
        return NewError(ret, "insertAppRecord failed");
    }
    job->setProgress(100, 0, "update database finish");
    job->setFinish(0, QString("install %1 success").arg(targetRef.toSpecString()));
    return NoError();
}

util::Error PackageManagerPrivate::update(const package::Ref &ref, util::Job *job)
{
    QString userName = linglong::util::getUserName();

    // found ref to update
    auto targetRef = ref;
    // check is install
    if (!isUserAppInstalled(userName, targetRef)) {
        return NewError(-1, "Installed");
    }

    install(targetRef, job);
    return NoError();
}

bool PackageManagerPrivate::isUserAppInstalled(const QString &userName, const package::Ref &ref)
{
    return util::getAppInstalledStatus(ref.appId, ref.version, ref.arch, ref.channel, ref.module, userName);
}

bool PackageManagerPrivate::isUserRuntimeInstalled(const QString &userName, const package::Ref &ref)
{
    return util::getAppInstalledStatus(ref.appId, ref.version, ref.arch, ref.channel, ref.module, userName);
}

bool PackageManagerPrivate::isUserBaseInstalled(const QString &userName, const package::Ref &ref)
{
    return linglong::util::isDeepinSysProduct();
}

package::Ref PackageManagerPrivate::getRuntimeBaseRef(const package::Ref &ref)
{
    return package::Ref(QString());
}

std::tuple<util::Error, std::unique_ptr<package::MetaInfo>>
PackageManagerPrivate::getLatestPackageMetaInfo(const package::Ref &ref)
{
    package::Ref latestRef("");
    std::unique_ptr<package::MetaInfo> latestMetaInfo(nullptr);
    Reply reply;
    QString appData = "";

    // 安装不查缓存
    auto ret = getAppInfoFromServer(ref.appId, ref.version, ref.arch, appData, reply.message);
    if (!ret) {
        return {NewError(STATUS_CODE(kPkgInstallFailed), reply.message), std::move(latestMetaInfo)};
    }

    qDebug() << "appData from server" << appData;

    linglong::package::MetaInfoList appList;
    ret = loadAppInfo(appData, appList, reply.message);
    if (!ret || appList.size() < 1) {
        reply.message = "app:" + ref.appId + ", version:" + ref.version + " not found in repo";
        reply.code = STATUS_CODE(kPkgInstallFailed);
        return {NewError(reply.code, reply.message), std::move(latestMetaInfo)};
    }

    // 查找最高版本，多版本场景安装应用appId要求完全匹配
    latestMetaInfo = takeLatestApp(ref.appId, appList);

    // fix: 当前服务端不支持按channel查询，返回的结果是默认channel，需要刷新channel/module
    latestMetaInfo->channel = ref.channel;
    latestMetaInfo->module = ref.module;
    qDebug() << "latestMetaInfo" << latestMetaInfo->appId << latestMetaInfo->version << latestMetaInfo->runtime;

    // 不支持模糊安装
    if (ref.appId != latestMetaInfo->appId) {
        reply.message = "app:" + ref.appId + ", version:" + ref.version + " not found in repo";
        reply.code = STATUS_CODE(kPkgInstallFailed);
        return {NewError(reply.code, reply.message), std::move(latestMetaInfo)};
    }

    return {NoError(), std::move(latestMetaInfo)};
}

util::Error PackageManagerPrivate::exportFiles(const package::Ref &ref)
{
    // 链接应用配置文件到系统配置目录
    addAppConfig(ref.appId, ref.version, ref.arch);
    updateHostShare(entriesSharePath);
    return NoError();
}

/*!
 * TODO: support user only delete owner record, it and map that user <--> version.
 * ref:
 *   - com.qq.music.deepin: remove all version
 *   - com.qq.music.deepin/1.1.3: single version and all arch & module
 *   - com.qq.music.deepin/1.1.3/x86_64: single version and arch and all module
 *   - com.qq.music.deepin/1.1.3/x86_64/devel: single version and arch and module
 * @param ref
 * @param options
 * @return
 */
util::Error PackageManagerPrivate::uninstall(uid_t uid, const package::Ref &ref, const QVariantMap &options)
{
    auto kDeleteData = "OptDeleteData";
    auto kDeleteAllVersion = "OptDeleteAllVersion";

    qDebug() << "uninstall" << ref.toSpecString() << "with" << options;

    Q_Q(PackageManager);
    util::Error error;

    bool deleteAllVersion = options[kDeleteAllVersion].toBool();
    bool deleteData = options[kDeleteData].toBool();

    auto uninstallRefs = findRefsToUninstall(ref, deleteAllVersion);
    if (uninstallRefs.isEmpty()) {
        return NewError(QDBusError::InternalError, "package not installed");
    }

    for (auto const &uninstallRef : uninstallRefs) {
        // remove package form database
        pdb.remove(util::getUserName(uid), uninstallRef);
        prunePackageFiles(uninstallRef);
    }

    updateHostShare(entriesSharePath);

    if (deleteData) {
        // delete user data with system-helper
        QDBusReply<void> reply = systemHelperInterface.DeletePackageUserData(uid, ref.toSpecString(), {});
        if (!reply.isValid()) {
            qCritical() << "process pre uninstall portal failed:" << reply.error();
        }
    }

    return NoError();
}

util::Error PackageManagerPrivate::prunePackageFiles(const package::Ref &ref)
{
    // remove ostree and layer files
    auto error = ostreeRepo.remove(ref);
    if (!error.success()) {
        // FIXME: it seem not import to failed here
        return WrapError(error, "remove ostree refs failed");
    }
    // remove entries files
    delAppConfig(ref.appId, ref.version, ref.arch);

    // process portal before uninstall
    if (ref.module == kMainModule) {
        const QString packageRootPath = ostreeRepo.rootOfLayer(ref);
        qDebug() << "call systemHelperInterface.RuinInstallPortal" << packageRootPath << ref.toSpecString();
        QDBusReply<void> reply = systemHelperInterface.RuinInstallPortal(packageRootPath, ref.toSpecString(), {});
        if (!reply.isValid()) {
            qCritical() << "process pre uninstall portal failed:" << reply.error();
        }
    }

    // TODO: do remove file
    return NoError();
}

QList<package::Ref> PackageManagerPrivate::findRefsToUninstall(const package::Ref &ref, bool deleteAllVersion)
{
    QList<package::Ref> refs;
    qDebug() << "findRefsToUninstall" << deleteAllVersion;
    // FIXME: reply.message = "uninstall " + appId + "/" + version + " is in conflict with all-version param";
    if (!pdb.isPackageInstalled("", ref)) {
        qDebug() << "can not find installed package of" << ref.toSpecString();
        // FIXME: message so notify user install
        return {};
    }

    package::MetaInfoList pkgList;
    if (deleteAllVersion) {
        util::getAllVerAppInfo(ref.appId, "", ref.appId, "", pkgList);
    } else {
        // 根据已安装文件查询已安装软件包信息
        auto version = ref.version;
        if (version == kDefaultVersion) {
            version = "";
        }
        util::getInstalledAppInfo(ref.appId, version, ref.arch, ref.channel, ref.module, "", pkgList);
    }
    qDebug() << "pkgList" << pkgList.size();

    for (auto const &metaInfo : pkgList) {
        refs << metaInfo->ref();
    }
    return refs;
}

PackageManager::PackageManager()
    : dd_ptr(new PackageManagerPrivate(this))
{
    // 检查安装数据库信息
    linglong::util::checkInstalledAppDb();
    linglong::util::updateInstalledAppInfoDb();
    // 检查应用缓存信息
    linglong::util::checkAppCache();
}

PackageManager::~PackageManager()
{
}

/**
 * @brief 修改仓库url
 * @param url 仓库url地址
 * @return Reply dbus方法调用应答 \n
 *         code:状态码 \n
 *         message:信息
 */
Reply PackageManager::ModifyRepo(const QString &url)
{
    Reply reply;
    Q_D(PackageManager);
    QUrl cfgUrl(url);
    if ((cfgUrl.scheme().toLower() != "http" && cfgUrl.scheme().toLower() != "https") || !cfgUrl.isValid()) {
        reply.message = "url format error";
        reply.code = STATUS_CODE(kUserInputParamErr);
        return reply;
    }
    auto ostreeCfg = d->kLocalRepoPath + "/repo/config";
    if (!linglong::util::fileExists(ostreeCfg)) {
        reply.message = ostreeCfg + " no exist";
        reply.code = STATUS_CODE(kErrorModifyRepoFailed);
        return reply;
    }

    QString dstUrl = "";
    if (url.endsWith("/")) {
        dstUrl = url + "repo";
    } else {
        dstUrl = url + "/repo";
    }

    // ostree config --repo=/persistent/linglong/repo set "remote \"repo\".url"
    // https://repo-dev.linglong.space/repo/ ostree config 文件中节名有""，QSettings 会自动转义，不用 QSettings
    // 直接修改 ostree config 文件
    auto ret = runner::Runner("ostree",
                              {"config", "--repo=" + d->kLocalRepoPath + "/repo", "set", "remote \"repo\".url", dstUrl},
                              1000 * 60 * 5);
    if (!ret) {
        reply.message = "modify repo url failed";
        qWarning() << reply.message;
        reply.code = STATUS_CODE(kErrorModifyRepoFailed);
        return reply;
    }

    ConfigInstance().repos[kDefaultRepo]->endpoint = url;
    ConfigInstance().save();

    reply.code = STATUS_CODE(kErrorModifyRepoSuccess);
    reply.message = "modify repo url success";
    return reply;
}

QString PackageManager::Install(const QString &ref, const QVariantMap &options)
{
    Q_D(PackageManager);

    auto job = JobManager::instance()->createJob(
        [=](util::Job *job) {
            qDebug() << "install job start";
            d->install(package::Ref(ref), job);
        },
        *this);

    return job->path();
}

QString PackageManager::Update(const QString &ref, const QVariantMap &options)
{
    Q_D(PackageManager);
    auto job = JobManager::instance()->createJob(
        [=](util::Job *job) {
            qDebug() << "install job start";
            d->update(package::Ref(ref), job);
        },
        *this);

    return job->path();
}

QString PackageManager::Uninstall(const QString &ref, const QVariantMap &options)
{
    Q_D(PackageManager);

    auto err = d->uninstall(getDBusCallerUid(*this), package::Ref(ref), options);

    if (!err.success()) {
        sendErrorReply(dbusErrorName(DBusErrorType::PackageNotInstalled), err.message());
    }

    return "";
}

QVariantMap PackageManager::Show(const QString &ref, const QVariantMap &options)
{
    sendErrorReply(QDBusError::NotSupported, "");
    return QVariantMap();
}

QVariantMapList PackageManager::List(const QVariantMap &options)
{
    QVariantMapList packages;
    QueryReply reply;

    auto ret = linglong::util::queryAllInstalledApp("", reply.result, reply.message);
    if (!ret) {
        qCritical() << "queryAllInstalledApp failed" << reply.message;
        sendErrorReply(QDBusError::InternalError, reply.message);
        return packages;
    }

    auto jsonDoc = QJsonDocument::fromJson(reply.result.toUtf8());
    auto packageList = jsonDoc.toVariant().toList();
    for (auto const &package : packageList) {
        packages.push_back(package.toMap());
    }
    return packages;
}

QVariantMapList PackageManager::Query(const QString &ref, const QVariantMap &options)
{
    Q_D(PackageManager);

    QVariantMapList packages;
    util::Error err;

    std::tie(err, packages) = d->query(package::Ref(ref), options[kOptNoCacheKey].toBool());
    if (!err.success()) {
        sendErrorReply(dbusErrorName(err.code()), err.message());
    }

    qDebug() << packages;
    return packages;
}

} // namespace package_manager
} // namespace linglong
