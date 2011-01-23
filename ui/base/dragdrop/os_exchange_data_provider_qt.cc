// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/dragdrop/os_exchange_data_provider_gtk.h"

#include <algorithm>

#include "base/file_path.h"
#include "base/utf_string_conversions.h"
#include "net/base/net_util.h"
#include "ui/base/dragdrop/gtk_dnd_util.h"

namespace ui {

OSExchangeDataProviderGtk::OSExchangeDataProviderGtk(
    int known_formats,
    const std::set<GdkAtom>& known_custom_formats)
    : known_formats_(known_formats),
      known_custom_formats_(known_custom_formats),
      formats_(0),
      drag_image_(NULL) {
}

OSExchangeDataProviderGtk::OSExchangeDataProviderGtk()
    : known_formats_(0),
      formats_(0),
      drag_image_(NULL) {
}

OSExchangeDataProviderGtk::~OSExchangeDataProviderGtk() {

}

bool OSExchangeDataProviderGtk::HasDataForAllFormats(
    int formats,
    const std::set<GdkAtom>& custom_formats) const {
   return false;
}

GtkTargetList* OSExchangeDataProviderGtk::GetTargetList() const {

  return NULL;
}

void OSExchangeDataProviderGtk::WriteFormatToSelection(
    int format,
    GtkSelectionData* selection) const {

}

void OSExchangeDataProviderGtk::SetString(const std::wstring& data) {

}

void OSExchangeDataProviderGtk::SetURL(const GURL& url,
                                       const std::wstring& title) {

}

void OSExchangeDataProviderGtk::SetFilename(const std::wstring& full_path) {

}

void OSExchangeDataProviderGtk::SetPickledData(GdkAtom format,
                                               const Pickle& data) {

}

bool OSExchangeDataProviderGtk::GetString(std::wstring* data) const {

  return false;
}

bool OSExchangeDataProviderGtk::GetURLAndTitle(GURL* url,
                                               std::wstring* title) const {
  return false;
}

bool OSExchangeDataProviderGtk::GetFilename(std::wstring* full_path) const {

  return false;
}

bool OSExchangeDataProviderGtk::GetPickledData(GdkAtom format,
                                               Pickle* data) const {
 return false;
}

bool OSExchangeDataProviderGtk::HasString() const {
  return false;
}

bool OSExchangeDataProviderGtk::HasURL() const {
  return false;
}

bool OSExchangeDataProviderGtk::HasFile() const {
  return false;
  }

bool OSExchangeDataProviderGtk::HasCustomFormat(GdkAtom format) const {
  return false;
}

bool OSExchangeDataProviderGtk::GetPlainTextURL(GURL* url) const {

  return false;
}

void OSExchangeDataProviderGtk::SetDragImage(GdkPixbuf* drag_image,
                                             const gfx::Point& cursor_offset) {

}

///////////////////////////////////////////////////////////////////////////////
// OSExchangeData, public:

// static
OSExchangeData::Provider* OSExchangeData::CreateProvider() {
  return NULL;
}

GdkAtom OSExchangeData::RegisterCustomFormat(const std::string& type) {
  //return gdk_atom_intern(type.c_str(), false);
  return NULL;
}

}  // namespace ui

