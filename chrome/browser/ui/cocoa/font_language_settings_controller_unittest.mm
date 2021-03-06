// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/scoped_nsobject.h"
#include "chrome/browser/character_encoding.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/cocoa/browser_test_helper.h"
#import "chrome/browser/ui/cocoa/cocoa_test_helper.h"
#import "chrome/browser/ui/cocoa/font_language_settings_controller.h"
#include "testing/gtest/include/gtest/gtest.h"
#import "testing/gtest_mac.h"
#include "testing/platform_test.h"

// The FontLanguageSettingsControllerForTest overrides the getFontFieldOrigin
// method to provide a dummy point, so we don't have to actually display the
// window to test the controller.
@interface FontLanguageSettingsControllerForTest :
    FontLanguageSettingsController {
}

- (NSPoint)getFontFieldOrigin:(NSTextField*)field
                     forLabel:(NSTextField*)label;

@end

@implementation FontLanguageSettingsControllerForTest

- (NSPoint)getFontFieldOrigin:(NSTextField*)field
                     forLabel:(NSTextField*)label {
  return NSMakePoint(10, 10);
}

@end

@interface FontLanguageSettingsController (Testing)
- (void)updateDisplayField:(NSTextField*)field
                  withFont:(NSFont*)font
                 withLabel:(NSTextField*)label;
@end

class FontLanguageSettingsControllerTest : public CocoaTest {
 public:
  FontLanguageSettingsControllerTest() {
    Profile* profile = helper_.profile();
    font_controller_.reset(
        [[FontLanguageSettingsControllerForTest alloc] initWithProfile:profile]);
   }
  ~FontLanguageSettingsControllerTest() {}

  BrowserTestHelper helper_;
  scoped_nsobject<FontLanguageSettingsController> font_controller_;
};

TEST_F(FontLanguageSettingsControllerTest, Init) {
  ASSERT_EQ(CharacterEncoding::GetSupportCanonicalEncodingCount(),
      static_cast<int>([[font_controller_ encodings] count]));
}

TEST_F(FontLanguageSettingsControllerTest, UpdateDisplayField) {
  NSFont* font = [NSFont fontWithName:@"Times-Roman" size:12.0];
  scoped_nsobject<NSTextField> field(
      [[NSTextField alloc] initWithFrame:NSMakeRect(100, 100, 100, 100)]);
  scoped_nsobject<NSTextField> label(
      [[NSTextField alloc] initWithFrame:NSMakeRect(100, 100, 100, 100)]);
  [font_controller_ updateDisplayField:field.get()
                              withFont:font
                             withLabel:label];

  ASSERT_NSEQ([font fontName], [[field font] fontName]);
  ASSERT_NSEQ(@"Times-Roman, 12", [field stringValue]);
}

TEST_F(FontLanguageSettingsControllerTest, UpdateDisplayFieldNilFont) {
  scoped_nsobject<NSTextField> field(
      [[NSTextField alloc] initWithFrame:NSMakeRect(100, 100, 100, 100)]);
  scoped_nsobject<NSTextField> label(
      [[NSTextField alloc] initWithFrame:NSMakeRect(100, 100, 100, 100)]);
  [field setStringValue:@"foo"];
  [font_controller_ updateDisplayField:field.get()
                              withFont:nil
                             withLabel:label];

  ASSERT_NSEQ(@"foo", [field stringValue]);
}

TEST_F(FontLanguageSettingsControllerTest, UpdateDisplayFieldNilField) {
  // Don't crash.
  NSFont* font = [NSFont fontWithName:@"Times-Roman" size:12.0];
  [font_controller_ updateDisplayField:nil withFont:font withLabel:nil];
}
