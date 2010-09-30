// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "app/l10n_util.h"
#include "base/scoped_nsobject.h"
#include "base/string_util.h"
#include "base/string_number_conversions.h"
#include "base/sys_string_conversions.h"
#include "base/utf_string_conversions.h"
#import "chrome/browser/cocoa/page_info_bubble_controller.h"
#include "chrome/browser/cocoa/browser_test_helper.h"
#import "chrome/browser/cocoa/cocoa_test_helper.h"
#include "chrome/browser/page_info_model.h"
#include "grit/generated_resources.h"

namespace {

class FakeModel : public PageInfoModel {
 public:
  void AddSection(SectionInfoState state,
                  const string16& title,
                  const string16& description,
                  SectionInfoType type) {
    sections_.push_back(SectionInfo(
        state, title, string16(), description, type));
  }
};

class FakeBridge : public PageInfoModel::PageInfoModelObserver {
 public:
  void ModelChanged() {}
};

class PageInfoBubbleControllerTest : public CocoaTest {
 public:
  PageInfoBubbleControllerTest() {
    controller_ = nil;
    model_ = new FakeModel();
  }

  virtual void TearDown() {
    [controller_ close];
    CocoaTest::TearDown();
  }

  void CreateBubble() {
    // The controller cleans up after itself when the window closes.
    controller_ =
        [[PageInfoBubbleController alloc] initWithPageInfoModel:model_
                                                  modelObserver:NULL
                                                   parentWindow:test_window()];
    window_ = [controller_ window];
    [controller_ showWindow:nil];
  }

  // Checks the controller's window for the requisite subviews in the given
  // numbers.
  void CheckWindow(int text_count,
                   int image_count,
                   int spacer_count,
                   int button_count) {
    for (NSView* view in [[window_ contentView] subviews]) {
      if ([view isKindOfClass:[NSTextField class]]) {
        --text_count;
      } else if ([view isKindOfClass:[NSImageView class]]) {
        --image_count;
      } else if ([view isKindOfClass:[NSBox class]]) {
        --spacer_count;
      } else if ([view isKindOfClass:[NSButton class]]) {
        --button_count;
        CheckButton(static_cast<NSButton*>(view));
      } else {
        ADD_FAILURE() << "Unknown subview: " << [[view description] UTF8String];
      }
    }
    EXPECT_EQ(0, text_count);
    EXPECT_EQ(0, image_count);
    EXPECT_EQ(0, spacer_count);
    EXPECT_EQ(0, button_count);
    EXPECT_EQ([window_ delegate], controller_);
  }

  // Checks that a button is hooked up correctly.
  void CheckButton(NSButton* button) {
    EXPECT_EQ(@selector(showCertWindow:), [button action]);
    EXPECT_EQ(controller_, [button target]);
    EXPECT_TRUE([button stringValue]);
  }

  BrowserTestHelper helper_;

  PageInfoBubbleController* controller_;  // Weak, owns self.
  FakeModel* model_;  // Weak, owned by controller.
  NSWindow* window_;  // Weak, owned by controller.
};


TEST_F(PageInfoBubbleControllerTest, NoHistoryNoSecurity) {
  model_->AddSection(PageInfoModel::SECTION_STATE_ERROR,
      l10n_util::GetStringUTF16(IDS_PAGE_INFO_SECURITY_TAB_IDENTITY_TITLE),
      l10n_util::GetStringFUTF16(
          IDS_PAGE_INFO_SECURITY_TAB_UNKNOWN_PARTY,
          ASCIIToUTF16("google.com")),
      PageInfoModel::SECTION_INFO_IDENTITY);
  model_->AddSection(PageInfoModel::SECTION_STATE_ERROR,
      l10n_util::GetStringUTF16(IDS_PAGE_INFO_SECURITY_TAB_CONNECTION_TITLE),
      l10n_util::GetStringFUTF16(
          IDS_PAGE_INFO_SECURITY_TAB_NOT_ENCRYPTED_CONNECTION_TEXT,
          ASCIIToUTF16("google.com")),
      PageInfoModel::SECTION_INFO_CONNECTION);

  CreateBubble();
  CheckWindow(/*text=*/4, /*image=*/2, /*spacer=*/1, /*button=*/1);
}


TEST_F(PageInfoBubbleControllerTest, HistoryNoSecurity) {
  model_->AddSection(PageInfoModel::SECTION_STATE_ERROR,
      l10n_util::GetStringUTF16(IDS_PAGE_INFO_SECURITY_TAB_IDENTITY_TITLE),
      l10n_util::GetStringFUTF16(
          IDS_PAGE_INFO_SECURITY_TAB_UNKNOWN_PARTY,
          ASCIIToUTF16("google.com")),
      PageInfoModel::SECTION_INFO_IDENTITY);
  model_->AddSection(PageInfoModel::SECTION_STATE_ERROR,
      l10n_util::GetStringUTF16(IDS_PAGE_INFO_SECURITY_TAB_CONNECTION_TITLE),
      l10n_util::GetStringFUTF16(
          IDS_PAGE_INFO_SECURITY_TAB_NOT_ENCRYPTED_CONNECTION_TEXT,
          ASCIIToUTF16("google.com")),
      PageInfoModel::SECTION_INFO_CONNECTION);

  // In practice, the history information comes later because it's queried
  // asynchronously, so replicate the double-build here.
  CreateBubble();

  model_->AddSection(PageInfoModel::SECTION_STATE_ERROR,
      l10n_util::GetStringUTF16(
          IDS_PAGE_INFO_SECURITY_TAB_PERSONAL_HISTORY_TITLE),
      l10n_util::GetStringUTF16(
          IDS_PAGE_INFO_SECURITY_TAB_FIRST_VISITED_TODAY),
      PageInfoModel::SECTION_INFO_FIRST_VISIT);

  [controller_ performLayout];

  CheckWindow(/*text=*/6, /*image=*/2, /*spacer=*/2, /*button=*/1);
}


TEST_F(PageInfoBubbleControllerTest, NoHistoryMixedSecurity) {
  model_->AddSection(PageInfoModel::SECTION_STATE_OK,
      l10n_util::GetStringUTF16(IDS_PAGE_INFO_SECURITY_TAB_IDENTITY_TITLE),
      l10n_util::GetStringFUTF16(
          IDS_PAGE_INFO_SECURITY_TAB_SECURE_IDENTITY,
          ASCIIToUTF16("Goat Security Systems")),
      PageInfoModel::SECTION_INFO_IDENTITY);

  // This string is super long and the text should overflow the default clip
  // region (kImageSize).
  string16 description = l10n_util::GetStringFUTF16(
      IDS_PAGE_INFO_SECURITY_TAB_ENCRYPTED_SENTENCE_LINK,
      l10n_util::GetStringFUTF16(
          IDS_PAGE_INFO_SECURITY_TAB_ENCRYPTED_CONNECTION_TEXT,
          ASCIIToUTF16("chrome.google.com"),
          base::IntToString16(1024)),
      l10n_util::GetStringUTF16(
          IDS_PAGE_INFO_SECURITY_TAB_ENCRYPTED_INSECURE_CONTENT_WARNING));

  model_->AddSection(PageInfoModel::SECTION_STATE_OK,
      l10n_util::GetStringUTF16(IDS_PAGE_INFO_SECURITY_TAB_CONNECTION_TITLE),
      description,
      PageInfoModel::SECTION_INFO_CONNECTION);

  CreateBubble();

  NSArray* subviews = [[window_ contentView] subviews];
  CheckWindow(/*text=*/4, /*image=*/2, /*spacer=*/1, /*button=*/1);

  // Look for the over-sized box.
  NSString* targetDesc = base::SysUTF16ToNSString(description);
  for (NSView* subview in subviews) {
    if ([subview isKindOfClass:[NSTextField class]]) {
      NSTextField* desc = static_cast<NSTextField*>(subview);
      if ([[desc stringValue] isEqualToString:targetDesc]) {
        // Typical box frame is ~55px, make sure this is extra large.
        EXPECT_LT(75, NSHeight([desc frame]));
      }
    }
  }
}

}  // namespace