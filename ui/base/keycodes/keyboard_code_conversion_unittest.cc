// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/keycodes/keyboard_code_conversion.h"
#include "ui/base/keycodes/keyboard_codes.h"
#include "base/logging.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ui {

TEST(KeyCodeFromKeyIdentifierTest, MatchOnIdentifier) {
  EXPECT_EQ(VKEY_APPS, KeyCodeFromKeyIdentifier("Apps"));
  EXPECT_EQ(VKEY_UNKNOWN, KeyCodeFromKeyIdentifier("Nonsense"));
}

TEST(KeyCodeFromKeyIdentifierTest, MatchOnCharacter) {
  EXPECT_EQ(VKEY_A, KeyCodeFromKeyIdentifier("a"));
  EXPECT_EQ(VKEY_A, KeyCodeFromKeyIdentifier("A"));
  EXPECT_EQ(VKEY_OEM_PERIOD, KeyCodeFromKeyIdentifier(">"));

  std::string non_printing_char(" ");
  non_printing_char[0] = static_cast<char>(1);
  EXPECT_EQ(VKEY_UNKNOWN, KeyCodeFromKeyIdentifier(non_printing_char));
}

TEST(KeyCodeFromKeyIdentifierTest, MatchOnUnicodeCodepoint) {
  EXPECT_EQ(VKEY_A, KeyCodeFromKeyIdentifier("U+0041"));
  EXPECT_EQ(VKEY_A, KeyCodeFromKeyIdentifier("U+0061"));
  EXPECT_EQ(VKEY_DELETE, KeyCodeFromKeyIdentifier("U+007F"));

  // this one exists in the map, but has no valid VKEY
  EXPECT_EQ(VKEY_UNKNOWN, KeyCodeFromKeyIdentifier("U+030A"));

  // this one is not in the map
  EXPECT_EQ(VKEY_UNKNOWN, KeyCodeFromKeyIdentifier("U+0001"));
}

TEST(KeyCodeFromKeyIdentifierTest, DoesNotMatchEmptyString) {
  EXPECT_EQ(VKEY_UNKNOWN, KeyCodeFromKeyIdentifier(""));
}

}  // namespace ui
