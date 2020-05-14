/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/logging_util.h"

#include "base/strings/stringprintf.h"

namespace ads {

std::string ToString(
    const std::string& url,
    const std::vector<std::string>& headers,
    const std::string& content,
    const std::string& content_type,
    const URLRequestMethod method) {
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
    const std::string& url,
    const int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  std::string formatted_headers;
  for (const auto& header : headers) {
    formatted_headers += "    ";
    formatted_headers += header.first;
    formatted_headers += ": ";
    formatted_headers += header.second;
    formatted_headers += "\n";
  }

  return base::StringPrintf("URL Response:\n  URL: %s\n  Response "
      "Status Code: %d\n  Response: %s\n  Headers: %s\n", url.c_str(),
          response_status_code, response.c_str(), formatted_headers.c_str());
}

}  // namespace ads
