/*
 * SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "linglong/cli/cli_printer.h"

#include "linglong/api/types/v1/Generators.hpp"

#include <QJsonArray>

#include <iomanip>
#include <iostream>

namespace linglong::cli {

std::wstring subwstr(std::wstring wstr, int width)
{
    if (wcswidth(wstr.c_str(), -1) <= width)
        return wstr;
    int halfsize = wstr.size() / 2;
    std::wstring halfwstr = wstr.substr(0, halfsize);
    int halfwidth = wcswidth(halfwstr.c_str(), -1);
    if (halfwidth >= width)
        return subwstr(halfwstr, width);
    return halfwstr + subwstr(wstr.substr(halfsize, halfsize), width - halfwidth);
}

void CLIPrinter::printErr(const utils::error::Error &err)
{
    std::cout << "Error: CODE=" << err.code() << std::endl
              << err.message().toStdString() << std::endl;
}

void CLIPrinter::printPackages(const std::vector<api::types::v1::PackageInfoV2> &list)
{
    std::cout << "\033[38;5;214m" << std::left << std::setw(33) << qUtf8Printable("id")
              << std::setw(33) << qUtf8Printable("name") << std::setw(16)
              << qUtf8Printable("version") << std::setw(12) << qUtf8Printable("arch")
              << std::setw(16) << qUtf8Printable("channel") << std::setw(12)
              << qUtf8Printable("module") << qUtf8Printable("description") << "\033[0m"
              << std::endl;

    for (const auto &info : list) {
        auto simpleDescription = QString::fromStdString(info.description.value_or("")).simplified();
        auto simpleDescriptionWStr = simpleDescription.toStdWString();
        auto simpleDescriptionWcswidth = wcswidth(simpleDescriptionWStr.c_str(), -1);
        if (simpleDescriptionWcswidth > 56) {
            simpleDescriptionWStr = subwstr(simpleDescriptionWStr, 53) + L"...";
            simpleDescription = QString::fromStdWString(simpleDescriptionWStr);
        }

        auto id = QString::fromStdString(info.id).simplified();
        if (id.size() > 32) {
            id.push_back(" ");
        }

        auto name = QString::fromStdString(info.name).simplified();
        auto nameWStr = name.toStdWString();
        auto nameWcswidth = wcswidth(nameWStr.c_str(), -1);
        if (nameWcswidth > 32) {
            nameWStr = subwstr(nameWStr, 29) + L"...";
            nameWcswidth = wcswidth(nameWStr.c_str(), -1);
            name = QString::fromStdWString(nameWStr);
        }
        auto nameStr = name.toStdString();
        auto nameOffset = nameStr.size() - nameWcswidth;
        std::cout << std::setw(33) << id.toStdString() << std::setw(33 + nameOffset) << nameStr
                  << std::setw(16) << info.version << std::setw(12) << info.arch[0] << std::setw(16)
                  << info.channel << std::setw(12) << info.packageInfoV2Module
                  << simpleDescription.toStdString() << std::endl;
    }
}

void CLIPrinter::printContainers(const std::vector<api::types::v1::CliContainer> &list)
{
    const std::size_t padding{ 5 };
    const std::string packageSection = qUtf8Printable("App");
    const std::string idSection = qUtf8Printable("ContainerID");
    const std::string pidSection = qUtf8Printable("Pid");

    std::size_t packageLen = 0;
    std::size_t idLen = 0;
    std::size_t pidLen = 0;

    std::for_each(list.cbegin(),
                  list.cend(),
                  [&packageLen, &idLen, &pidLen](const api::types::v1::CliContainer &con) {
                      packageLen = std::max(packageLen, con.package.size());
                      idLen = std::max(idLen, con.id.size());
                      pidLen = std::max(pidLen, std::to_string(con.pid).size());
                  });

    packageLen = std::max(packageSection.size(), packageLen);
    idLen = std::max(idSection.size(), idLen);
    pidLen = std::max(pidSection.size(), pidLen);

    packageLen += padding;
    idLen += padding;
    pidLen += padding;

    std::cout << "\033[38;5;214m" << std::left << std::setw(static_cast<int>(packageLen))
              << packageSection << std::setw(static_cast<int>(idLen)) << idSection
              << std::setw(static_cast<int>(pidLen)) << pidSection << "\033[0m" << std::endl;

    for (auto const &container : list) {
        std::cout << std::setw(static_cast<int>(packageLen)) << container.package
                  << std::setw(static_cast<int>(idLen)) << container.id
                  << std::setw(static_cast<int>(pidLen)) << container.pid << std::endl;
    }
}

void CLIPrinter::printReply(const api::types::v1::CommonResult &reply)
{
    std::cout << "code: " << reply.code << std::endl;
    std::cout << "message:" << std::endl << reply.message << std::endl;
}

void CLIPrinter::printRepoConfig(const api::types::v1::RepoConfig &repoInfo)
{
    std::cout << "Default: " << repoInfo.defaultRepo << std::endl;
    std::cout << std::left << std::setw(11) << "Name";
    std::cout << "Url" << std::endl;
    for (const auto &repo : repoInfo.repos) {
        std::cout << std::left << std::setw(10) << repo.first << " " << repo.second << std::endl;
    }
}

void CLIPrinter::printLayerInfo(const api::types::v1::LayerInfo &info)
{
    std::cout << info.info.dump(4) << std::endl;
}

void CLIPrinter::printContent(const QStringList &filePaths)
{
    for (const auto &path : filePaths) {
        std::cout << path.toStdString() << std::endl;
    }
}

void CLIPrinter::printTaskMessage(const api::types::v1::PackageTaskMessage &message)
{
    auto &stdout = std::cout;
    stdout << "\r\33[K" << "\033[?25l" << message.percentage << "% ";

    if (message.result) {
        stdout << message.result->message << ":" << message.result->code;
    } else {
        stdout << message.message;
    }

    stdout << "\033[?25h\n";
    stdout.flush();
}

void CLIPrinter::printPackage(const api::types::v1::PackageInfoV2 &info)
{
    std::cout << nlohmann::json(info).dump(4) << std::endl;
}

void CLIPrinter::printUpgradeList(std::vector<api::types::v1::UpgradeListResult> &list)
{
    std::cout << "\033[38;5;214m" << std::left << std::setw(33) << qUtf8Printable("id")
              << std::setw(16) << qUtf8Printable("installed") << std::setw(16)
              << qUtf8Printable("new") << "\033[0m" << std::endl;

    for (const auto &result : list) {
        auto id = QString::fromStdString(result.id).simplified();
        if (id.size() > 32) {
            id.push_back(" ");
        }

        std::cout << std::setw(33) << id.toStdString() << std::setw(16) << result.oldVersion
                  << std::setw(16) << result.newVersion << std::endl;
    }
}

} // namespace linglong::cli