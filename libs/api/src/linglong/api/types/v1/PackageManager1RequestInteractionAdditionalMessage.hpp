// This file is generated by tools/codegen.sh
// DO NOT EDIT IT.

// clang-format off

//  To parse this JSON data, first install
//
//      json.hpp  https://github.com/nlohmann/json
//
//  Then include this file, and then do
//
//     PackageManager1RequestInteractionAdditionalMessage.hpp data = nlohmann::json::parse(jsonString);

#pragma once

#include <optional>
#include <nlohmann/json.hpp>
#include "linglong/api/types/v1/helper.hpp"

namespace linglong {
namespace api {
namespace types {
namespace v1 {
/**
* fill ref string to additional message in requestInteraction
*/

using nlohmann::json;

/**
* fill ref string to additional message in requestInteraction
*/
struct PackageManager1RequestInteractionAdditionalMessage {
std::string localRef;
std::string remoteRef;
};
}
}
}
}

// clang-format on