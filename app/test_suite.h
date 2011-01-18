// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef APP_TEST_SUITE_H_
#define APP_TEST_SUITE_H_
#pragma once

#include "build/build_config.h"

#include "app/app_paths.h"
#include "app/resource_bundle.h"
#include "base/path_service.h"
#include "base/mac/scoped_nsautorelease_pool.h"
#include "base/test/test_suite.h"
#include "build/build_config.h"

#if defined(OS_MACOSX)
#include "base/mac/mac_util.h"
#include "base/test/mock_chrome_application_mac.h"
#endif

class AppTestSuite : public base::TestSuite {
 public:
  AppTestSuite(int argc, char** argv) : TestSuite(argc, argv) {
  }

 protected:

  virtual void Initialize() {
#if defined(OS_MACOSX)
    // Some of the app unit tests try to open windows.
    mock_cr_app::RegisterMockCrApp();
#endif

    base::mac::ScopedNSAutoreleasePool autorelease_pool;

    TestSuite::Initialize();

    app::RegisterPathProvider();
#if defined(OS_MACOSX)
    // Look in the framework bundle for resources.
    // TODO(port): make a resource bundle for non-app exes.  What's done here
    // isn't really right because this code needs to depend on chrome_dll
    // being built.  This is inappropriate in app.
    FilePath path;
    PathService::Get(base::DIR_EXE, &path);
#if defined(GOOGLE_CHROME_BUILD)
    path = path.AppendASCII("Google Chrome Framework.framework");
#elif defined(CHROMIUM_BUILD)
    path = path.AppendASCII("Chromium Framework.framework");
#else
#error Unknown branding
#endif
    base::mac::SetOverrideAppBundlePath(path);
#elif defined(OS_POSIX)
    FilePath pak_dir;
    PathService::Get(base::DIR_MODULE, &pak_dir);
    pak_dir = pak_dir.AppendASCII("app_unittests_strings");
    PathService::Override(app::DIR_LOCALES, pak_dir);
    PathService::Override(app::FILE_RESOURCES_PAK,
                          pak_dir.AppendASCII("app_resources.pak"));
#endif

    // Force unittests to run using en-US so if we test against string
    // output, it'll pass regardless of the system language.
    ResourceBundle::InitSharedInstance("en-US");
  }

  virtual void Shutdown() {
    ResourceBundle::CleanupSharedInstance();

#if defined(OS_MACOSX)
    base::mac::SetOverrideAppBundle(NULL);
#endif
    TestSuite::Shutdown();
  }
};

#endif  // APP_TEST_SUITE_H_
