// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base/command_line.h"
#include "base/file_util.h"
#include "base/path_service.h"
#include "base/test/test_file_util.h"
#include "base/values.h"
#include "build/build_config.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/json_value_serializer.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/automation/browser_proxy.h"
#include "chrome/test/automation/window_proxy.h"
#include "chrome/test/ui/ui_test.h"
#include "gfx/rect.h"

class PreferenceServiceTest : public UITest {
 public:
  void SetUp() {
    PathService::Get(base::DIR_TEMP, &tmp_profile_);
    tmp_profile_ = tmp_profile_.AppendASCII("tmp_profile");

    // Create a fresh, empty copy of this directory.
    file_util::Delete(tmp_profile_, true);
    file_util::CreateDirectory(tmp_profile_);

    FilePath reference_pref_file =
        test_data_directory_
            .AppendASCII("profiles")
            .AppendASCII("window_placement")
            .Append(chrome::kLocalStateFilename);

    tmp_pref_file_ = tmp_profile_.Append(chrome::kLocalStateFilename);

    ASSERT_TRUE(file_util::PathExists(reference_pref_file));

    // Copy only the Local State file, the rest will be automatically created
    ASSERT_TRUE(file_util::CopyFile(reference_pref_file, tmp_pref_file_));

#if defined(OS_WIN)
    // Make the copy writable.  On POSIX we assume the umask allows files
    // we create to be writable.
    ASSERT_TRUE(::SetFileAttributesW(tmp_pref_file_.value().c_str(),
        FILE_ATTRIBUTE_NORMAL));
#endif

    launch_arguments_.AppendSwitchPath(switches::kUserDataDir, tmp_profile_);
  }

  bool LaunchAppWithProfile() {
    if (!file_util::PathExists(tmp_pref_file_))
      return false;
    UITest::SetUp();
    return true;
  }

  void TearDown() {
    UITest::TearDown();

    EXPECT_TRUE(file_util::DieFileDie(tmp_profile_, true));
  }

 public:
  FilePath tmp_pref_file_;
  FilePath tmp_profile_;
};

#if defined(OS_WIN)
// This test verifies that the window position from the prefs file is restored
// when the app restores.  This doesn't really make sense on Linux, where
// the window manager might fight with you over positioning.  However, we
// might be able to make this work on buildbots.
// Also, not sure what should happen on the mac.  In any case, the code below
// (minus the Windows bits) compiles fine on my Linux box now.
// TODO(port): revisit this.
TEST_F(PreferenceServiceTest, PreservedWindowPlacementIsLoaded) {
  // The window should open with the reference profile
  ASSERT_TRUE(LaunchAppWithProfile());

  ASSERT_TRUE(file_util::PathExists(tmp_pref_file_));

  JSONFileValueSerializer deserializer(tmp_pref_file_);
  scoped_ptr<Value> root(deserializer.Deserialize(NULL, NULL));

  ASSERT_TRUE(root.get());
  ASSERT_TRUE(root->IsType(Value::TYPE_DICTIONARY));

  DictionaryValue* root_dict = static_cast<DictionaryValue*>(root.get());

  // Retrieve the screen rect for the launched window
  scoped_refptr<BrowserProxy> browser(automation()->GetBrowserWindow(0));
  ASSERT_TRUE(browser.get());
  scoped_refptr<WindowProxy> window(browser->GetWindow());
  ASSERT_TRUE(window.get());

  gfx::Rect bounds;
  ASSERT_TRUE(window->GetBounds(&bounds));

  // Retrieve the expected rect values from "Preferences"
  int bottom = 0;
  std::string kBrowserWindowPlacement(prefs::kBrowserWindowPlacement);
  EXPECT_TRUE(root_dict->GetInteger(kBrowserWindowPlacement + ".bottom",
      &bottom));
  EXPECT_EQ(bottom, bounds.y() + bounds.height());

  int top = 0;
  EXPECT_TRUE(root_dict->GetInteger(kBrowserWindowPlacement + ".top",
      &top));
  EXPECT_EQ(top, bounds.y());

  int left = 0;
  EXPECT_TRUE(root_dict->GetInteger(kBrowserWindowPlacement + ".left",
      &left));
  EXPECT_EQ(left, bounds.x());

  int right = 0;
  EXPECT_TRUE(root_dict->GetInteger(kBrowserWindowPlacement + ".right",
      &right));
  EXPECT_EQ(right, bounds.x() + bounds.width());

  // Find if launched window is maximized.
  bool is_window_maximized = false;
  ASSERT_TRUE(window->IsMaximized(&is_window_maximized));
  bool is_maximized = false;
  EXPECT_TRUE(root_dict->GetBoolean(kBrowserWindowPlacement + ".maximized",
      &is_maximized));
  EXPECT_EQ(is_maximized, is_window_maximized);
}
#endif
