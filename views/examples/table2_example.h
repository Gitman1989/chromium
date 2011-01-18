// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef VIEWS_EXAMPLES_TABLE2_EXAMPLE_H_
#define VIEWS_EXAMPLES_TABLE2_EXAMPLE_H_
#pragma once

#include <string>
#include <vector>

#include "base/utf_string_conversions.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "ui/base/models/table_model.h"
#include "views/controls/button/checkbox.h"
#include "views/controls/table/table_view_observer.h"
#include "views/controls/table/table_view2.h"
#include "views/examples/example_base.h"

using ui::TableModel;
using ui::TableModelObserver;  // TODO(beng): remove these.

namespace examples {

class Table2Example
    : public ExampleBase,
      public TableModel,
      public views::ButtonListener,
      public views::TableViewObserver {
 public:
  explicit Table2Example(ExamplesMain* main) : ExampleBase(main) {
  }

  virtual ~Table2Example() {}

  virtual std::wstring GetExampleTitle() {
    return L"Table2";
  }

  virtual void CreateExampleView(views::View* container) {
    column1_visible_checkbox_ = new views::Checkbox(L"Fruit column visible");
    column1_visible_checkbox_->SetChecked(true);
    column1_visible_checkbox_->set_listener(this);
    column2_visible_checkbox_ = new views::Checkbox(L"Color column visible");
    column2_visible_checkbox_->SetChecked(true);
    column2_visible_checkbox_->set_listener(this);
    column3_visible_checkbox_ = new views::Checkbox(L"Origin column visible");
    column3_visible_checkbox_->SetChecked(true);
    column3_visible_checkbox_->set_listener(this);
    column4_visible_checkbox_ = new views::Checkbox(L"Price column visible");
    column4_visible_checkbox_->SetChecked(true);
    column4_visible_checkbox_->set_listener(this);

    views::GridLayout* layout = new views::GridLayout(container);
    container->SetLayoutManager(layout);

    std::vector<TableColumn> columns;
    columns.push_back(TableColumn(0, ASCIIToUTF16("Fruit"), TableColumn::LEFT,
        100));
    columns.push_back(TableColumn(1, ASCIIToUTF16("Color"), TableColumn::LEFT,
        100));
    columns.push_back(TableColumn(2, ASCIIToUTF16("Origin"), TableColumn::LEFT,
        100));
    columns.push_back(TableColumn(3, ASCIIToUTF16("Price"), TableColumn::LEFT,
        100));
    const int options = (views::TableView2::SINGLE_SELECTION |
                         views::TableView2::RESIZABLE_COLUMNS |
                         views::TableView2::AUTOSIZE_COLUMNS |
                         views::TableView2::HORIZONTAL_LINES |
                         views::TableView2::VERTICAL_LINES);
    table_ = new views::TableView2(this, columns, views::ICON_AND_TEXT,
                                   options);
    table_->SetObserver(this);
    icon1.setConfig(SkBitmap::kARGB_8888_Config, 16, 16);
    icon1.allocPixels();
    SkCanvas canvas1(icon1);
    canvas1.drawColor(SK_ColorRED);

    icon2.setConfig(SkBitmap::kARGB_8888_Config, 16, 16);
    icon2.allocPixels();
    SkCanvas canvas2(icon2);
    canvas2.drawColor(SK_ColorBLUE);

    views::ColumnSet* column_set = layout->AddColumnSet(0);
    column_set->AddColumn(views::GridLayout::FILL, views::GridLayout::FILL, 1,
                          views::GridLayout::USE_PREF, 0, 0);
    layout->StartRow(1 /* expand */, 0);
    layout->AddView(table_);

    column_set = layout->AddColumnSet(1);
    column_set->AddColumn(views::GridLayout::FILL, views::GridLayout::FILL,
                          0.5f, views::GridLayout::USE_PREF, 0, 0);
    column_set->AddColumn(views::GridLayout::FILL, views::GridLayout::FILL,
                          0.5f, views::GridLayout::USE_PREF, 0, 0);
    column_set->AddColumn(views::GridLayout::FILL, views::GridLayout::FILL,
                          0.5f, views::GridLayout::USE_PREF, 0, 0);
    column_set->AddColumn(views::GridLayout::FILL, views::GridLayout::FILL,
                          0.5f, views::GridLayout::USE_PREF, 0, 0);

    layout->StartRow(0 /* no expand */, 1);

    layout->AddView(column1_visible_checkbox_);
    layout->AddView(column2_visible_checkbox_);
    layout->AddView(column3_visible_checkbox_);
    layout->AddView(column4_visible_checkbox_);
  }

  // TableModel implementation:
  virtual int RowCount() {
    return 10;
  }

  virtual string16 GetText(int row, int column_id) {
    const char* const cells[5][4] = {
      { "Orange", "Orange", "South america", "$5" },
      { "Apple", "Green", "Canada", "$3" },
      { "Blue berries", "Blue", "Mexico", "$10.3" },
      { "Strawberries", "Red", "California", "$7" },
      { "Cantaloupe", "Orange", "South america", "$5" }
    };
    return ASCIIToUTF16(cells[row % 5][column_id]);
  }

  virtual SkBitmap GetIcon(int row) {
    return row % 2 ? icon1 : icon2;
  }

  virtual void SetObserver(TableModelObserver* observer) {
  }

  // TableViewObserver implementation:
  virtual void OnSelectionChanged() {
    PrintStatus(L"Selection changed: %d",
                table_->GetFirstSelectedRow());
  }

  virtual void OnDoubleClick() {}

  virtual void OnMiddleClick() {}

  virtual void OnKeyDown(ui::KeyboardCode virtual_keycode) {}

  virtual void OnTableViewDelete(views::TableView* table_view) {}

  virtual void OnTableView2Delete(views::TableView2* table_view) {}

  // ButtonListener implementation:
  virtual void ButtonPressed(views::Button* sender, const views::Event& event) {
    int index = 0;
    bool show = true;
    if (sender == column1_visible_checkbox_) {
      index = 0;
      show = column1_visible_checkbox_->checked();
    } else if (sender == column2_visible_checkbox_) {
      index = 1;
      show = column2_visible_checkbox_->checked();
    } else if (sender == column3_visible_checkbox_) {
      index = 2;
      show = column3_visible_checkbox_->checked();
    } else if (sender == column4_visible_checkbox_) {
      index = 3;
      show = column4_visible_checkbox_->checked();
    }
    table_->SetColumnVisibility(index, show);
  }

 private:
  // The table to be tested.
  views::TableView2* table_;

  views::Checkbox* column1_visible_checkbox_;
  views::Checkbox* column2_visible_checkbox_;
  views::Checkbox* column3_visible_checkbox_;
  views::Checkbox* column4_visible_checkbox_;

  SkBitmap icon1;
  SkBitmap icon2;

  DISALLOW_COPY_AND_ASSIGN(Table2Example);
};

}  // namespace examples

#endif  // VIEWS_EXAMPLES_TABLE2_EXAMPLE_H_
