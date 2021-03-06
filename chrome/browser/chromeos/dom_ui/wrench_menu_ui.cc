// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/dom_ui/wrench_menu_ui.h"

#include "app/l10n_util.h"
#include "base/string_number_conversions.h"
#include "base/string_util.h"
#include "base/utf_string_conversions.h"
#include "base/values.h"
#include "base/weak_ptr.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/browser_list.h"
#include "chrome/browser/chromeos/views/domui_menu_widget.h"
#include "chrome/browser/chromeos/views/native_menu_domui.h"
#include "chrome/browser/tab_contents/tab_contents.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/notification_registrar.h"
#include "chrome/common/notification_source.h"
#include "chrome/common/url_constants.h"
#include "googleurl/src/gurl.h"
#include "grit/browser_resources.h"
#include "grit/generated_resources.h"
#include "views/controls/menu/menu_2.h"

namespace {

class WrenchMenuSourceDelegate : public chromeos::MenuSourceDelegate {
 public:
  virtual void AddCustomConfigValues(DictionaryValue* config) const {
    // Resources that are necessary to build wrench menu.
    config->SetInteger("IDC_CUT", IDC_CUT);
    config->SetInteger("IDC_COPY", IDC_COPY);
    config->SetInteger("IDC_PASTE", IDC_PASTE);
    config->SetInteger("IDC_ZOOM_MINUS", IDC_ZOOM_MINUS);
    config->SetInteger("IDC_ZOOM_PLUS", IDC_ZOOM_PLUS);
    config->SetInteger("IDC_FULLSCREEN", IDC_FULLSCREEN);

    config->SetString("IDS_EDIT2", WideToUTF8(l10n_util::GetString(IDS_EDIT2)));
    config->SetString("IDS_ZOOM_MENU2",
                      WideToUTF8(l10n_util::GetString(IDS_ZOOM_MENU2)));
    config->SetString("IDS_CUT", WideToUTF8(l10n_util::GetString(IDS_CUT)));
    config->SetString("IDS_COPY", WideToUTF8(l10n_util::GetString(IDS_COPY)));
    config->SetString("IDS_PASTE", WideToUTF8(l10n_util::GetString(IDS_PASTE)));
  }
};

}  // namespace

namespace chromeos {

////////////////////////////////////////////////////////////////////////////////
//
// WrenchMenuUI
//
////////////////////////////////////////////////////////////////////////////////

WrenchMenuUI::WrenchMenuUI(TabContents* contents)
    : chromeos::MenuUI(
        contents,
        ALLOW_THIS_IN_INITIALIZER_LIST(
            CreateMenuUIHTMLSource(new WrenchMenuSourceDelegate(),
                                   chrome::kChromeUIWrenchMenu,
                                   "WrenchMenu"  /* class name */,
                                   IDR_WRENCH_MENU_JS,
                                   IDR_WRENCH_MENU_CSS))) {
  registrar_.Add(this, NotificationType::ZOOM_LEVEL_CHANGED,
                 Source<Profile>(GetProfile()));
}

void WrenchMenuUI::ModelUpdated(const menus::MenuModel* new_model) {
  MenuUI::ModelUpdated(new_model);
  UpdateZoomControls();
}

void WrenchMenuUI::Observe(NotificationType type,
                           const NotificationSource& source,
                           const NotificationDetails& details) {
  DCHECK_EQ(NotificationType::ZOOM_LEVEL_CHANGED, type.value);
  UpdateZoomControls();
}

void WrenchMenuUI::UpdateZoomControls() {
  DOMUIMenuWidget* widget = DOMUIMenuWidget::FindDOMUIMenuWidget(
      tab_contents()->GetNativeView());
  if (!widget || !widget->is_root())
    return;
  Browser* browser = BrowserList::GetLastActive();
  if (!browser)
    return;
  TabContents* selected_tab = browser->GetSelectedTabContents();
  bool enable_increment = false;
  bool enable_decrement = false;
  int zoom = 100;
  if (selected_tab)
    zoom = selected_tab->GetZoomPercent(&enable_increment, &enable_decrement);

  DictionaryValue params;
  params.SetBoolean("plus", enable_increment);
  params.SetBoolean("minus", enable_decrement);
  params.SetString("percent", l10n_util::GetStringFUTF16(
      IDS_ZOOM_PERCENT, UTF8ToUTF16(base::IntToString(zoom))));
  CallJavascriptFunction(L"updateZoomControls", params);
}

views::Menu2* WrenchMenuUI::CreateMenu2(menus::MenuModel* model) {
  views::Menu2* menu = new views::Menu2(model);
  NativeMenuDOMUI::SetMenuURL(
      menu, GURL(StringPrintf("chrome://%s", chrome::kChromeUIWrenchMenu)));
  return menu;
}

}  // namespace chromeos
