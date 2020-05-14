/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_LEDGER_INTERNAL_LOGGING_UTIL_H_
#define BAT_LEDGER_INTERNAL_LOGGING_UTIL_H_

#include <map>
#include <string>
#include <vector>

#include "bat/ledger/mojom_structs.h"

namespace ledger {

std::string ToString(
    const std::string& url,
    const std::vector<std::string>& headers,
    const std::string& content,
    const std::string& content_type,
    const UrlMethod method);

std::string ToString(
    const char* func,
    const int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers);

}  // namespace ledger

#endif  // BAT_LEDGER_INTERNAL_LOGGING_UTIL_H_
