/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/logging_util.h"

#include <time.h>

#include "base/strings/stringprintf.h"

namespace ledger {

std::string ToString(
    const std::string& url,
    const std::vector<std::string>& headers,
    const std::string& content,
    const std::string& content_type,
    const UrlMethod method) {
  std::string formatted_headers;
  for (const auto& header : headers) {
    formatted_headers += "    ";
    formatted_headers += header;
    formatted_headers += "\n";
  }

  return base::StringPrintf("URL Request:\n  URL: %s\n  Headers: %s\n"
      "  Content: %s\n  Content Type: %s\n  Method: %d", url.c_str(),
          formatted_headers.c_str(), content.c_str(), content_type.c_str(),
              method);
}

std::string ToString(
    const char* func,
    const int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  const std::string result =
      response_status_code >= 200 && response_status_code < 300
          ? "Success" : "Failure";

  std::string formatted_headers = "";
  for (auto header = headers.begin(); header != headers.end(); ++header) {
    formatted_headers += "> headers " + header->first + ": " + header->second;
    if (header != headers.end()) {
      formatted_headers += "\n";
    }
  }

  const std::string timestamp = std::to_string(std::time(nullptr));

  return base::StringPrintf("[ RESPONSE - %s ]\n> time: %s\n> result: %s\n"
      "> http code: %d\n> response: %s\n%s[ END RESPONSE ]", func,
          timestamp.c_str(), result.c_str(), response_status_code,
              response.c_str(), formatted_headers.c_str());
}

}  // namespace ledger
