// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_TRANSLATE_OPTIONS_MENU_MODEL_H_
#define CHROME_BROWSER_TRANSLATE_OPTIONS_MENU_MODEL_H_
#pragma once

#include "app/menus/simple_menu_model.h"

class TranslateInfoBarDelegate;

// A menu model that builds the contents of the options menu in the translate
// infobar. This menu has only one level (no submenus).
class OptionsMenuModel : public menus::SimpleMenuModel,
                         public menus::SimpleMenuModel::Delegate {
 public:
  explicit OptionsMenuModel(TranslateInfoBarDelegate* translate_delegate);
  virtual ~OptionsMenuModel();

  // menus::SimpleMenuModel::Delegate implementation:
  virtual bool IsCommandIdChecked(int command_id) const;
  virtual bool IsCommandIdEnabled(int command_id) const;
  virtual bool GetAcceleratorForCommandId(int command_id,
                                          menus::Accelerator* accelerator);
  virtual void ExecuteCommand(int command_id);

 private:
  TranslateInfoBarDelegate* translate_infobar_delegate_;

  DISALLOW_COPY_AND_ASSIGN(OptionsMenuModel);
};

#endif  // CHROME_BROWSER_TRANSLATE_OPTIONS_MENU_MODEL_H_
