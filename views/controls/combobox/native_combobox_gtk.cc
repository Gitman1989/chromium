// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "views/controls/combobox/native_combobox_gtk.h"

#include <gtk/gtk.h>

#include <algorithm>

#include "app/combobox_model.h"
#include "base/utf_string_conversions.h"
#include "views/controls/combobox/combobox.h"

namespace views {

////////////////////////////////////////////////////////////////////////////////
// NativeComboboxGtk, public:

NativeComboboxGtk::NativeComboboxGtk(Combobox* combobox)
    : combobox_(combobox) {
  set_focus_view(combobox);
}

NativeComboboxGtk::~NativeComboboxGtk() {
}

////////////////////////////////////////////////////////////////////////////////
// NativeComboboxGtk, NativeComboboxWrapper implementation:

void NativeComboboxGtk::UpdateFromModel() {
  if (!native_view())
    return;

  preferred_size_ = gfx::Size();

  GtkListStore* store =
      GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(native_view())));
  ComboboxModel* model = combobox_->model();
  int count = model->GetItemCount();
  gtk_list_store_clear(store);
  GtkTreeIter iter;
  while (count-- > 0) {
    gtk_list_store_prepend(store, &iter);
    gtk_list_store_set(store, &iter,
                       0, UTF16ToUTF8(model->GetItemAt(count)).c_str(),
                       -1);
  }
}

void NativeComboboxGtk::UpdateSelectedItem() {
  if (!native_view())
    return;
  gtk_combo_box_set_active(
      GTK_COMBO_BOX(native_view()), combobox_->selected_item());
}

void NativeComboboxGtk::UpdateEnabled() {
  SetEnabled(combobox_->IsEnabled());
}

int NativeComboboxGtk::GetSelectedItem() const {
  if (!native_view())
    return 0;
  return gtk_combo_box_get_active(GTK_COMBO_BOX(native_view()));
}

bool NativeComboboxGtk::IsDropdownOpen() const {
  if (!native_view())
    return false;
  gboolean popup_shown;
  g_object_get(G_OBJECT(native_view()), "popup-shown", &popup_shown, NULL);
  return popup_shown;
}

gfx::Size NativeComboboxGtk::GetPreferredSize() {
  if (!native_view())
    return gfx::Size();

  if (preferred_size_.IsEmpty()) {
    GtkRequisition size_request = { 0, 0 };
    gtk_widget_size_request(native_view(), &size_request);
    // TODO(oshima|scott): we may not need ::max to 29. revisit this.
    preferred_size_.SetSize(size_request.width,
                            std::max(size_request.height, 29));
  }
  return preferred_size_;
}

View* NativeComboboxGtk::GetView() {
  return this;
}

void NativeComboboxGtk::SetFocus() {
  Focus();
}

gfx::NativeView NativeComboboxGtk::GetTestingHandle() const {
  return native_view();
}


////////////////////////////////////////////////////////////////////////////////
// NativeComboboxGtk, NativeControlGtk overrides:
void NativeComboboxGtk::CreateNativeControl() {
  GtkListStore* store = gtk_list_store_new(1, G_TYPE_STRING);
  GtkWidget* widget = gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));
  g_object_unref(G_OBJECT(store));

  GtkCellRenderer* cell = gtk_cell_renderer_text_new();
  gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(widget), cell, TRUE);
  gtk_cell_layout_set_attributes(
      GTK_CELL_LAYOUT(widget), cell, "text", 0, NULL);
  g_signal_connect(widget, "changed",
                   G_CALLBACK(CallChangedThunk), this);

  NativeControlCreated(widget);
}

void NativeComboboxGtk::NativeControlCreated(GtkWidget* native_control) {
  NativeControlGtk::NativeControlCreated(native_control);
  // Set the initial state of the combobox.
  UpdateFromModel();
  UpdateEnabled();
  UpdateSelectedItem();
}

////////////////////////////////////////////////////////////////////////////////
// NativeComboboxGtk, private:
void NativeComboboxGtk::SelectionChanged() {
  combobox_->SelectionChanged();
}

void NativeComboboxGtk::CallChanged(GtkWidget* widget) {
  SelectionChanged();
}

////////////////////////////////////////////////////////////////////////////////
// NativeComboboxWrapper, public:

// static
NativeComboboxWrapper* NativeComboboxWrapper::CreateWrapper(
    Combobox* combobox) {
  return new NativeComboboxGtk(combobox);
}

}  // namespace views
