/*
 * Copyright (c) 2020-2021. Uniontech Software Ltd. All rights reserved.
 *
 * Author:     KeZhihuan <kezhihuan@uniontech.com>
 *
 * Maintainer: KeZhihuan <kezhihuan@uniontech.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

// todo: 该头文件必须放在 QDBus 前，否则会报错
#include "module/repo/ostree_repohelper.h"
#include <pwd.h>

#include "package_manager.h"
#include "package_manager_p.h"

#include <QDebug>
#include <QDBusInterface>
#include <QDBusReply>
#include <QJsonArray>

#include "dbus_gen_system_helper_interface.h"
#include "module/util/config/config.h"
#include "module/util/httpclient.h"
#include "module/util/sysinfo.h"
#include "module/util/runner.h"
#include "module/dbus_ipc/dbus_system_helper_common.h"

#include "app_status.h"
#include "appinfo_cache.h"
#include "job.h"
#include "job_manager.h"
#include "service_dbus_common.h"

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

std::unique_ptr<package::AppMetaInfo> takeLatestApp(const QString &appId, package::AppMetaInfoList &appList)
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
    return std::unique_ptr<package::AppMetaInfo>(appList.takeAt(latestIndex).data());
}

PackageManagerPrivate::PackageManagerPrivate(PackageManager *parent)
    : sysLinglongInstalltions(linglong::util::getLinglongRootPath() + "/entries/share")
    , kAppInstallPath(linglong::util::getLinglongRootPath() + "/layers/")
    , kLocalRepoPath(linglong::util::getLinglongRootPath())
    , kRemoteRepoName("repo")
    , ostreeRepo(kLocalRepoPath)
    , q_ptr(parent)
{
    q_ptr->connect(&ostreeRepo, &repo::OSTreeRepo::pullProgressChanged, q_ptr, [=](const QVariantMap &extraData) {
        auto jobId = extraData[KeyInstallJobId].toString();
        auto ref = extraData[KeyInstallJobRef].toString();

        auto stageProgressBegin = extraData[KeyInstallJobStageProgressBegin].toInt();
        auto stageProgressEnd = extraData[KeyInstallJobStageProgressEnd].toInt();

        auto bytesTransferred = extraData["bytesTransferred"].toULongLong();
        auto fetched = extraData["fetched"].toUInt();
        auto requested = extraData["requested"].toUInt();
        //                       auto startTime = extraData["startTime"].toULongLong();
        auto elapsedTime = extraData["elapsedTime"].toULongLong();
        auto status = extraData["status"].toString();
        elapsedTime += 1;

        auto averageRate = (elapsedTime == 0) ? 0 : bytesTransferred / elapsedTime;
        auto rate = averageRate;
        auto remaining = (fetched == 0) ? 0 : requested * elapsedTime / fetched;

        auto stageProgress = fetched * 100 / requested;
        auto progress = stageProgressBegin + stageProgress * (stageProgressEnd - stageProgressBegin) / 100;
        auto job = JobManager::instance()->job(jobId);
        if (job) {
            job->setProgress(progress, rate, averageRate, remaining, ref);
        } else {
            qWarning() << "can not find job" << job;
        }
    });
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
bool PackageManagerPrivate::loadAppInfo(const QString &jsonString, linglong::package::AppMetaInfoList &appList,
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
        QPointer<package::AppMetaInfo> appItem(util::loadJSONString<package::AppMetaInfo>(jsonString));
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
bool PackageManagerPrivate::getAppInfofromServer(const QString &pkgName, const QString &pkgVer, const QString &pkgArch,
                                                 QString &appData, QString &err)
{
    // FIXME: replace version latest to ""
    auto queryVer = (pkgVer == kDefaultVersion) ? "" : pkgVer;

    bool ret = G_HTTPCLIENT->queryRemoteApp(pkgName, queryVer, pkgArch, appData);
    if (!ret) {
        err = "getAppInfofromServer err, " + appData + " ,please check the network";
        qCritical() << err;
        qCritical().noquote() << "receive from server:" << appData;
        return false;
    }

    return true;
}

/*
 * 将在线包数据部分签出到指定目录
 *
 * @param pkgName: 软件包包名
 * @param pkgVer: 软件包版本号
 * @param pkgArch: 软件包对应的架构
 * @param channel: 软件包对应的渠道
 * @param module: 软件包类型
 * @param dstPath: 在线包数据部分存储路径
 * @param err: 错误信息
 *
 * @return bool: true:成功 false:失败
 */
bool PackageManagerPrivate::downloadAppData(const QString &pkgName, const QString &pkgVer, const QString &pkgArch,
                                            const QString &channel, const QString &module, const QString &dstPath,
                                            QString &err)
{
    bool ret = OSTREE_REPO_HELPER->ensureRepoEnv(kLocalRepoPath, err);
    if (!ret) {
        qCritical() << err;
        return false;
    }

    // new format --> linglong/org.deepin.downloader/5.3.69/x86_64/devel
    QString matchRef = QString("%1/%2/%3/%4/%5").arg(channel).arg(pkgName).arg(pkgVer).arg(pkgArch).arg(module);
    qInfo() << "downloadAppData ref:" << matchRef;

    // ret = repo.repoPull(repoPath, qrepoList[0], pkgName, err);
    ret = OSTREE_REPO_HELPER->repoPullbyCmd(kLocalRepoPath, kRemoteRepoName, matchRef, err);
    if (!ret) {
        qCritical() << err;
        return false;
    }
    // checkout 目录
    // const QString dstPath = repoPath + "/AppData";
    ret = OSTREE_REPO_HELPER->checkOutAppData(kLocalRepoPath, kRemoteRepoName, matchRef, dstPath, err);
    if (!ret) {
        qCritical() << err;
        return false;
    }
    qInfo() << "downloadAppData success, path:" << dstPath;

    return ret;
}

Reply PackageManagerPrivate::GetDownloadStatus(const ParamOption &paramOption, int type)
{
    Reply reply;
    QString appId = paramOption.appId.trimmed();
    QString version = paramOption.version.trimmed();
    QString arch = linglong::util::hostArch();
    QString channel = paramOption.channel.trimmed();
    QString appModule = paramOption.appModule.trimmed();
    QString latestVersion = version;
    if (version.isEmpty() || type == 1) {
        if (type == 1) {
            // 判断是否已安装 bug 146229
            if (!linglong::util::getAppInstalledStatus(appId, "", arch, channel, appModule, "")) {
                reply.message = appId + ", version:" + version + ", arch:" + arch + ", channel:" + channel
                                + ", module:" + appModule + " not installed";
                reply.code = STATUS_CODE(kPkgNotInstalled);
                appState.insert(appId + "/" + version + "/" + arch, reply);
                return reply;
            }
        }
        QString appData = "";
        // 安装不查缓存
        auto ret = getAppInfofromServer(appId, "", arch, appData, reply.message);
        if (!ret) {
            reply.code = STATUS_CODE(kPkgInstallFailed);
            return reply;
        }

        linglong::package::AppMetaInfoList appList;
        ret = loadAppInfo(appData, appList, reply.message);
        if (!ret || appList.size() < 1) {
            reply.message = "app:" + appId + ", version:" + paramOption.version + " not found in repo";
            qCritical() << reply.message;
            reply.code = STATUS_CODE(kPkgInstallFailed);
            return reply;
        }

        // 查找最高版本
        linglong::package::AppMetaInfo *appInfo = getLatestApp(appId, appList);
        latestVersion = appInfo->version;
    }

    if (type > 0) {
        reply.message = appId + " is updating...";
    } else {
        reply.message = appId + " is installing...";
    }

    // 获取到完成状态则返回
    QString key = appId + "/" + version + "/" + arch;
    int dstState = type > 0 ? STATUS_CODE(kErrorPkgUpdateSuccess) : STATUS_CODE(kPkgInstallSuccess);
    if (appState.contains(key) && appState[key].code == dstState) {
        return appState[key];
    } else {
        // Fix to do get more specific param 首次安装应用的时候 安装runtime 提示不准
        QString fileName = QStringList {channel, appId, latestVersion, arch, appModule}.join("-");
        QString filePath = "/tmp/.linglong/" + fileName;
        QFile progressFile(filePath);
        if (progressFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QStringList ret = QString(progressFile.readAll()).split(QRegExp("[\r\n]"), QString::SkipEmptyParts);
            if (ret.size() > 1) {
                QStringList processList = ret.at(1).trimmed().split("\u001B8");
                reply.message = processList.at(processList.size() - 1).trimmed();
            }
            qInfo() << reply.message;
            progressFile.close();
        }
        if (type > 0) {
            reply.code = STATUS_CODE(kPkgUpdating);
        } else {
            reply.code = STATUS_CODE(kPkgInstalling);
        }
    }
    // 下载过程中有异常直接返回
    if (appState.contains(key)) {
        // 对于更新操作，下载完成了需要等待卸载完成
        if (type == 1) {
            if (appState[key].code == STATUS_CODE(kPkgInstallSuccess)) {
                appState[key].code = STATUS_CODE(kPkgUpdating);
            }
        }
        return appState[key];
    }
    return reply;
}

/*
 * 安装应用runtime
 *
 * @param runtimeId: runtime对应的appId
 * @param runtimeVer: runtime版本号
 * @param runtimeArch: runtime对应的架构
 * @param channel: 软件包对应的渠道
 * @param module: 软件包类型
 * @param err: 错误信息
 *
 * @return bool: true:成功 false:失败
 */
bool PackageManagerPrivate::installRuntime(const QString &runtimeId, const QString &runtimeVer,
                                           const QString &runtimeArch, const QString &channel, const QString &module,
                                           QString &err)
{
    linglong::package::AppMetaInfoList appList;
    QString appData = "";

    bool ret = getAppInfofromServer(runtimeId, runtimeVer, runtimeArch, appData, err);
    if (!ret) {
        return false;
    }
    ret = loadAppInfo(appData, appList, err);
    if (!ret) {
        qCritical() << err;
        return false;
    }

    // app runtime 只能匹配一个 debug模式会有两个同版本的runtime
    // if (appList.size() != 1) {
    //     err = "installRuntime app:" + runtimeId + ", version:" + runtimeVer + " not found in repo";
    //     return false;
    // }

    auto pkgInfo = appList.at(0);
    QString savePath = kAppInstallPath + runtimeId + "/" + runtimeVer + "/" + runtimeArch;
    if ("devel" == module) {
        savePath.append("/" + module);
    }
    ret = downloadAppData(runtimeId, runtimeVer, runtimeArch, channel, module, savePath, err);
    if (!ret) {
        err = "installRuntime download runtime data err";
        return false;
    }

    // 更新本地数据库文件
    QString userName = linglong::util::getUserName();
    if (noDBusMode) {
        userName = "deepin-linglong";
    }
    pkgInfo->kind = "runtime";
    // fix 当前服务端不支持按channel查询，返回的结果是默认channel，需要刷新channel/module
    pkgInfo->channel = channel;
    pkgInfo->module = module;
    linglong::util::insertAppRecord(pkgInfo.data(), "user", userName);

    return true;
}

/*
 * 检查应用runtime安装状态
 *
 * @param runtime: 应用runtime字符串
 * @param channel: 软件包对应的渠道
 * @param module: 软件包类型
 * @param err: 错误信息
 *
 * @return bool: true:安装成功或已安装返回true false:安装失败
 */
bool PackageManagerPrivate::checkAppRuntime(const QString &runtime, const QString &channel, const QString &module,
                                            QString &err)
{
    // runtime ref in repo org.deepin.Runtime/20/x86_64
    QStringList runtimeInfo = runtime.split("/");
    if (runtimeInfo.size() < 3) {
        err = "app runtime:" + runtime + " runtime format err";
        return false;
    }
    const QString runtimeId = runtimeInfo.at(0);
    const QString runtimeVer = runtimeInfo.at(1);
    const QString runtimeArch = runtimeInfo.at(2);

    // runtimeId 校验
    if (runtimeId.isEmpty()) {
        err = "app runtime:" + runtime + " runtimeId format err";
        return false;
    }

    bool ret = true;
    // 判断app依赖的runtime是否安装 runtime 不区分用户
    if (!linglong::util::getAppInstalledStatus(runtimeId, runtimeVer, runtimeArch, channel, module, "")) {
        ret = installRuntime(runtimeId, runtimeVer, runtimeArch, channel, module, err);
    }
    return ret;
}

/*
 * 针对非deepin发行版检查应用base安装状态
 *
 * @param runtime: runtime ref
 * @param channel: 软件包对应的渠道
 * @param module: 软件包类型
 * @param err: 错误信息
 *
 * @return bool: true:安装成功或已安装返回true false:安装失败
 */
bool PackageManagerPrivate::checkAppBase(const QString &runtime, const QString &channel, const QString &module,
                                         QString &err)
{
    // 通过runtime获取base ref
    QStringList runtimeList = runtime.split("/");
    if (runtimeList.size() < 3) {
        err = "app runtime:" + runtime + " runtime format err";
        return false;
    }
    const QString runtimeId = runtimeList.at(0);
    const QString runtimeVer = runtimeList.at(1);
    const QString runtimeArch = runtimeList.at(2);

    // runtimeId 校验
    if (runtimeId.isEmpty()) {
        err = "app runtime:" + runtime + " runtimeId format err";
        return false;
    }

    linglong::package::AppMetaInfoList appList;
    QString appData = "";

    bool ret = getAppInfofromServer(runtimeId, runtimeVer, runtimeArch, appData, err);
    if (!ret) {
        return false;
    }
    ret = loadAppInfo(appData, appList, err);
    if (!ret) {
        qCritical() << err;
        return false;
    }

    auto runtimeInfo = appList.at(0);
    auto baseRef = runtimeInfo->runtime;
    QStringList baseList = baseRef.split('/');
    if (baseList.size() < 3) {
        err = "app base:" + baseRef + " base format err";
        return false;
    }
    const QString baseId = baseList.at(0);
    const QString baseVer = baseList.at(1);
    const QString baseArch = baseList.at(2);

    bool retbase = true;
    // 判断app依赖的runtime是否安装 runtime 不区分用户
    if (!linglong::util::getAppInstalledStatus(baseId, baseVer, baseArch, channel, module, "")) {
        retbase = installRuntime(baseId, baseVer, baseArch, channel, module, err);
    }
    return retbase;
}

/*
 * 从给定的软件包列表中查找最新版本的软件包
 *
 * @param appId: 待匹配应用的appId
 * @param appList: 待搜索的软件包列表信息
 *
 * @return AppMetaInfo: 最新版本的软件包
 *
 */
linglong::package::AppMetaInfo *PackageManagerPrivate::getLatestApp(const QString &appId,
                                                                    const linglong::package::AppMetaInfoList &appList)
{
    linglong::package::AppMetaInfo *latestApp = appList.at(0).data();
    if (appList.size() == 1) {
        return latestApp;
    }

    QString curVersion = linglong::util::APP_MIN_VERSION;
    for (auto item : appList) {
        linglong::util::AppVersion dstVersion(curVersion);
        linglong::util::AppVersion iterVersion(item->version);
        if (appId == item->appId && iterVersion.isBigThan(dstVersion)) {
            curVersion = item->version;
            latestApp = item.data();
        }
    }
    return latestApp;
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
        linglong::package::AppMetaInfoList pkgList;
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
            linglong::util::removeDstDirLinkFiles(appEntriesDirPath, sysLinglongInstalltions);
        } else {
            const QString appEntriesDirPath = installPath + "/" + arch + "/entries";
            linglong::util::removeDstDirLinkFiles(appEntriesDirPath, sysLinglongInstalltions);
        }
    }

    const QString savePath = kAppInstallPath + appId + "/" + version + "/" + arch;
    // 链接应用配置文件到系统配置目录
    if (linglong::util::dirExists(savePath + "/outputs/share")) {
        const QString appEntriesDirPath = savePath + "/outputs/share";
        linglong::util::linkDirFiles(appEntriesDirPath, sysLinglongInstalltions);
    } else {
        const QString appEntriesDirPath = savePath + "/entries";
        linglong::util::linkDirFiles(appEntriesDirPath, sysLinglongInstalltions);
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
        linglong::package::AppMetaInfoList pkgList;
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
            linglong::util::removeDstDirLinkFiles(appEntriesDirPath, sysLinglongInstalltions);
        } else {
            const QString appEntriesDirPath = installPath + "/" + arch + "/entries";
            linglong::util::removeDstDirLinkFiles(appEntriesDirPath, sysLinglongInstalltions);
        }

        const QString savePath = kAppInstallPath + appId + "/" + it->version + "/" + arch;
        // 链接应用配置文件到系统配置目录
        if (linglong::util::dirExists(savePath + "/outputs/share")) {
            const QString appEntriesDirPath = savePath + "/outputs/share";
            linglong::util::linkDirFiles(appEntriesDirPath, sysLinglongInstalltions);
        } else {
            const QString appEntriesDirPath = savePath + "/entries";
            linglong::util::linkDirFiles(appEntriesDirPath, sysLinglongInstalltions);
        }
        return;
    }
    // 目标版本较已有版本高，先删除旧的desktop
    const QString installPath = kAppInstallPath + appId + "/" + version;
    // 删掉安装配置链接文件
    if (linglong::util::dirExists(installPath + "/" + arch + "/outputs/share")) {
        const QString appEntriesDirPath = installPath + "/" + arch + "/outputs/share";
        linglong::util::removeDstDirLinkFiles(appEntriesDirPath, sysLinglongInstalltions);
    } else {
        const QString appEntriesDirPath = installPath + "/" + arch + "/entries";
        linglong::util::removeDstDirLinkFiles(appEntriesDirPath, sysLinglongInstalltions);
    }
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
util::Error PackageManagerPrivate::install(const package::Ref &ref, Job *job)
{
    QString userName = linglong::util::getUserName();
    if (noDBusMode) {
        userName = "deepin-linglong";
    }
    util::Error result(NoError());

    std::unique_ptr<package::AppMetaInfo> latestMetaInfo;
    std::tie(result, latestMetaInfo) = getLatestPackageMetaInfo(ref);

    if (!latestMetaInfo) {
        auto errorCode = STATUS_CODE(kPkgInstallFailed);
        auto errorMsg = QString("query package %1 failed").arg(ref.toSpecString());
        job->setProgress(100, 0, "package has installed");
        job->setFinish(errorCode, errorMsg);
        return NewError(errorCode, errorMsg);
    }

    package::Ref targetRef = latestMetaInfo->ref();
    package::Ref runtimeRef(latestMetaInfo->runtime);
    package::Ref baseRef = getRuntimeBaseRef(runtimeRef);

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
    extraData[KeyInstallJobRef] = targetRef.toString();

    auto installPath = ostreeRepo.rootOfLayer(targetRef);
    qDebug() << "check package" << targetRef.toString() << "is installed";
    if (!isUserAppInstalled(userName, targetRef)) {
        extraData[KeyInstallJobStageProgressBegin] = stagePackageProgressBegin;
        extraData[KeyInstallJobStageProgressEnd] = stagePackageProgressEnd;
        ostreeRepo.pull(targetRef, extraData);
        ostreeRepo.checkout(targetRef, "", installPath);
    } else {
        qDebug() << "package" << targetRef.toString() << "installed";
        job->setProgress(100, 0, "package has installed");
        job->setFinish(0, QString("install %1 success").arg(targetRef.toLocalFullRef()));
        return NoError();
    }

    auto runtimeInstallPath = ostreeRepo.rootOfLayer(runtimeRef);
    qDebug() << "check runtime" << runtimeRef.toString() << "is installed";
    if (needInstallRuntime) {
        // install runtime
        extraData[KeyInstallJobStageProgressBegin] = stageRuntimeProgressBegin;
        extraData[KeyInstallJobStageProgressEnd] = stageRuntimeProgressEnd;
        ostreeRepo.pull(runtimeRef, extraData);
        ostreeRepo.checkout(runtimeRef, "", runtimeInstallPath);
    } else {
        job->setProgress(stageRuntimeProgressEnd, "runtime " + runtimeRef.toString() + " installed");
    }

    qDebug() << "check base" << baseRef.toString() << "is installed";
    auto baseInstallPath = ostreeRepo.rootOfLayer(baseRef);
    if (needInstallBase) {
        // install base
        extraData[KeyInstallJobStageProgressBegin] = stageBaseProgressBegin;
        extraData[KeyInstallJobStageProgressEnd] = stageBaseProgressEnd;
        ostreeRepo.pull(baseRef, extraData);
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
    OrgDeepinLinglongSystemHelperInterface systemHelperInterface(SystemHelperDBusServiceName, SystemHelperDBusPath,
                                                                 QDBusConnection::systemBus());

    job->setProgress(98, 2, "process post install portal...");
    qDebug() << "call systemHelperInterface.RebuildInstallPortal" << installPath << ref.toLocalFullRef();
    QDBusReply<void> reply = systemHelperInterface.RebuildInstallPortal(installPath, ref.toString(), {});
    if (!reply.isValid()) {
        qCritical() << "process post install portal failed:" << reply.error();
    }

    // update database
    job->setProgress(99, 1, "update package database...");
    latestMetaInfo->kind = "app";
    auto ret = linglong::util::insertAppRecord(latestMetaInfo.get(), "user", userName);
    if (0 != ret) {
        return NewError(ret, "insertAppRecord failed");
    }
    job->setProgress(100, 0, "update database finish");

    job->setFinish(0, QString("install %1 success").arg(targetRef.toLocalFullRef()));
    return NoError();
}

/*!
 * 在线安装软件包
 * @param installParamOption
 */

Reply PackageManagerPrivate::Install(const InstallParamOption &installParamOption)
{
    Reply reply;
    QString userName = linglong::util::getUserName();
    if (noDBusMode) {
        userName = "deepin-linglong";
    }

    QString appId = installParamOption.appId.trimmed();
    QString arch = installParamOption.arch.trimmed().toLower();
    QString version = installParamOption.version.trimmed();
    QString channel = installParamOption.channel.trimmed();
    QString appModule = installParamOption.appModule.trimmed();

    if (arch.isEmpty()) {
        arch = linglong::util::hostArch();
    }

    if (channel.isEmpty()) {
        channel = "linglong";
    }
    if (appModule.isEmpty()) {
        appModule = "runtime";
    }

    package::Ref ref("", channel, appId, version, arch, appModule);

    // 异常后重新安装需要清除上次状态
    appState.remove(appId + "/" + version + "/" + arch);

    if (arch != linglong::util::hostArch()) {
        reply.message = "app arch:" + arch + " not support in host";
        reply.code = STATUS_CODE(kUserInputParamErr);
        appState.insert(appId + "/" + version + "/" + arch, reply);
        return reply;
    }

    QString appData = "";
    // 安装不查缓存
    auto ret = getAppInfofromServer(appId, version, arch, appData, reply.message);
    if (!ret) {
        reply.code = STATUS_CODE(kPkgInstallFailed);
        appState.insert(appId + "/" + version + "/" + arch, reply);
        return reply;
    }

    linglong::package::AppMetaInfoList appList;
    ret = loadAppInfo(appData, appList, reply.message);
    if (!ret || appList.size() < 1) {
        reply.message = "app:" + appId + ", version:" + version + " not found in repo";
        qCritical() << reply.message;
        reply.code = STATUS_CODE(kPkgInstallFailed);
        appState.insert(appId + "/" + version + "/" + arch, reply);
        return reply;
    }

    // 查找最高版本，多版本场景安装应用appId要求完全匹配
    linglong::package::AppMetaInfo *appInfo = getLatestApp(appId, appList);
    // 不支持模糊安装
    if (appId != appInfo->appId) {
        reply.message = "app:" + appId + ", version:" + version + " not found in repo";
        qCritical() << "found latest app:" << appInfo->appId << ", " << reply.message;
        reply.code = STATUS_CODE(kPkgInstallFailed);
        appState.insert(appId + "/" + version + "/" + arch, reply);
        return reply;
    }

    // 判断对应版本的应用是否已安装
    if (isUserAppInstalled("", ref)) {
        reply.code = STATUS_CODE(kPkgAlreadyInstalled);
        reply.message = appInfo->appId + ", version: " + appInfo->version + " already installed";
        qCritical() << reply.message;
        appState.insert(appId + "/" + version + "/" + arch, reply);
        return reply;
    }

    if (linglong::util::getAppInstalledStatus(appInfo->appId, appInfo->version, "", channel, appModule, "")) {
        reply.code = STATUS_CODE(kPkgAlreadyInstalled);
        reply.message = appInfo->appId + ", version: " + appInfo->version + " already installed";
        qCritical() << reply.message;
        appState.insert(appId + "/" + version + "/" + arch, reply);
        return reply;
    }

    // 检查软件包依赖的runtime安装状态
    ret = checkAppRuntime(appInfo->runtime, channel, appModule, reply.message);
    if (!ret) {
        qCritical() << reply.message;
        reply.code = STATUS_CODE(kInstallRuntimeFailed);
        appState.insert(appId + "/" + version + "/" + arch, reply);
        return reply;
    }

    // 检查软件包依赖的base安装状态
    if (!linglong::util::isDeepinSysProduct()) {
        ret = checkAppBase(appInfo->runtime, channel, appModule, reply.message);
        if (!ret) {
            qCritical() << reply.message;
            reply.code = STATUS_CODE(kInstallBaseFailed);
            appState.insert(appId + "/" + version + "/" + arch, reply);
            return reply;
        }
    }

    // 下载在线包数据到目标目录
    QString savePath = kAppInstallPath + appInfo->appId + "/" + appInfo->version + "/" + appInfo->arch;
    if ("devel" == appModule) {
        savePath.append("/" + appModule);
    }
    ret = downloadAppData(appInfo->appId, appInfo->version, appInfo->arch, channel, appModule, savePath, reply.message);
    if (!ret) {
        qCritical() << "downloadAppData app:" << appInfo->appId << ", version:" << appInfo->version << " error";
        reply.code = STATUS_CODE(kLoadPkgDataFailed);
        appState.insert(appId + "/" + version + "/" + arch, reply);
        return reply;
    }

    // 删除下载进度的重定向文件
    QString fileName = QStringList {channel, appInfo->appId, appInfo->version, appInfo->arch, appModule}.join("-");
    QString filePath = "/tmp/.linglong/" + fileName;
    QFile(filePath).remove();

    // 链接应用配置文件到系统配置目录
    addAppConfig(appInfo->appId, appInfo->version, appInfo->arch);

    // 更新desktop database
    auto retRunner = linglong::runner::Runner("update-desktop-database", {sysLinglongInstalltions + "/applications/"},
                                              1000 * 60 * 1);
    if (!retRunner) {
        qWarning() << "warning: update desktop database of " + sysLinglongInstalltions + "/applications/ failed!";
    }

    // 更新mime type database
    if (linglong::util::dirExists(sysLinglongInstalltions + "/mime/packages")) {
        auto retUpdateMime =
            linglong::runner::Runner("update-mime-database", {sysLinglongInstalltions + "/mime/"}, 1000 * 60 * 1);
        if (!retUpdateMime) {
            qWarning() << "warning: update mime type database of " + sysLinglongInstalltions + "/mime/ failed!";
        }
    }

    // 更新 glib-2.0/schemas
    if (linglong::util::dirExists(sysLinglongInstalltions + "/glib-2.0/schemas")) {
        auto retUpdateSchemas = linglong::runner::Runner(
            "glib-compile-schemas", {sysLinglongInstalltions + "/glib-2.0/schemas"}, 1000 * 60 * 1);
        if (!retUpdateSchemas) {
            qWarning() << "warning: update schemas of " + sysLinglongInstalltions + "/glib-2.0/schemas failed!";
        }
    }

    // 更新本地数据库文件
    appInfo->kind = "app";
    // fix 当前服务端不支持按channel查询，返回的结果是默认channel，需要刷新channel/module
    appInfo->channel = channel;
    appInfo->module = appModule;

    linglong::util::insertAppRecord(appInfo, "user", userName);

    // process portal after install
    {
        OrgDeepinLinglongSystemHelperInterface systemHelperInterface(SystemHelperDBusServiceName, SystemHelperDBusPath,
                                                                     QDBusConnection::systemBus());
        auto installPath = savePath;
        qDebug() << "call systemHelperInterface.RebuildInstallPortal" << installPath, ref.toLocalFullRef();
        QDBusReply<void> reply = systemHelperInterface.RebuildInstallPortal(installPath, ref.toString(), {});
        if (!reply.isValid()) {
            qCritical() << "process post install portal failed:" << reply.error();
        }
    }

    reply.code = STATUS_CODE(kPkgInstallSuccess);
    reply.message = "install " + appInfo->appId + ", version:" + appInfo->version + " success";
    qInfo() << reply.message;
    appState.insert(appId + "/" + version + "/" + arch, reply);
    return reply;
}

/*
 * 查询软件包
 *
 * @param paramOption 查询命令参数
 *
 * @return QueryReply dbus方法调用应答
 */
QueryReply PackageManagerPrivate::Query(const QueryParamOption &paramOption)
{
    QueryReply reply;
    QString appId = paramOption.appId.trimmed();
    bool ret = false;
    if ("installed" == appId) {
        ret = linglong::util::queryAllInstalledApp("", reply.result, reply.message);
        if (ret) {
            reply.code = STATUS_CODE(kErrorPkgQuerySuccess);
            reply.message = "query " + appId + " success";
        } else {
            reply.code = STATUS_CODE(kErrorPkgQueryFailed);
        }
        return reply;
    }

    linglong::package::AppMetaInfoList pkgList;
    QString arch = linglong::util::hostArch();

    QString appData = "";
    int status = STATUS_CODE(kFail);
    if (!paramOption.force) {
        status = linglong::util::queryLocalCache(appId, appData);
    }

    bool fromServer = false;
    // 缓存查不到从服务器查
    if (status != STATUS_CODE(kSuccess)) {
        ret = getAppInfofromServer(appId, "", arch, appData, reply.message);
        if (!ret) {
            reply.code = STATUS_CODE(kErrorPkgQueryFailed);
            qCritical() << reply.message;
            return reply;
        }
        fromServer = true;
    }

    QJsonValue jsonValue;
    ret = getAppJsonArray(appData, jsonValue, reply.message);
    if (!ret) {
        reply.code = STATUS_CODE(kErrorPkgQueryFailed);
        qCritical() << reply.message;
        return reply;
    }
    // 若从服务器查询得到正确的数据则更新缓存
    if (fromServer) {
        linglong::util::updateCache(appId, appData);
    }

    QJsonDocument document = QJsonDocument(jsonValue.toArray());
    reply.code = STATUS_CODE(kErrorPkgQuerySuccess);
    reply.message = "query " + appId + " success";
    reply.result = QString(document.toJson());
    return reply;
}

Reply PackageManagerPrivate::Uninstall(const UninstallParamOption &paramOption)
{
    Q_Q(PackageManager);
    Reply reply;

    QString appId = paramOption.appId.trimmed();
    QString version = paramOption.version.trimmed();
    QString arch = paramOption.arch.trimmed().toLower();

    if (arch.isEmpty()) {
        arch = linglong::util::hostArch();
    }

    if (!version.isEmpty() && paramOption.delAllVersion) {
        reply.message = "uninstall " + appId + "/" + version + " is in conflict with all-version param";
        reply.code = STATUS_CODE(kUserInputParamErr);
        qCritical() << reply.message;
        return reply;
    }

    QString channel = paramOption.channel.trimmed();
    QString appModule = paramOption.appModule.trimmed();
    if (channel.isEmpty()) {
        channel = "linglong";
    }
    if (appModule.isEmpty()) {
        appModule = "runtime";
    }

    package::Ref ref("", channel, appId, version, arch, appModule);

    // 判断是否已安装 不校验用户名
    QString userName = linglong::util::getUserName();
    if (!linglong::util::getAppInstalledStatus(appId, version, arch, channel, appModule, "")) {
        reply.message = appId + ", version:" + version + ", arch:" + arch + ", channel:" + channel
                        + ", module:" + appModule + " not installed";
        reply.code = STATUS_CODE(kPkgNotInstalled);
        qCritical() << reply.message;
        return reply;
    }

    linglong::package::AppMetaInfoList pkgList;
    if (paramOption.delAllVersion) {
        linglong::util::getAllVerAppInfo(appId, "", arch, "", pkgList);
    } else {
        // 根据已安装文件查询已安装软件包信息
        linglong::util::getInstalledAppInfo(appId, version, arch, channel, appModule, "", pkgList);
    }

    QStringList delVersionList;
    for (auto it : pkgList) {
        bool isRoot = (getgid() == 0) ? true : false;
        qInfo() << "install app user:" << it->user << ", current user:" << userName
                << ", has root permission:" << isRoot;

        // 非root用户卸载不属于该用户安装的应用
        if (userName != it->user && !isRoot) {
            reply.code = STATUS_CODE(kPkgUninstallFailed);
            reply.message = appId + " uninstall permission deny";
            qCritical() << reply.message;
            return reply;
        }

        if (paramOption.delAllVersion && (it->channel != channel || it->module != appModule)) {
            continue;
        }

        QString err = "";
        // 更新本地repo仓库
        bool ret = OSTREE_REPO_HELPER->ensureRepoEnv(kLocalRepoPath, err);
        if (!ret) {
            qCritical() << err;
            reply.code = STATUS_CODE(kPkgUninstallFailed);
            reply.message = "uninstall local repo not exist";
            return reply;
        }
        // 应从安装数据库获取应用所属仓库信息 to do fix
        QVector<QString> qrepoList;
        ret = OSTREE_REPO_HELPER->getRemoteRepoList(kLocalRepoPath, qrepoList, err);
        if (!ret) {
            qCritical() << err;
            reply.code = STATUS_CODE(kPkgUninstallFailed);
            reply.message = "uninstall remote repo not exist";
            return reply;
        }

        // new ref format --> channel/org.deepin.calculator/1.2.2/x86_64/module
        QString matchRef =
            QString("%1/%2/%3/%4/%5").arg(channel).arg(it->appId).arg(it->version).arg(arch).arg(appModule);

        qInfo() << "Uninstall app ref:" << matchRef;
        ret = OSTREE_REPO_HELPER->repoDeleteDatabyRef(kLocalRepoPath, qrepoList[0], matchRef, err);
        if (!ret) {
            qCritical() << err;
            reply.code = STATUS_CODE(kPkgUninstallFailed);
            reply.message = "uninstall " + appId + ", version:" + it->version + " failed";
            return reply;
        }

        // A 用户 sudo 卸载 B 用户安装的软件
        if (isRoot) {
            userName = "";
        }
        // 更新安装数据库
        linglong::util::deleteAppRecord(appId, it->version, arch, channel, appModule, userName);

        delAppConfig(appId, it->version, arch);
        // 更新desktop database
        auto retRunner = linglong::runner::Runner("update-desktop-database",
                                                  {sysLinglongInstalltions + "/applications/"}, 1000 * 60 * 1);
        if (!retRunner) {
            qWarning() << "warning: update desktop database of " + sysLinglongInstalltions + "/applications/ failed!";
        }

        // 更新mime type database
        if (linglong::util::dirExists(sysLinglongInstalltions + "/mime/packages")) {
            auto retUpdateMime =
                linglong::runner::Runner("update-mime-database", {sysLinglongInstalltions + "/mime/"}, 1000 * 60 * 1);
            if (!retUpdateMime) {
                qWarning() << "warning: update mime type database of " + sysLinglongInstalltions + "/mime/ failed!";
            }
        }

        // 更新 glib-2.0/schemas
        if (linglong::util::dirExists(sysLinglongInstalltions + "/glib-2.0/schemas")) {
            auto retUpdateSchemas = linglong::runner::Runner(
                "glib-compile-schemas", {sysLinglongInstalltions + "/glib-2.0/schemas"}, 1000 * 60 * 1);
            if (!retUpdateSchemas) {
                qWarning() << "warning: update schemas of " + sysLinglongInstalltions + "/glib-2.0/schemas failed!";
            }
        }

        // 删除应用对应的安装目录
        const QString installPath = kAppInstallPath + it->appId + "/" + it->version;

        // process portal before uninstall
        {
            QVariantMap variantMap;
            if (paramOption.delAppData) {
                if (!noDBusMode) {
                    QDBusReply<uint> dbusReply = q->connection().interface()->serviceUid(q->message().service());
                    QString caller = "";
                    if (dbusReply.isValid()) {
                        caller = getUserName(dbusReply.value());
                    }
                    qDebug() << "Uninstall app call user:" << caller << dbusReply.error();
                    if (!caller.isEmpty()) {
                        QString appDataPath = QString("/home/%1/.linglong/%2").arg(caller).arg(it->appId);
                        variantMap.insert(linglong::util::kKeyDelData, appDataPath);
                    }
                }
            }
            auto packageRootPath = installPath + "/" + arch;
            OrgDeepinLinglongSystemHelperInterface systemHelperInterface(
                SystemHelperDBusServiceName, SystemHelperDBusPath, QDBusConnection::systemBus());
            qDebug() << "call systemHelperInterface.RuinInstallPortal" << packageRootPath << ref.toLocalFullRef()
                     << paramOption.delAppData;
            QDBusReply<void> reply =
                systemHelperInterface.RuinInstallPortal(packageRootPath, ref.toString(), variantMap);
            if (!reply.isValid()) {
                qCritical() << "process pre uninstall portal failed:" << reply.error();
            }
        }

        // 删除release版本时需要判断是否存在debug版本
        if ("devel" != appModule) {
            if (linglong::util::getAppInstalledStatus(appId, it->version, arch, channel, "devel", "")) {
                // 删除安装目录下除devel外的所有文件
                QDir dir(installPath + "/" + arch);
                dir.setFilter(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
                QFileInfoList fileList = dir.entryInfoList();
                foreach (QFileInfo file, fileList) {
                    if (file.isFile()) {
                        file.dir().remove(file.fileName());
                    } else {
                        if ("devel" != file.fileName()) {
                            linglong::util::removeDir(file.absoluteFilePath());
                        }
                    }
                }
            } else {
                linglong::util::removeDir(installPath);
            }
        } else {
            // 卸载debug版本
            linglong::util::removeDir(installPath + "/" + arch + "/devel");
        }

        QDir archDir(installPath + "/" + arch);
        archDir.setFilter(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
        if (archDir.entryInfoList().size() <= 0) {
            linglong::util::removeDir(installPath);
        }

        QDir dir(kAppInstallPath + it->appId);
        dir.setFilter(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
        if (dir.entryInfoList().size() <= 0) {
            linglong::util::removeDir(kAppInstallPath + it->appId);
        }
        qInfo() << "Uninstall del dir:" << installPath;
        reply.message = "uninstall " + appId + ", version:" + it->version + " success";
        delVersionList.append(it->version);
    }
    reply.code = STATUS_CODE(kPkgUninstallSuccess);
    if (paramOption.delAllVersion && pkgList.size() > 1) {
        reply.message = "uninstall " + appId + " " + delVersionList.join(",") + " success";
    }
    return reply;
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

Reply PackageManagerPrivate::Update(const ParamOption &paramOption)
{
    Reply reply;

    QString appId = paramOption.appId.trimmed();
    QString arch = paramOption.arch.trimmed().toLower();
    QString version = paramOption.version.trimmed();
    if (arch.isEmpty()) {
        arch = linglong::util::hostArch();
    }

    QString channel = paramOption.channel.trimmed();
    QString appModule = paramOption.appModule.trimmed();
    if (channel.isEmpty()) {
        channel = "linglong";
    }
    if (appModule.isEmpty()) {
        appModule = "runtime";
    }

    // 异常后重新安装需要清除上次状态
    appState.remove(appId + "/" + version + "/" + arch);

    // 判断是否已安装
    if (!linglong::util::getAppInstalledStatus(appId, version, arch, channel, appModule, "")) {
        reply.message = appId + ", version:" + version + ", arch:" + arch + ", channel:" + channel
                        + ", module:" + appModule + " not installed";
        qCritical() << reply.message;
        reply.code = STATUS_CODE(kPkgNotInstalled);

        appState.insert(appId + "/" + version + "/" + arch, reply);
        return reply;
    }

    // 检查是否存在版本更新
    linglong::package::AppMetaInfoList pkgList;
    // 根据已安装文件查询已经安装软件包信息
    linglong::util::getInstalledAppInfo(appId, version, arch, channel, appModule, "", pkgList);
    if (pkgList.size() != 1) {
        reply.message = "query local app:" + appId + " info err";
        qCritical() << reply.message;
        reply.code = STATUS_CODE(kErrorPkgUpdateFailed);

        appState.insert(appId + "/" + version + "/" + arch, reply);
        return reply;
    }

    auto installedApp = pkgList.at(0);
    QString currentVersion = installedApp->version;
    QString appData = QString();
    auto ret = getAppInfofromServer(appId, "", arch, appData, reply.message);
    if (!ret) {
        reply.message = "query server app:" + appId + " info err";
        qCritical() << reply.message;
        reply.code = STATUS_CODE(kErrorPkgUpdateFailed);

        appState.insert(appId + "/" + version + "/" + arch, reply);
        return reply;
    }

    linglong::package::AppMetaInfoList serverPkgList;
    ret = loadAppInfo(appData, serverPkgList, reply.message);
    if (!ret) {
        reply.message = "load app:" + appId + " info err";
        qCritical() << reply.message;
        reply.code = STATUS_CODE(kErrorPkgUpdateFailed);

        appState.insert(appId + "/" + version + "/" + arch, reply);
        return reply;
    }

    auto serverApp = getLatestApp(appId, serverPkgList);
    if (currentVersion == serverApp->version) {
        reply.message = "app:" + appId + ", latest version:" + currentVersion + " already installed";
        qCritical() << reply.message;
        // bug 149881
        reply.code = STATUS_CODE(kErrorPkgUpdateSuccess);
        appState.insert(appId + "/" + version + "/" + arch, reply);
        return reply;
    }

    // 判断是否已安装最新版本软件
    if (linglong::util::getAppInstalledStatus(appId, serverApp->version, arch, channel, appModule, "")) {
        reply.message = appId + ", latest version:" + serverApp->version + " already installed";
        qCritical() << reply.message;
        // bug 149881
        reply.code = STATUS_CODE(kErrorPkgUpdateSuccess);

        appState.insert(appId + "/" + version + "/" + arch, reply);
        return reply;
    }

    InstallParamOption installParamOption;
    installParamOption.appId = appId;
    installParamOption.version = serverApp->version;
    installParamOption.arch = arch;
    installParamOption.channel = channel;
    installParamOption.appModule = appModule;
    reply = Install({installParamOption});
    if (reply.code != STATUS_CODE(kPkgInstallSuccess)) {
        reply.message = "download app:" + appId + ", version:" + installParamOption.version + " err";
        qCritical() << reply.message;
        reply.code = STATUS_CODE(kErrorPkgUpdateFailed);

        appState.insert(appId + "/" + version + "/" + arch, reply);
        return reply;
    }

    UninstallParamOption uninstallParamOption;
    uninstallParamOption.appId = appId;
    uninstallParamOption.version = currentVersion;
    uninstallParamOption.channel = channel;
    uninstallParamOption.appModule = appModule;
    reply = Uninstall(uninstallParamOption);
    if (reply.code != STATUS_CODE(kPkgUninstallSuccess)) {
        reply.message = "uninstall app:" + appId + ", version:" + currentVersion + " err";
        qCritical() << reply.message;
        reply.code = STATUS_CODE(kErrorPkgUpdateFailed);

        appState.insert(appId + "/" + version + "/" + arch, reply);
        return reply;
    }

    reply.code = STATUS_CODE(kErrorPkgUpdateSuccess);
    reply.message = "update " + appId + " success, version:" + currentVersion + " --> " + serverApp->version;
    appState.insert(appId + "/" + version + "/" + arch, reply);
    return reply;
}

package::Ref PackageManagerPrivate::getRuntimeBaseRef(const package::Ref &ref)
{
    return package::Ref(QString());
}

std::tuple<util::Error, std::unique_ptr<package::AppMetaInfo>>
PackageManagerPrivate::getLatestPackageMetaInfo(const package::Ref &ref)
{
    package::Ref latestRef("");
    std::unique_ptr<package::AppMetaInfo> latestMetaInfo(nullptr);
    Reply reply;
    QString appData = "";

    // 安装不查缓存
    auto ret = getAppInfofromServer(ref.appId, ref.version, ref.arch, appData, reply.message);
    if (!ret) {
        return {NewError(STATUS_CODE(kPkgInstallFailed), ""), std::move(latestMetaInfo)};
    }

    qDebug() << "appData from server" << appData;

    linglong::package::AppMetaInfoList appList;
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

    // 更新desktop database
    auto retRunner = linglong::runner::Runner("update-desktop-database", {sysLinglongInstalltions + "/applications/"},
                                              1000 * 60 * 1);
    if (!retRunner) {
        qWarning() << "warning: update desktop database of " + sysLinglongInstalltions + "/applications/ failed!";
    }

    // 更新mime type database
    if (linglong::util::dirExists(sysLinglongInstalltions + "/mime/packages")) {
        auto retUpdateMime =
            linglong::runner::Runner("update-mime-database", {sysLinglongInstalltions + "/mime/"}, 1000 * 60 * 1);
        if (!retUpdateMime) {
            qWarning() << "warning: update mime type database of " + sysLinglongInstalltions + "/mime/ failed!";
        }
    }

    // 更新 glib-2.0/schemas
    if (linglong::util::dirExists(sysLinglongInstalltions + "/glib-2.0/schemas")) {
        auto retUpdateSchemas = linglong::runner::Runner(
            "glib-compile-schemas", {sysLinglongInstalltions + "/glib-2.0/schemas"}, 1000 * 60 * 1);
        if (!retUpdateSchemas) {
            qWarning() << "warning: update schemas of " + sysLinglongInstalltions + "/glib-2.0/schemas failed!";
        }
    }

    return NoError();
}

QString PackageManagerPrivate::getUserName(uid_t uid)
{
    struct passwd *user;
    user = getpwuid(uid);
    if (user) {
        return QString::fromLocal8Bit(user->pw_name);
    }
    return "";
}

PackageManager::PackageManager()
    : pool(new QThreadPool)
    , dd_ptr(new PackageManagerPrivate(this))
{
    // 检查安装数据库信息
    linglong::util::checkInstalledAppDb();
    linglong::util::updateInstalledAppInfoDb();
    pool->setMaxThreadCount(POOL_MAX_THREAD);
    // 检查应用缓存信息
    linglong::util::checkAppCache();
}

PackageManager::~PackageManager()
{
}

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

    // ostree config --repo=/persistent/linglong/repo set "remote \"repo\".url" https://repo-dev.linglong.space/repo/
    // ostree config 文件中节名有""，QSettings 会自动转义，不用 QSettings 直接修改 ostree config 文件
    auto ret = linglong::runner::Runner(
        "ostree", {"config", "--repo=" + d->kLocalRepoPath + "/repo", "set", "remote \"repo\".url", dstUrl},
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

/**
 * @brief 查询软件包下载安装状态
 *
 * @param paramOption 查询参数
 * @param type 查询类型 0:查询应用安装进度 1:查询应用更新进度
 *
 * @return Reply dbus方法调用应答 \n
 *          code:状态码 \n
 *          message:信息
 */
Reply PackageManager::GetDownloadStatus(const ParamOption &paramOption, int type)
{
    Q_D(PackageManager);
    Reply reply;
    QString appId = paramOption.appId.trimmed();
    if (appId.isEmpty()) {
        qCritical() << "package name err";
        reply.code = STATUS_CODE(kUserInputParamErr);
        reply.message = "package name err";
        return reply;
    }

    QString arch = paramOption.arch.trimmed().toLower();
    if (!arch.isEmpty() && arch != linglong::util::hostArch()) {
        reply.message = "app arch:" + arch + " not support in host";
        reply.code = STATUS_CODE(kUserInputParamErr);
        return reply;
    }
    return d->GetDownloadStatus(paramOption, type);
}

QString PackageManager::Install(const QString &ref, const QVariantMap &options)
{
    Q_D(PackageManager);

    auto job = JobManager::instance()->createJob([=](Job *job) {
        qDebug() << "install job start";
        d->install(package::Ref(ref), job);
    });

    return job->path();
}

Reply PackageManager::Uninstall(const UninstallParamOption &paramOption)
{
    Q_D(PackageManager);
    Reply reply;
    QString appId = paramOption.appId.trimmed();

    if (appId.isEmpty()) {
        reply.message = "appId input err";
        reply.code = STATUS_CODE(kUserInputParamErr);
        return reply;
    }

    if ("flatpak" == paramOption.repoPoint) {
        return PACKAGEMANAGER_FLATPAK_IMPL->Uninstall(paramOption);
    }
    // QFuture<void> future = QtConcurrent::run(pool.data(), [=]() { d->Uninstall(paramOption); });
    return d->Uninstall(paramOption);
}

Reply PackageManager::Update(const ParamOption &paramOption)
{
    Q_D(PackageManager);

    Reply reply;
    QString appId = paramOption.appId.trimmed();
    if (appId.isEmpty()) {
        reply.message = "appId input err";
        reply.code = STATUS_CODE(kUserInputParamErr);
        return reply;
    }

    QFuture<void> future = QtConcurrent::run(pool.data(), [=]() { d->Update(paramOption); });
    reply.code = STATUS_CODE(kPkgUpdating);
    reply.message = appId + " is updating";
    return reply;
}

QueryReply PackageManager::Query(const QueryParamOption &paramOption)
{
    Q_D(PackageManager);
    if ("flatpak" == paramOption.repoPoint) {
        return PACKAGEMANAGER_FLATPAK_IMPL->Query(paramOption);
    }
    QueryReply reply;
    QString appId = paramOption.appId.trimmed();
    if (appId.isEmpty()) {
        reply.code = STATUS_CODE(kUserInputParamErr);
        reply.message = "appId input err";
        qCritical() << reply.message;
        return reply;
    }
    return d->Query(paramOption);
}

void PackageManager::setNoDBusMode(bool enable)
{
    Q_D(PackageManager);
    d->noDBusMode = true;
    qInfo() << "setNoDBusMode enable:" << enable;
}

} // namespace package_manager
} // namespace linglong
