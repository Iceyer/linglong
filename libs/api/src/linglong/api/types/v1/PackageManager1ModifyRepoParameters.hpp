// This file is generated by tools/codegen.sh
// DO NOT EDIT IT.

// clang-format off

//  To parse this JSON data, first install
//
//      json.hpp  https://github.com/nlohmann/json
//
//  Then include this file, and then do
//
//     PackageManager1ModifyRepoParameters.hpp data = nlohmann::json::parse(jsonString);

#pragma once

#include <optional>
#include <nlohmann/json.hpp>
#include "linglong/api/types/v1/helper.hpp"

namespace linglong {
namespace api {
namespace types {
namespace v1 {
using nlohmann::json;

struct PackageManager1ModifyRepoParameters {
/**
* default repo of package manager modify repo parameters
*/
std::string defaultRepo;
/**
* repos of of package manager modify repo
*/
std::map<std::string, std::string> repos;
};
}
}
}
}

// clang-format on