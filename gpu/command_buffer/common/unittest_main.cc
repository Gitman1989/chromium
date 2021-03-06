// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/at_exit.h"
#include "app/gfx/gl/gl_implementation.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

int main(int argc, char** argv) {
  base::AtExitManager exit_manager;
  gfx::InitializeGLBindings(gfx::kGLImplementationMockGL);
  testing::InitGoogleMock(&argc, argv);
  return RUN_ALL_TESTS();
}
