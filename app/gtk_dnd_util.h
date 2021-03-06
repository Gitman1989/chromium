// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef APP_GTK_DND_UTIL_H_
#define APP_GTK_DND_UTIL_H_
#pragma once

#include <gtk/gtk.h>

#include <vector>

#include "base/string16.h"

class GURL;

namespace gtk_dnd_util {

// Registry of all internal int codes for drag and drop.
enum {
  // Intra-application types.
  CHROME_TAB = 1 << 0,
  CHROME_BOOKMARK_ITEM = 1 << 1,
  CHROME_WEBDROP_FILE_CONTENTS = 1 << 2,
  CHROME_NAMED_URL = 1 << 3,

  // Standard types.
  TEXT_PLAIN = 1 << 4,
  TEXT_URI_LIST = 1 << 5,
  TEXT_HTML = 1 << 6,

  // Other types.  NETSCAPE_URL is provided for compatibility with other
  // apps.
  NETSCAPE_URL = 1 << 7,

  // Used for drag-out download.
  TEXT_PLAIN_NO_CHARSET = 1 << 8,
  DIRECT_SAVE_FILE = 1 << 9,

  INVALID_TARGET = 1 << 10,
};

// Get the atom for a given target (of the above enum type). Will return NULL
// for non-custom targets, such as CHROME_TEXT_PLAIN.
GdkAtom GetAtomForTarget(int target);

// Creates a target list from the given mask. The mask should be an OR of
// CHROME_* values. The target list is returned with ref count 1; the caller
// is responsible for calling gtk_target_list_unref() when it is no longer
// needed.
// Since the MIME type for WEBDROP_FILE_CONTENTS depends on the file's
// contents, that flag is ignored by this function. It is the responsibility
// of the client code to do the right thing.
GtkTargetList* GetTargetListFromCodeMask(int code_mask);

// Set the drag target list for |source| with the target list that
// corresponds to |code_mask|.
void SetSourceTargetListFromCodeMask(GtkWidget* source, int code_mask);

// Set the accepted targets list for |dest|. The |target_codes| array should
// be sorted in preference order and should be terminated with -1.
void SetDestTargetList(GtkWidget* dest, const int* target_codes);

// Write a URL to the selection in the given type.
void WriteURLWithName(GtkSelectionData* selection_data,
                      const GURL& url,
                      string16 title,
                      int type);

// Extracts data of type CHROME_NAMED_URL from |selection_data| into
// |url| and |title|. Returns true if the url/title were safely extracted
// and the url is valid.
bool ExtractNamedURL(GtkSelectionData* selection_data,
                     GURL* url,
                     string16* title);

// Extracts data of type TEXT_URI_LIST from |selection_data| into |urls|.
bool ExtractURIList(GtkSelectionData* selection_data,
                    std::vector<GURL>* urls);

// Extracts a Netscape URL (url\ntitle) from |selection_data|.
bool ExtractNetscapeURL(GtkSelectionData* selection_data,
                        GURL* url,
                        string16* title);

}  // namespace gtk_dnd_util

#endif  // APP_GTK_DND_UTIL_H_
