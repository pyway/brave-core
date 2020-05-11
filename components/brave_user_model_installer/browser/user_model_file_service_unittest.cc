/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "brave/components/brave_user_model_installer/browser/user_model_file_service.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=UserModelFileService*

namespace brave_user_model_installer {

TEST(UserModelFileServiceTest, InitializesServiceTest) {
  EXPECT_TRUE(true);
}

// TODO(Moritz Haller): Test deserialisation

}  // namespace brave_user_model_installer
