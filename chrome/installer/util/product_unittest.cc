// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/util/product_unittest.h"

#include "base/logging.h"
#include "base/utf_string_conversions.h"
#include "chrome/installer/util/chrome_frame_distribution.h"
#include "chrome/installer/util/google_update_constants.h"
#include "chrome/installer/util/installation_state.h"
#include "chrome/installer/util/package.h"
#include "chrome/installer/util/package_properties.h"
#include "chrome/installer/util/master_preferences.h"
#include "chrome/installer/util/product.h"

using base::win::RegKey;
using installer::ChromePackageProperties;
using installer::ChromiumPackageProperties;
using installer::Package;
using installer::Product;
using installer::ProductPackageMapping;
using installer::MasterPreferences;

void TestWithTempDir::SetUp() {
  // Name a subdirectory of the user temp directory.
  ASSERT_TRUE(test_dir_.CreateUniqueTempDir());
}

void TestWithTempDir::TearDown() {
  logging::CloseLogFile();
  ASSERT_TRUE(test_dir_.Delete());
}

////////////////////////////////////////////////////////////////////////////////

void TestWithTempDirAndDeleteTempOverrideKeys::SetUp() {
  TempRegKeyOverride::DeleteAllTempKeys();
  TestWithTempDir::SetUp();
}

void TestWithTempDirAndDeleteTempOverrideKeys::TearDown() {
  TestWithTempDir::TearDown();
  TempRegKeyOverride::DeleteAllTempKeys();
}

////////////////////////////////////////////////////////////////////////////////
const wchar_t TempRegKeyOverride::kTempTestKeyPath[] =
    L"Software\\Chromium\\TempTestKeys";

TempRegKeyOverride::TempRegKeyOverride(HKEY override, const wchar_t* temp_name)
    : override_(override), temp_name_(temp_name) {
  DCHECK(temp_name && lstrlenW(temp_name));
  std::wstring key_path(kTempTestKeyPath);
  key_path += L"\\" + temp_name_;
  EXPECT_TRUE(temp_key_.Create(HKEY_CURRENT_USER, key_path.c_str(),
                               KEY_ALL_ACCESS));
  EXPECT_EQ(ERROR_SUCCESS,
            ::RegOverridePredefKey(override_, temp_key_.Handle()));
}

TempRegKeyOverride::~TempRegKeyOverride() {
  ::RegOverridePredefKey(override_, NULL);
  // The temp key will be deleted via a call to DeleteAllTempKeys().
}

// static
void TempRegKeyOverride::DeleteAllTempKeys() {
  RegKey key;
  if (key.Open(HKEY_CURRENT_USER, L"", KEY_ALL_ACCESS)) {
    key.DeleteKey(kTempTestKeyPath);
  }
}

////////////////////////////////////////////////////////////////////////////////

class ProductTest : public TestWithTempDirAndDeleteTempOverrideKeys {
 protected:
};

TEST_F(ProductTest, ProductInstallBasic) {
  // TODO(tommi): We should mock this and use our mocked distribution.
  const bool multi_install = false;
  const bool system_level = true;
  const installer::MasterPreferences& prefs =
      installer::MasterPreferences::ForCurrentProcess();
  BrowserDistribution* distribution =
      BrowserDistribution::GetSpecificDistribution(
          BrowserDistribution::CHROME_BROWSER, prefs);
  ChromePackageProperties properties;
  scoped_refptr<Package> package(new Package(multi_install, system_level,
                                             test_dir_.path(), &properties));
  scoped_refptr<Product> product(new Product(distribution, package.get()));

  FilePath user_data(product->GetUserDataPath());
  EXPECT_FALSE(user_data.empty());
  EXPECT_NE(std::wstring::npos,
            user_data.value().find(installer::kInstallUserDataDir));

  FilePath program_files;
  PathService::Get(base::DIR_PROGRAM_FILES, &program_files);
  // The User Data path should never be under program files, even though
  // system_level is true.
  EXPECT_EQ(std::wstring::npos,
            user_data.value().find(program_files.value()));

  // We started out with a non-msi product.
  EXPECT_FALSE(product->IsMsi());

  HKEY root = system_level ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
  {
    TempRegKeyOverride override(root, L"root_pit");

    // Create a make-believe client state key.
    RegKey key;
    std::wstring state_key_path(distribution->GetStateKey());
    ASSERT_TRUE(key.Create(root, state_key_path.c_str(), KEY_ALL_ACCESS));

    // Set the MSI marker, delete the objects, create new ones and verify
    // that we now see the MSI marker.
    EXPECT_TRUE(product->SetMsiMarker(true));
    package = new Package(multi_install, system_level, test_dir_.path(),
                          &properties);
    product = new Product(distribution, package.get());
    EXPECT_TRUE(product->IsMsi());

    // There should be no installed version in the registry.
    {
      installer::InstallationState state;
      state.Initialize(prefs);
      EXPECT_TRUE(state.GetProductState(system_level,
                                        distribution->GetType()) == NULL);
    }

    // Let's pretend chrome is installed.
    RegKey version_key(root, distribution->GetVersionKey().c_str(),
                       KEY_ALL_ACCESS);
    ASSERT_TRUE(version_key.Valid());

    const char kCurrentVersion[] = "1.2.3.4";
    scoped_ptr<Version> current_version(
        Version::GetVersionFromString(kCurrentVersion));
    version_key.WriteValue(google_update::kRegVersionField,
                           UTF8ToWide(current_version->GetString()).c_str());

    {
      installer::InstallationState state;
      state.Initialize(prefs);
      const installer::ProductState* prod_state =
          state.GetProductState(system_level, distribution->GetType());
      EXPECT_TRUE(prod_state != NULL);
      if (prod_state != NULL) {
        EXPECT_TRUE(prod_state->version().Equals(*current_version.get()));
      }
    }
  }
}

TEST_F(ProductTest, LaunchChrome) {
  // TODO(tommi): Test Product::LaunchChrome and
  // Product::LaunchChromeAndWait.
  LOG(ERROR) << "Test not implemented.";
}

// Overrides ChromeFrameDistribution for the sole purpose of returning
// the Chrome (not Chrome Frame) installation path.
class FakeChromeFrameDistribution : public ChromeFrameDistribution {
 public:
  explicit FakeChromeFrameDistribution(
      const installer::MasterPreferences& prefs)
          : ChromeFrameDistribution(prefs) {}
  virtual std::wstring GetInstallSubDir() {
    const MasterPreferences& prefs =
        installer::MasterPreferences::ForCurrentProcess();
    return BrowserDistribution::GetSpecificDistribution(
        BrowserDistribution::CHROME_BROWSER, prefs)->GetInstallSubDir();
  }
};

TEST_F(ProductTest, ProductInstallsBasic) {
  const bool multi_install = true;
  const bool system_level = true;
  ProductPackageMapping installs(multi_install, system_level);
  EXPECT_EQ(multi_install, installs.multi_install());
  EXPECT_EQ(system_level, installs.system_level());
  EXPECT_EQ(0U, installs.packages().size());
  EXPECT_EQ(0U, installs.products().size());

  // TODO(robertshield): Include test that use mock master preferences.
  const MasterPreferences& prefs =
      installer::MasterPreferences::ForCurrentProcess();

  installs.AddDistribution(BrowserDistribution::CHROME_BROWSER, prefs);
  FakeChromeFrameDistribution fake_chrome_frame(prefs);
  installs.AddDistribution(&fake_chrome_frame);
  EXPECT_EQ(2U, installs.products().size());
  // Since our fake Chrome Frame distribution class is reporting the same
  // installation directory as Chrome, we should have only one package object.
  EXPECT_EQ(1U, installs.packages().size());
  EXPECT_EQ(multi_install, installs.packages()[0]->multi_install());
}
