// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef VIEWS_EXAMPLES_COMBOBOX_EXAMPLE_H_
#define VIEWS_EXAMPLES_COMBOBOX_EXAMPLE_H_
#pragma once

#include "base/string_util.h"
#include "base/utf_string_conversions.h"
#include "ui/base/models/combobox_model.h"
#include "views/controls/combobox/combobox.h"
#include "views/examples/example_base.h"
#include "views/fill_layout.h"

using ui::ComboboxModel;  // TODO(beng): remove

namespace examples {

// ComboboxExample
class ComboboxExample : public ExampleBase, public views::Combobox::Listener {
 public:
  explicit ComboboxExample(ExamplesMain* main) : ExampleBase(main) {
    combobox_ = new views::Combobox(new ComboboxModelExample());
    combobox_->set_listener(this);
    combobox_->SetSelectedItem(3);
  }
  virtual ~ComboboxExample() {}

  virtual std::wstring GetExampleTitle() {
    return L"Combo Box";
  }

  virtual void CreateExampleView(views::View* container) {
    container->SetLayoutManager(new views::FillLayout);
    container->AddChildView(combobox_);
  }

 private:
  // An sample combobox model that generates list of "Item <index>".
  class ComboboxModelExample : public ComboboxModel {
   public:
    ComboboxModelExample() {}
    virtual ~ComboboxModelExample() {}

    virtual int GetItemCount() {
      return 10;
    }

    virtual string16 GetItemAt(int index) {
      return WideToUTF16Hack(StringPrintf(L"Item %d", index));
    }

   private:
    DISALLOW_COPY_AND_ASSIGN(ComboboxModelExample);
  };

  // Lister implementation.
  virtual void ItemChanged(views::Combobox* combo_box,
                           int prev_index,
                           int new_index) {
    PrintStatus(L"Selected: index=%d, label=%ls",
                new_index, UTF16ToWideHack(
                    combo_box->model()->GetItemAt(new_index)).c_str());
  }

  // This test only control.
  views::Combobox* combobox_;

  DISALLOW_COPY_AND_ASSIGN(ComboboxExample);
};

}  // namespace examples

#endif  // VIEWS_EXAMPLES_COMBOBOX_EXAMPLE_H_
