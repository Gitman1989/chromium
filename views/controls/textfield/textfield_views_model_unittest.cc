// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/auto_reset.h"
#include "base/message_loop.h"
#include "base/scoped_ptr.h"
#include "base/utf_string_conversions.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/clipboard/clipboard.h"
#include "ui/base/clipboard/scoped_clipboard_writer.h"
#include "views/controls/textfield/textfield_views_model.h"
#include "views/test/test_views_delegate.h"
#include "views/views_delegate.h"

namespace views {

#define EXPECT_STR_EQ(ascii, utf16) \
  EXPECT_EQ(ASCIIToWide(ascii), UTF16ToWide(utf16))

TEST(TextfieldViewsModelTest, EditString) {
  TextfieldViewsModel model;
  // append two strings
  model.Append(ASCIIToUTF16("HILL"));
  EXPECT_STR_EQ("HILL", model.text());
  model.Append(ASCIIToUTF16("WORLD"));
  EXPECT_STR_EQ("HILLWORLD", model.text());

  // Insert "E" to make hello
  model.MoveCursorRight(false);
  model.Insert('E');
  EXPECT_STR_EQ("HEILLWORLD", model.text());
  // Replace "I" with "L"
  model.Replace('L');
  EXPECT_STR_EQ("HELLLWORLD", model.text());
  model.Replace('L');
  model.Replace('O');
  EXPECT_STR_EQ("HELLOWORLD", model.text());

  // Delete 6th char "W", then delete 5th char O"
  EXPECT_EQ(5U, model.cursor_pos());
  EXPECT_TRUE(model.Delete());
  EXPECT_STR_EQ("HELLOORLD", model.text());
  EXPECT_TRUE(model.Backspace());
  EXPECT_EQ(4U, model.cursor_pos());
  EXPECT_STR_EQ("HELLORLD", model.text());

  // Move the cursor to start. backspace should fail.
  model.MoveCursorToStart(false);
  EXPECT_FALSE(model.Backspace());
  EXPECT_STR_EQ("HELLORLD", model.text());
  // Move the cursor to the end. delete should fail.
  model.MoveCursorToEnd(false);
  EXPECT_FALSE(model.Delete());
  EXPECT_STR_EQ("HELLORLD", model.text());
  // but backspace should work.
  EXPECT_TRUE(model.Backspace());
  EXPECT_STR_EQ("HELLORL", model.text());
}

TEST(TextfieldViewsModelTest, EmptyString) {
  TextfieldViewsModel model;
  EXPECT_EQ(string16(), model.text());
  EXPECT_EQ(string16(), model.GetSelectedText());
  EXPECT_EQ(string16(), model.GetVisibleText());

  model.MoveCursorLeft(true);
  EXPECT_EQ(0U, model.cursor_pos());
  model.MoveCursorRight(true);
  EXPECT_EQ(0U, model.cursor_pos());

  EXPECT_EQ(string16(), model.GetSelectedText());

  EXPECT_FALSE(model.Delete());
  EXPECT_FALSE(model.Backspace());
}

TEST(TextfieldViewsModelTest, Selection) {
  TextfieldViewsModel model;
  model.Append(ASCIIToUTF16("HELLO"));
  model.MoveCursorRight(false);
  model.MoveCursorRight(true);
  EXPECT_STR_EQ("E", model.GetSelectedText());
  model.MoveCursorRight(true);
  EXPECT_STR_EQ("EL", model.GetSelectedText());

  model.MoveCursorToStart(true);
  EXPECT_STR_EQ("H", model.GetSelectedText());
  model.MoveCursorToEnd(true);
  EXPECT_STR_EQ("ELLO", model.GetSelectedText());
  model.ClearSelection();
  EXPECT_EQ(string16(), model.GetSelectedText());
  model.SelectAll();
  EXPECT_STR_EQ("HELLO", model.GetSelectedText());

  // Select and move cursor
  model.MoveCursorTo(1U, false);
  model.MoveCursorTo(3U, true);
  EXPECT_STR_EQ("EL", model.GetSelectedText());
  model.MoveCursorLeft(false);
  EXPECT_EQ(1U, model.cursor_pos());
  model.MoveCursorTo(1U, false);
  model.MoveCursorTo(3U, true);
  model.MoveCursorRight(false);
  EXPECT_EQ(3U, model.cursor_pos());

  // Select all and move cursor
  model.SelectAll();
  model.MoveCursorLeft(false);
  EXPECT_EQ(0U, model.cursor_pos());
  model.SelectAll();
  model.MoveCursorRight(false);
  EXPECT_EQ(5U, model.cursor_pos());
}

TEST(TextfieldViewsModelTest, SelectionAndEdit) {
  TextfieldViewsModel model;
  model.Append(ASCIIToUTF16("HELLO"));
  model.MoveCursorRight(false);
  model.MoveCursorRight(true);
  model.MoveCursorRight(true);  // select "EL"
  EXPECT_TRUE(model.Backspace());
  EXPECT_STR_EQ("HLO", model.text());

  model.Append(ASCIIToUTF16("ILL"));
  model.MoveCursorRight(true);
  model.MoveCursorRight(true);  // select "LO"
  EXPECT_TRUE(model.Delete());
  EXPECT_STR_EQ("HILL", model.text());
  EXPECT_EQ(1U, model.cursor_pos());
  model.MoveCursorRight(true);  // select "I"
  model.Insert('E');
  EXPECT_STR_EQ("HELL", model.text());
  model.MoveCursorToStart(false);
  model.MoveCursorRight(true);  // select "H"
  model.Replace('B');
  EXPECT_STR_EQ("BELL", model.text());
  model.MoveCursorToEnd(false);
  model.MoveCursorLeft(true);
  model.MoveCursorLeft(true);  // select ">LL"
  model.Replace('E');
  EXPECT_STR_EQ("BEE", model.text());
}

TEST(TextfieldViewsModelTest, Password) {
  TextfieldViewsModel model;
  model.set_is_password(true);
  model.Append(ASCIIToUTF16("HELLO"));
  EXPECT_STR_EQ("*****", model.GetVisibleText());
  EXPECT_STR_EQ("HELLO", model.text());
  EXPECT_TRUE(model.Delete());

  EXPECT_STR_EQ("****", model.GetVisibleText());
  EXPECT_STR_EQ("ELLO", model.text());
  EXPECT_EQ(0U, model.cursor_pos());

  model.SelectAll();
  EXPECT_STR_EQ("ELLO", model.GetSelectedText());
  EXPECT_EQ(0U, model.cursor_pos());

  model.Insert('X');
  EXPECT_STR_EQ("*", model.GetVisibleText());
  EXPECT_STR_EQ("X", model.text());
}

TEST(TextfieldViewsModelTest, Word) {
  TextfieldViewsModel model;
  model.Append(
      ASCIIToUTF16("The answer to Life, the Universe, and Everything"));
  model.MoveCursorToNextWord(false);
  EXPECT_EQ(3U, model.cursor_pos());
  model.MoveCursorToNextWord(false);
  EXPECT_EQ(10U, model.cursor_pos());
  model.MoveCursorToNextWord(false);
  model.MoveCursorToNextWord(false);
  EXPECT_EQ(18U, model.cursor_pos());

  // Should passes the non word char ','
  model.MoveCursorToNextWord(true);
  EXPECT_EQ(23U, model.cursor_pos());
  EXPECT_STR_EQ(", the", model.GetSelectedText());

  // Move to the end.
  model.MoveCursorToNextWord(true);
  model.MoveCursorToNextWord(true);
  model.MoveCursorToNextWord(true);
  EXPECT_STR_EQ(", the Universe, and Everything", model.GetSelectedText());
  // Should be safe to go next word at the end.
  model.MoveCursorToNextWord(true);
  EXPECT_STR_EQ(", the Universe, and Everything", model.GetSelectedText());
  model.Insert('2');
  EXPECT_EQ(19U, model.cursor_pos());

  // Now backwards.
  model.MoveCursorLeft(false);  // leave 2.
  model.MoveCursorToPreviousWord(true);
  EXPECT_EQ(14U, model.cursor_pos());
  EXPECT_STR_EQ("Life", model.GetSelectedText());
  model.MoveCursorToPreviousWord(true);
  EXPECT_STR_EQ("to Life", model.GetSelectedText());
  model.MoveCursorToPreviousWord(true);
  model.MoveCursorToPreviousWord(true);
  model.MoveCursorToPreviousWord(true);  // Select to the begining.
  EXPECT_STR_EQ("The answer to Life", model.GetSelectedText());
  // Should be safe to go pervious word at the begining.
  model.MoveCursorToPreviousWord(true);
  EXPECT_STR_EQ("The answer to Life", model.GetSelectedText());
  model.Replace('4');
  EXPECT_EQ(string16(), model.GetSelectedText());
  EXPECT_STR_EQ("42", model.GetVisibleText());
}

TEST(TextfieldViewsModelTest, TextFragment) {
  TextfieldViewsModel model;
  TextfieldViewsModel::TextFragments fragments;
  // Empty string
  model.GetFragments(&fragments);
  EXPECT_EQ(1U, fragments.size());
  EXPECT_EQ(0U, fragments[0].begin);
  EXPECT_EQ(0U, fragments[0].end);
  EXPECT_FALSE(fragments[0].selected);

  // Some string
  model.Append(ASCIIToUTF16("Hello world"));
  model.GetFragments(&fragments);
  EXPECT_EQ(1U, fragments.size());
  EXPECT_EQ(0U, fragments[0].begin);
  EXPECT_EQ(11U, fragments[0].end);
  EXPECT_FALSE(fragments[0].selected);

  // Select 1st word
  model.MoveCursorToNextWord(true);
  model.GetFragments(&fragments);
  EXPECT_EQ(2U, fragments.size());
  EXPECT_EQ(0U, fragments[0].begin);
  EXPECT_EQ(5U, fragments[0].end);
  EXPECT_TRUE(fragments[0].selected);
  EXPECT_EQ(5U, fragments[1].begin);
  EXPECT_EQ(11U, fragments[1].end);
  EXPECT_FALSE(fragments[1].selected);

  // Select empty string
  model.ClearSelection();
  model.MoveCursorRight(true);
  model.GetFragments(&fragments);
  EXPECT_EQ(3U, fragments.size());
  EXPECT_EQ(0U, fragments[0].begin);
  EXPECT_EQ(5U, fragments[0].end);
  EXPECT_FALSE(fragments[0].selected);
  EXPECT_EQ(5U, fragments[1].begin);
  EXPECT_EQ(6U, fragments[1].end);
  EXPECT_TRUE(fragments[1].selected);

  EXPECT_EQ(6U, fragments[2].begin);
  EXPECT_EQ(11U, fragments[2].end);
  EXPECT_FALSE(fragments[2].selected);

  // Select to the end.
  model.MoveCursorToEnd(true);
  model.GetFragments(&fragments);
  EXPECT_EQ(2U, fragments.size());
  EXPECT_EQ(0U, fragments[0].begin);
  EXPECT_EQ(5U, fragments[0].end);
  EXPECT_FALSE(fragments[0].selected);
  EXPECT_EQ(5U, fragments[1].begin);
  EXPECT_EQ(11U, fragments[1].end);
  EXPECT_TRUE(fragments[1].selected);
}

TEST(TextfieldViewsModelTest, SetText) {
  TextfieldViewsModel model;
  model.Append(ASCIIToUTF16("HELLO"));
  model.MoveCursorToEnd(false);
  model.SetText(ASCIIToUTF16("GOODBYE"));
  EXPECT_STR_EQ("GOODBYE", model.text());
  EXPECT_EQ(5U, model.cursor_pos());
  model.SelectAll();
  EXPECT_STR_EQ("GOODBYE", model.GetSelectedText());
  // Selection move the current pos to the begining.
  EXPECT_EQ(0U, model.cursor_pos());
  model.MoveCursorToEnd(false);
  EXPECT_EQ(7U, model.cursor_pos());

  model.SetText(ASCIIToUTF16("BYE"));
  EXPECT_EQ(3U, model.cursor_pos());
  EXPECT_EQ(string16(), model.GetSelectedText());
  model.SetText(ASCIIToUTF16(""));
  EXPECT_EQ(0U, model.cursor_pos());
}

#if defined(OS_WIN)
#define MAYBE_Clipboard DISABLED_Clipboard
#else
#define MAYBE_Clipboard Clipboard
#endif
TEST(TextfieldViewsModelTest, MAYBE_Clipboard) {
  scoped_ptr<TestViewsDelegate> test_views_delegate(new TestViewsDelegate());
  AutoReset<views::ViewsDelegate*> auto_reset(
      &views::ViewsDelegate::views_delegate, test_views_delegate.get());
  ui::Clipboard* clipboard
      = views::ViewsDelegate::views_delegate->GetClipboard();
  string16 initial_clipboard_text;
  clipboard->ReadText(ui::Clipboard::BUFFER_STANDARD, &initial_clipboard_text);
  string16 clipboard_text;
  TextfieldViewsModel model;
  model.Append(ASCIIToUTF16("HELLO WORLD"));
  model.MoveCursorToEnd(false);

  // Test for cut: Empty selection.
  EXPECT_FALSE(model.Cut());
  clipboard->ReadText(ui::Clipboard::BUFFER_STANDARD, &clipboard_text);
  EXPECT_STR_EQ(UTF16ToUTF8(initial_clipboard_text), clipboard_text);
  EXPECT_STR_EQ("HELLO WORLD", model.text());
  EXPECT_EQ(11U, model.cursor_pos());

  // Test for cut: Non-empty selection.
  model.MoveCursorToPreviousWord(true);
  EXPECT_TRUE(model.Cut());
  clipboard->ReadText(ui::Clipboard::BUFFER_STANDARD, &clipboard_text);
  EXPECT_STR_EQ("WORLD", clipboard_text);
  EXPECT_STR_EQ("HELLO ", model.text());
  EXPECT_EQ(6U, model.cursor_pos());

  // Test for copy: Empty selection.
  model.Copy();
  clipboard->ReadText(ui::Clipboard::BUFFER_STANDARD, &clipboard_text);
  EXPECT_STR_EQ("WORLD", clipboard_text);
  EXPECT_STR_EQ("HELLO ", model.text());
  EXPECT_EQ(6U, model.cursor_pos());

  // Test for copy: Non-empty selection.
  model.Append(ASCIIToUTF16("HELLO WORLD"));
  model.SelectAll();
  model.Copy();
  clipboard->ReadText(ui::Clipboard::BUFFER_STANDARD, &clipboard_text);
  EXPECT_STR_EQ("HELLO HELLO WORLD", clipboard_text);
  EXPECT_STR_EQ("HELLO HELLO WORLD", model.text());
  EXPECT_EQ(0U, model.cursor_pos());

  // Test for paste.
  model.ClearSelection();
  model.MoveCursorToEnd(false);
  model.MoveCursorToPreviousWord(true);
  EXPECT_TRUE(model.Paste());
  clipboard->ReadText(ui::Clipboard::BUFFER_STANDARD, &clipboard_text);
  EXPECT_STR_EQ("HELLO HELLO WORLD", clipboard_text);
  EXPECT_STR_EQ("HELLO HELLO HELLO HELLO WORLD", model.text());
  EXPECT_EQ(29U, model.cursor_pos());
}

}  // namespace views
