// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webkit/plugins/npapi/plugin_group.h"

#include <string>
#include <vector>

#include "base/scoped_ptr.h"
#include "base/string_util.h"
#include "base/utf_string_conversions.h"
#include "base/values.h"
#include "base/version.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "webkit/plugins/npapi/webplugininfo.h"
#include "webkit/plugins/npapi/plugin_list.h"

namespace webkit {
namespace npapi {

static const VersionRangeDefinition kPluginVersionRange[] = {
    { "", "", "3.0.44" }
};
static const VersionRangeDefinition kPlugin3VersionRange[] = {
    { "0", "4", "3.0.44" }
};
static const VersionRangeDefinition kPlugin4VersionRange[] = {
    { "4", "5", "4.0.44" }
};
static const VersionRangeDefinition kPlugin34VersionRange[] = {
    { "0", "4", "3.0.44" },
    { "4", "5", "" }
};

static const PluginGroupDefinition kPluginDef = {
    "myplugin", "MyPlugin", "MyPlugin", kPluginVersionRange,
    arraysize(kPluginVersionRange), "http://latest/" };
static const PluginGroupDefinition kPluginDef3 = {
    "myplugin-3", "MyPlugin 3", "MyPlugin", kPlugin3VersionRange,
    arraysize(kPlugin3VersionRange), "http://latest" };
static const PluginGroupDefinition kPluginDef4 = {
    "myplugin-4", "MyPlugin 4", "MyPlugin", kPlugin4VersionRange,
    arraysize(kPlugin4VersionRange), "http://latest" };
static const PluginGroupDefinition kPluginDef34 = {
    "myplugin-34", "MyPlugin 3/4", "MyPlugin", kPlugin34VersionRange,
    arraysize(kPlugin34VersionRange), "http://latest" };
static const PluginGroupDefinition kPluginDefNotVulnerable = {
    "myplugin-latest", "MyPlugin", "MyPlugin", NULL, 0, "http://latest" };

// name, path, version, desc.
static WebPluginInfo kPluginNoVersion = WebPluginInfo(
    ASCIIToUTF16("MyPlugin"), FilePath(FILE_PATH_LITERAL("myplugin.so.2.0.43")),
    ASCIIToUTF16(""), ASCIIToUTF16("MyPlugin version 2.0.43"));
static WebPluginInfo kPlugin2043 = WebPluginInfo(
    ASCIIToUTF16("MyPlugin"), FilePath(FILE_PATH_LITERAL("myplugin.so.2.0.43")),
    ASCIIToUTF16("2.0.43"), ASCIIToUTF16("MyPlugin version 2.0.43"));
static WebPluginInfo kPlugin3043 = WebPluginInfo(
    ASCIIToUTF16("MyPlugin"), FilePath(FILE_PATH_LITERAL("myplugin.so.3.0.43")),
    ASCIIToUTF16("3.0.43"), ASCIIToUTF16("MyPlugin version 3.0.43"));
static WebPluginInfo kPlugin3044 = WebPluginInfo(
    ASCIIToUTF16("MyPlugin"), FilePath(FILE_PATH_LITERAL("myplugin.so.3.0.44")),
    ASCIIToUTF16("3.0.44"), ASCIIToUTF16("MyPlugin version 3.0.44"));
static WebPluginInfo kPlugin3045 = WebPluginInfo(
    ASCIIToUTF16("MyPlugin"), FilePath(FILE_PATH_LITERAL("myplugin.so.3.0.45")),
     ASCIIToUTF16("3.0.45"), ASCIIToUTF16("MyPlugin version 3.0.45"));
static WebPluginInfo kPlugin3045r = WebPluginInfo(
    ASCIIToUTF16("MyPlugin"), FilePath(FILE_PATH_LITERAL("myplugin.so.3.0.45")),
     ASCIIToUTF16("3.0r45"), ASCIIToUTF16("MyPlugin version 3.0r45"));
static WebPluginInfo kPlugin4043 = WebPluginInfo(
    ASCIIToUTF16("MyPlugin"), FilePath(FILE_PATH_LITERAL("myplugin.so.4.0.43")),
     ASCIIToUTF16("4.0.43"), ASCIIToUTF16("MyPlugin version 4.0.43"));

class PluginGroupTest : public testing::Test {
 public:
  static PluginGroup* CreatePluginGroup(
      const PluginGroupDefinition& definition) {
    return PluginGroup::FromPluginGroupDefinition(definition);
  }
  static PluginGroup* CreatePluginGroup(const WebPluginInfo& wpi) {
    return PluginGroup::FromWebPluginInfo(wpi);
  }
 protected:
  virtual void TearDown() {
    PluginGroup::SetPolicyDisabledPluginPatterns(std::set<string16>());
  }
};

TEST(PluginGroupTest, PluginGroupMatch) {
  scoped_ptr<PluginGroup> group(PluginGroupTest::CreatePluginGroup(
      kPluginDef3));
  EXPECT_TRUE(group->Match(kPlugin3045));
  EXPECT_TRUE(group->Match(kPlugin3045r));
  EXPECT_FALSE(group->Match(kPluginNoVersion));
  group->AddPlugin(kPlugin3045, 0);
  EXPECT_FALSE(group->IsVulnerable());

  group.reset(PluginGroupTest::CreatePluginGroup(kPluginDef));
  EXPECT_FALSE(group->Match(kPluginNoVersion));
}

TEST(PluginGroupTest, PluginGroupMatchCorrectVersion) {
  scoped_ptr<PluginGroup> group(PluginGroupTest::CreatePluginGroup(
      kPluginDef3));
  EXPECT_TRUE(group->Match(kPlugin2043));
  EXPECT_TRUE(group->Match(kPlugin3043));
  EXPECT_FALSE(group->Match(kPlugin4043));

  group.reset(PluginGroupTest::CreatePluginGroup(kPluginDef4));
  EXPECT_FALSE(group->Match(kPlugin2043));
  EXPECT_FALSE(group->Match(kPlugin3043));
  EXPECT_TRUE(group->Match(kPlugin4043));

  group.reset(PluginGroupTest::CreatePluginGroup(kPluginDef34));
  EXPECT_TRUE(group->Match(kPlugin2043));
  EXPECT_TRUE(group->Match(kPlugin3043));
  EXPECT_TRUE(group->Match(kPlugin4043));
}

TEST(PluginGroupTest, PluginGroupDescription) {
  string16 desc3043(ASCIIToUTF16("MyPlugin version 3.0.43"));
  string16 desc3045(ASCIIToUTF16("MyPlugin version 3.0.45"));

  PluginGroupDefinition plugindefs[] =
      { kPluginDef, kPluginDef3, kPluginDef34 };
  for (size_t i = 0; i < arraysize(plugindefs); ++i) {
    WebPluginInfo plugin3043(kPlugin3043);
    WebPluginInfo plugin3045(kPlugin3045);
    {
      scoped_ptr<PluginGroup> group(PluginGroupTest::CreatePluginGroup(
          plugindefs[i]));
      EXPECT_TRUE(group->Match(plugin3043));
      group->AddPlugin(plugin3043, 0);
      EXPECT_EQ(desc3043, group->description());
      EXPECT_TRUE(group->IsVulnerable());
      EXPECT_TRUE(group->Match(plugin3045));
      group->AddPlugin(plugin3045, 1);
      EXPECT_EQ(desc3043, group->description());
      EXPECT_TRUE(group->IsVulnerable());
    }

    {
      // Disable the first plugin.
      plugin3043.enabled = false;
      scoped_ptr<PluginGroup> group(PluginGroupTest::CreatePluginGroup(
          plugindefs[i]));
      EXPECT_TRUE(group->Match(plugin3043));
      group->AddPlugin(plugin3043, 0);
      EXPECT_EQ(desc3043, group->description());
      EXPECT_TRUE(group->IsVulnerable());
      EXPECT_FALSE(group->Enabled());
      EXPECT_TRUE(group->Match(plugin3045));
      group->AddPlugin(plugin3045, 1);
      EXPECT_EQ(desc3045, group->description());
      EXPECT_FALSE(group->IsVulnerable());
    }

    {
      // Disable the second plugin.
      plugin3045.enabled = false;
      scoped_ptr<PluginGroup> group(PluginGroupTest::CreatePluginGroup(
          plugindefs[i]));
      EXPECT_TRUE(group->Match(plugin3043));
      group->AddPlugin(plugin3043, 1);
      EXPECT_EQ(desc3043, group->description());
      EXPECT_TRUE(group->IsVulnerable());
      EXPECT_TRUE(group->Match(plugin3045));
      group->AddPlugin(plugin3045, 0);
      EXPECT_EQ(desc3043, group->description());
      EXPECT_TRUE(group->IsVulnerable());
    }
  }
}

TEST(PluginGroupTest, PluginGroupDefinition) {
  const PluginGroupDefinition* definitions =
      PluginList::GetPluginGroupDefinitions();
  for (size_t i = 0; i < PluginList::GetPluginGroupDefinitionsSize(); ++i) {
    scoped_ptr<PluginGroup> def_group(
        PluginGroupTest::CreatePluginGroup(definitions[i]));
    ASSERT_TRUE(def_group.get() != NULL);
    EXPECT_FALSE(def_group->Match(kPlugin2043));
  }
}

TEST(PluginGroupTest, DisableOutdated) {
  PluginGroupDefinition plugindefs[] = { kPluginDef3, kPluginDef34 };
  for (size_t i = 0; i < 2; ++i) {
    scoped_ptr<PluginGroup> group(PluginGroupTest::CreatePluginGroup(
        plugindefs[i]));
    group->AddPlugin(kPlugin3043, 0);
    group->AddPlugin(kPlugin3045, 1);
    EXPECT_EQ(ASCIIToUTF16("MyPlugin version 3.0.43"), group->description());
    EXPECT_TRUE(group->IsVulnerable());

    group->DisableOutdatedPlugins();
    EXPECT_EQ(ASCIIToUTF16("MyPlugin version 3.0.45"), group->description());
    EXPECT_FALSE(group->IsVulnerable());
  }
}

TEST(PluginGroupTest, VersionExtraction) {
  // Some real-world plugin versions (spaces, commata, parentheses, 'r', oh my)
  const char* versions[][2] = {
    { "7.6.6 (1671)", "7.6.6.1671" },  // Quicktime
    { "2, 0, 0, 254", "2.0.0.254" },   // DivX
    { "3, 0, 0, 0", "3.0.0.0" },       // Picasa
    { "1, 0, 0, 1", "1.0.0.1" },       // Earth
    { "10,0,45,2", "10.0.45.2" },      // Flash
    { "11.5.7r609", "11.5.7.609"},     // Shockwave
    { "10.1 r102", "10.1.102"},        // Flash
    { "1.6.0_22", "1.6.0.22"},         // Java
  };

  for (size_t i = 0; i < arraysize(versions); i++) {
    const WebPluginInfo plugin = WebPluginInfo(
        ASCIIToUTF16("Blah Plugin"), FilePath(FILE_PATH_LITERAL("blahfile")),
        ASCIIToUTF16(versions[i][0]), string16());
    scoped_ptr<PluginGroup> group(PluginGroupTest::CreatePluginGroup(plugin));
    EXPECT_TRUE(group->Match(plugin));
    group->AddPlugin(plugin, 0);
    scoped_ptr<DictionaryValue> data(group->GetDataForUI());
    std::string version;
    data->GetString("version", &version);
    EXPECT_EQ(versions[i][1], version);
  }
}

TEST(PluginGroupTest, DisabledByPolicy) {
  std::set<string16> disabled_plugins;
  disabled_plugins.insert(ASCIIToUTF16("Disable this!"));
  disabled_plugins.insert(ASCIIToUTF16("*Google*"));
  PluginGroup::SetPolicyDisabledPluginPatterns(disabled_plugins);

  EXPECT_FALSE(PluginGroup::IsPluginNameDisabledByPolicy(ASCIIToUTF16("42")));
  EXPECT_TRUE(PluginGroup::IsPluginNameDisabledByPolicy(
      ASCIIToUTF16("Disable this!")));
  EXPECT_TRUE(PluginGroup::IsPluginNameDisabledByPolicy(
      ASCIIToUTF16("Google Earth")));
}

TEST(PluginGroupTest, IsVulnerable) {
  // Adobe Reader 10
  VersionRangeDefinition adobe_reader_version_range[] = {
      { "10", "11", "" },
      { "9", "10", "9.4.1" },
      { "0", "9", "8.2.5" }
  };
  PluginGroupDefinition adobe_reader_plugin_def = {
      "adobe-reader", "Adobe Reader", "Adobe Acrobat",
      adobe_reader_version_range, arraysize(adobe_reader_version_range),
      "http://get.adobe.com/reader/" };
  WebPluginInfo adobe_reader_plugin(ASCIIToUTF16("Adobe Reader"),
                                    FilePath(FILE_PATH_LITERAL("/reader.so")),
                                    ASCIIToUTF16("10.0.0.396"),
                                    ASCIIToUTF16("adobe reader 10"));
  scoped_ptr<PluginGroup> group(PluginGroupTest::CreatePluginGroup(
      adobe_reader_plugin_def));
  group->AddPlugin(adobe_reader_plugin, 0);
  PluginGroup group_copy(*group);  // Exercise the copy constructor.
  EXPECT_FALSE(group_copy.IsVulnerable());

  // Silverlight 4
  VersionRangeDefinition silverlight_version_range[] = {
      { "0", "4", "3.0.50106.0" },
      { "4", "5", "" }
  };
  PluginGroupDefinition silverlight_plugin_def = {
      "silverlight", "Silverlight", "Silverlight", silverlight_version_range,
      arraysize(silverlight_version_range),
      "http://www.microsoft.com/getsilverlight/" };
  WebPluginInfo silverlight_plugin(ASCIIToUTF16("Silverlight"),
                                   FilePath(FILE_PATH_LITERAL("/silver.so")),
                                   ASCIIToUTF16("4.0.50917.0"),
                                   ASCIIToUTF16("silverlight 4"));
  group.reset(PluginGroupTest::CreatePluginGroup(silverlight_plugin_def));
  group->AddPlugin(silverlight_plugin, 0);
  EXPECT_FALSE(PluginGroup(*group).IsVulnerable());
}
}  // namespace npapi
}  // namespace webkit
