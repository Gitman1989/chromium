// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/automation/tab_proxy.h"

#include <algorithm>

#include "base/logging.h"
#include "base/string16.h"
#include "base/utf_string_conversions.h"
#include "chrome/common/automation_messages.h"
#include "chrome/common/json_value_serializer.h"
#include "chrome/test/automation/automation_proxy.h"
#include "googleurl/src/gurl.h"

TabProxy::TabProxy(AutomationMessageSender* sender,
                   AutomationHandleTracker* tracker,
                   int handle)
    : AutomationResourceProxy(tracker, sender, handle) {
}


bool TabProxy::GetTabTitle(std::wstring* title) const {
  if (!is_valid())
    return false;

  if (!title) {
    NOTREACHED();
    return false;
  }

  int tab_title_size_response = 0;

  bool succeeded = sender_->Send(
      new AutomationMsg_TabTitle(0, handle_, &tab_title_size_response, title));
  return succeeded;
}

bool TabProxy::GetTabIndex(int* index) const {
  if (!is_valid())
    return false;

  if (!index) {
    NOTREACHED();
    return false;
  }

  return sender_->Send(new AutomationMsg_TabIndex(0, handle_, index));
}

int TabProxy::FindInPage(const std::wstring& search_string,
                         FindInPageDirection forward,
                         FindInPageCase match_case,
                         bool find_next,
                         int* ordinal) {
  if (!is_valid())
    return -1;

  AutomationMsg_Find_Params params;
  params.unused = 0;
  params.search_string = WideToUTF16Hack(search_string);
  params.find_next = find_next;
  params.match_case = (match_case == CASE_SENSITIVE);
  params.forward = (forward == FWD);

  int matches = 0;
  int ordinal2 = 0;
  bool succeeded = sender_->Send(new AutomationMsg_Find(0, handle_,
                                                        params,
                                                        &ordinal2,
                                                        &matches));
  if (!succeeded)
    return -1;
  if (ordinal)
    *ordinal = ordinal2;
  return matches;
}

AutomationMsg_NavigationResponseValues TabProxy::NavigateToURL(
    const GURL& url) {
  return NavigateToURLBlockUntilNavigationsComplete(url, 1);
}

AutomationMsg_NavigationResponseValues
    TabProxy::NavigateToURLBlockUntilNavigationsComplete(
        const GURL& url, int number_of_navigations) {
  if (!is_valid())
    return AUTOMATION_MSG_NAVIGATION_ERROR;

  AutomationMsg_NavigationResponseValues navigate_response =
      AUTOMATION_MSG_NAVIGATION_ERROR;

  if (number_of_navigations == 1) {
    // TODO(phajdan.jr): Remove when the reference build gets updated.
    // This is only for backwards compatibility.
    sender_->Send(new AutomationMsg_NavigateToURL(0, handle_, url,
                                                  &navigate_response));
  } else {
    sender_->Send(new AutomationMsg_NavigateToURLBlockUntilNavigationsComplete(
        0, handle_, url, number_of_navigations, &navigate_response));
  }

  return navigate_response;
}

bool TabProxy::SetAuth(const std::wstring& username,
                       const std::wstring& password) {
  if (!is_valid())
    return false;

  AutomationMsg_NavigationResponseValues navigate_response =
      AUTOMATION_MSG_NAVIGATION_ERROR;
  sender_->Send(new AutomationMsg_SetAuth(0, handle_, username, password,
                                          &navigate_response));
  return navigate_response == AUTOMATION_MSG_NAVIGATION_SUCCESS;
}

bool TabProxy::CancelAuth() {
  if (!is_valid())
    return false;

  AutomationMsg_NavigationResponseValues navigate_response =
      AUTOMATION_MSG_NAVIGATION_ERROR;
  sender_->Send(new AutomationMsg_CancelAuth(0, handle_, &navigate_response));
  return navigate_response == AUTOMATION_MSG_NAVIGATION_SUCCESS;
}

bool TabProxy::NeedsAuth() const {
  if (!is_valid())
    return false;

  bool needs_auth = false;
  sender_->Send(new AutomationMsg_NeedsAuth(0, handle_, &needs_auth));
  return needs_auth;
}

AutomationMsg_NavigationResponseValues TabProxy::GoBack() {
  return GoBackBlockUntilNavigationsComplete(1);
}

AutomationMsg_NavigationResponseValues
    TabProxy::GoBackBlockUntilNavigationsComplete(int number_of_navigations) {
  if (!is_valid())
    return AUTOMATION_MSG_NAVIGATION_ERROR;

  AutomationMsg_NavigationResponseValues navigate_response =
      AUTOMATION_MSG_NAVIGATION_ERROR;
  sender_->Send(new AutomationMsg_GoBackBlockUntilNavigationsComplete(
      0, handle_, number_of_navigations, &navigate_response));
  return navigate_response;
}

AutomationMsg_NavigationResponseValues TabProxy::GoForward() {
  return GoForwardBlockUntilNavigationsComplete(1);
}

AutomationMsg_NavigationResponseValues
    TabProxy::GoForwardBlockUntilNavigationsComplete(
        int number_of_navigations) {
  if (!is_valid())
    return AUTOMATION_MSG_NAVIGATION_ERROR;

  AutomationMsg_NavigationResponseValues navigate_response =
      AUTOMATION_MSG_NAVIGATION_ERROR;
  sender_->Send(new AutomationMsg_GoForwardBlockUntilNavigationsComplete(
      0, handle_, number_of_navigations, &navigate_response));
  return navigate_response;
}

AutomationMsg_NavigationResponseValues TabProxy::Reload() {
  if (!is_valid())
    return AUTOMATION_MSG_NAVIGATION_ERROR;

  AutomationMsg_NavigationResponseValues navigate_response =
      AUTOMATION_MSG_NAVIGATION_ERROR;
  sender_->Send(new AutomationMsg_Reload(0, handle_, &navigate_response));
  return navigate_response;
}

bool TabProxy::GetRedirectsFrom(const GURL& source_url,
                                std::vector<GURL>* redirects) {
  bool succeeded = false;
  sender_->Send(new AutomationMsg_RedirectsFrom(0, handle_,
                                                source_url,
                                                &succeeded,
                                                redirects));
  return succeeded;
}

bool TabProxy::GetCurrentURL(GURL* url) const {
  if (!is_valid())
    return false;

  if (!url) {
    NOTREACHED();
    return false;
  }

  bool succeeded = false;
  sender_->Send(new AutomationMsg_TabURL(0, handle_, &succeeded, url));
  return succeeded;
}

bool TabProxy::NavigateToURLAsync(const GURL& url) {
  if (!is_valid())
    return false;

  bool status = false;
  sender_->Send(new AutomationMsg_NavigationAsync(0,
                                                  handle_,
                                                  url,
                                                  &status));
  return status;
}

bool TabProxy::NavigateToURLAsyncWithDisposition(
    const GURL& url,
    WindowOpenDisposition disposition) {
  if (!is_valid())
    return false;

  bool status = false;
  sender_->Send(new AutomationMsg_NavigationAsyncWithDisposition(0,
                                                                 handle_,
                                                                 url,
                                                                 disposition,
                                                                 &status));
  return status;
}

bool TabProxy::GetProcessID(int* process_id) const {
  if (!is_valid())
    return false;

  if (!process_id) {
    NOTREACHED();
    return false;
  }

  return sender_->Send(new AutomationMsg_TabProcessID(0, handle_, process_id));
}

bool TabProxy::ExecuteAndExtractString(const std::wstring& frame_xpath,
                                       const std::wstring& jscript,
                                       std::wstring* string_value) {
  Value* root = NULL;
  bool succeeded = ExecuteAndExtractValue(frame_xpath, jscript, &root);
  if (!succeeded)
    return false;

  DCHECK(root->IsType(Value::TYPE_LIST));
  Value* value = NULL;
  succeeded = static_cast<ListValue*>(root)->Get(0, &value);
  if (succeeded) {
    string16 read_value;
    succeeded = value->GetAsString(&read_value);
    if (succeeded) {
      // TODO(viettrungluu): remove conversion. (But should |jscript| be UTF-8?)
      *string_value = UTF16ToWideHack(read_value);
    }
  }

  delete root;
  return succeeded;
}

bool TabProxy::ExecuteAndExtractBool(const std::wstring& frame_xpath,
                                     const std::wstring& jscript,
                                     bool* bool_value) {
  Value* root = NULL;
  bool succeeded = ExecuteAndExtractValue(frame_xpath, jscript, &root);
  if (!succeeded)
    return false;

  bool read_value = false;
  DCHECK(root->IsType(Value::TYPE_LIST));
  Value* value = NULL;
  succeeded = static_cast<ListValue*>(root)->Get(0, &value);
  if (succeeded) {
    succeeded = value->GetAsBoolean(&read_value);
    if (succeeded) {
      *bool_value = read_value;
    }
  }

  delete value;
  return succeeded;
}

bool TabProxy::ExecuteAndExtractInt(const std::wstring& frame_xpath,
                                    const std::wstring& jscript,
                                    int* int_value) {
  Value* root = NULL;
  bool succeeded = ExecuteAndExtractValue(frame_xpath, jscript, &root);
  if (!succeeded)
    return false;

  int read_value = 0;
  DCHECK(root->IsType(Value::TYPE_LIST));
  Value* value = NULL;
  succeeded = static_cast<ListValue*>(root)->Get(0, &value);
  if (succeeded) {
    succeeded = value->GetAsInteger(&read_value);
    if (succeeded) {
      *int_value = read_value;
    }
  }

  delete value;
  return succeeded;
}

bool TabProxy::ExecuteAndExtractValue(const std::wstring& frame_xpath,
                                      const std::wstring& jscript,
                                      Value** value) {
  if (!is_valid())
    return false;

  if (!value) {
    NOTREACHED();
    return false;
  }

  std::string json;
  if (!sender_->Send(new AutomationMsg_DomOperation(0, handle_, frame_xpath,
                                                    jscript, &json)))
    return false;
  // Wrap |json| in an array before deserializing because valid JSON has an
  // array or an object as the root.
  json.insert(0, "[");
  json.append("]");

  JSONStringValueSerializer deserializer(json);
  *value = deserializer.Deserialize(NULL, NULL);
  return *value != NULL;
}

DOMElementProxyRef TabProxy::GetDOMDocument() {
  if (!is_valid())
    return NULL;

  int element_handle;
  if (!ExecuteJavaScriptAndGetReturn("document", &element_handle))
    return NULL;
  return GetObjectProxy<DOMElementProxy>(element_handle);
}

bool TabProxy::SetEnableExtensionAutomation(
    const std::vector<std::string>& functions_enabled) {
  if (!is_valid())
    return false;

  return sender_->Send(new AutomationMsg_SetEnableExtensionAutomation(
      0, handle_, functions_enabled));
}

bool TabProxy::GetConstrainedWindowCount(int* count) const {
  if (!is_valid())
    return false;

  if (!count) {
    NOTREACHED();
    return false;
  }

  return sender_->Send(new AutomationMsg_ConstrainedWindowCount(
      0, handle_, count));
}

bool TabProxy::WaitForChildWindowCountToChange(int count, int* new_count,
                                               int wait_timeout) {
  int intervals = std::max(wait_timeout / automation::kSleepTime, 1);
  for (int i = 0; i < intervals; ++i) {
    PlatformThread::Sleep(automation::kSleepTime);
    bool succeeded = GetConstrainedWindowCount(new_count);
    if (!succeeded)
      return false;
    if (count != *new_count)
      return true;
  }
  // Constrained Window count did not change, return false.
  return false;
}

bool TabProxy::GetBlockedPopupCount(int* count) const {
  if (!is_valid())
    return false;

  if (!count) {
    NOTREACHED();
    return false;
  }

  return sender_->Send(new AutomationMsg_BlockedPopupCount(
      0, handle_, count));
}

bool TabProxy::WaitForBlockedPopupCountToChangeTo(int target_count,
                                                  int wait_timeout) {
  int intervals = std::max(wait_timeout / automation::kSleepTime, 1);
  for (int i = 0; i < intervals; ++i) {
    PlatformThread::Sleep(automation::kSleepTime);
    int new_count = -1;
    bool succeeded = GetBlockedPopupCount(&new_count);
    if (!succeeded)
      return false;
    if (target_count == new_count)
      return true;
  }
  // Constrained Window count did not change, return false.
  return false;
}

bool TabProxy::GetCookies(const GURL& url, std::string* cookies) {
  if (!is_valid())
    return false;

  int size = 0;
  return sender_->Send(new AutomationMsg_GetCookies(0, url, handle_, &size,
                                                    cookies));
}

bool TabProxy::GetCookieByName(const GURL& url,
                               const std::string& name,
                               std::string* cookie) {
  std::string cookies;
  if (!GetCookies(url, &cookies))
    return false;

  std::string namestr = name + "=";
  std::string::size_type idx = cookies.find(namestr);
  if (idx != std::string::npos) {
    cookies.erase(0, idx + namestr.length());
    *cookie = cookies.substr(0, cookies.find(";"));
  } else {
    cookie->clear();
  }

  return true;
}

bool TabProxy::SetCookie(const GURL& url, const std::string& value) {
  int response_value = 0;
  return sender_->Send(new AutomationMsg_SetCookie(0, url, value, handle_,
                                                   &response_value));
}

bool TabProxy::DeleteCookie(const GURL& url, const std::string& name) {
  bool succeeded;
  sender_->Send(new AutomationMsg_DeleteCookie(0, url, name, handle_,
                                               &succeeded));
  return succeeded;
}

bool TabProxy::ShowCollectedCookiesDialog() {
  bool succeeded = false;
  return sender_->Send(new AutomationMsg_ShowCollectedCookiesDialog(
                       0, handle_, &succeeded)) && succeeded;
}

int TabProxy::InspectElement(int x, int y) {
  if (!is_valid())
    return -1;

  int ret = -1;
  sender_->Send(new AutomationMsg_InspectElement(0, handle_, x, y, &ret));
  return ret;
}

bool TabProxy::GetDownloadDirectory(FilePath* directory) {
  DCHECK(directory);
  if (!is_valid())
    return false;

  return sender_->Send(new AutomationMsg_DownloadDirectory(0, handle_,
                                                           directory));
}

bool TabProxy::ShowInterstitialPage(const std::string& html_text) {
  if (!is_valid())
    return false;

  AutomationMsg_NavigationResponseValues result =
      AUTOMATION_MSG_NAVIGATION_ERROR;
  if (!sender_->Send(new AutomationMsg_ShowInterstitialPage(
                         0, handle_, html_text, &result))) {
    return false;
  }

  return result == AUTOMATION_MSG_NAVIGATION_SUCCESS;
}

bool TabProxy::HideInterstitialPage() {
  if (!is_valid())
    return false;

  bool result = false;
  sender_->Send(new AutomationMsg_HideInterstitialPage(0, handle_, &result));
  return result;
}

bool TabProxy::Close() {
  return Close(false);
}

bool TabProxy::Close(bool wait_until_closed) {
  if (!is_valid())
    return false;

  bool succeeded = false;
  sender_->Send(new AutomationMsg_CloseTab(0, handle_, wait_until_closed,
                                           &succeeded));
  return succeeded;
}

#if defined(OS_WIN)
bool TabProxy::ProcessUnhandledAccelerator(const MSG& msg) {
  if (!is_valid())
    return false;

  return sender_->Send(
      new AutomationMsg_ProcessUnhandledAccelerator(0, handle_, msg));
  // This message expects no response
}

bool TabProxy::SetInitialFocus(bool reverse, bool restore_focus_to_view) {
  if (!is_valid())
    return false;

  return sender_->Send(
      new AutomationMsg_SetInitialFocus(0, handle_, reverse,
                                        restore_focus_to_view));
  // This message expects no response
}

AutomationMsg_NavigationResponseValues TabProxy::NavigateInExternalTab(
    const GURL& url, const GURL& referrer) {
  if (!is_valid())
    return AUTOMATION_MSG_NAVIGATION_ERROR;

  AutomationMsg_NavigationResponseValues rv = AUTOMATION_MSG_NAVIGATION_ERROR;
  sender_->Send(new AutomationMsg_NavigateInExternalTab(0, handle_, url,
                                                        referrer, &rv));
  return rv;
}

AutomationMsg_NavigationResponseValues TabProxy::NavigateExternalTabAtIndex(
    int index) {
  if (!is_valid())
    return AUTOMATION_MSG_NAVIGATION_ERROR;

  AutomationMsg_NavigationResponseValues rv = AUTOMATION_MSG_NAVIGATION_ERROR;
  sender_->Send(new AutomationMsg_NavigateExternalTabAtIndex(0, handle_, index,
                                                             &rv));
  return rv;
}

void TabProxy::HandleMessageFromExternalHost(const std::string& message,
                                             const std::string& origin,
                                             const std::string& target) {
  if (!is_valid())
    return;

  sender_->Send(
      new AutomationMsg_HandleMessageFromExternalHost(
          0, handle_, message, origin, target));
}
#endif  // defined(OS_WIN)

bool TabProxy::WaitForTabToBeRestored(uint32 timeout_ms) {
  if (!is_valid())
    return false;
  return sender_->Send(new AutomationMsg_WaitForTabToBeRestored(0, handle_));
}

bool TabProxy::GetSecurityState(SecurityStyle* security_style,
                                int* ssl_cert_status,
                                int* insecure_content_status) {
  DCHECK(security_style && ssl_cert_status && insecure_content_status);

  if (!is_valid())
    return false;

  bool succeeded = false;

  sender_->Send(new AutomationMsg_GetSecurityState(
      0, handle_, &succeeded, security_style, ssl_cert_status,
      insecure_content_status));

  return succeeded;
}

bool TabProxy::GetPageType(PageType* type) {
  DCHECK(type);

  if (!is_valid())
    return false;

  bool succeeded = false;
  sender_->Send(new AutomationMsg_GetPageType(0, handle_, &succeeded, type));
  return succeeded;
}

bool TabProxy::TakeActionOnSSLBlockingPage(bool proceed) {
  if (!is_valid())
    return false;

  AutomationMsg_NavigationResponseValues result =
      AUTOMATION_MSG_NAVIGATION_ERROR;
  sender_->Send(new AutomationMsg_ActionOnSSLBlockingPage(0, handle_, proceed,
                                                          &result));
  return result == AUTOMATION_MSG_NAVIGATION_SUCCESS ||
      result == AUTOMATION_MSG_NAVIGATION_AUTH_NEEDED;
}

bool TabProxy::PrintNow() {
  if (!is_valid())
    return false;

  bool succeeded = false;
  sender_->Send(new AutomationMsg_PrintNow(0, handle_, &succeeded));
  return succeeded;
}

bool TabProxy::PrintAsync() {
  if (!is_valid())
    return false;

  return sender_->Send(new AutomationMsg_PrintAsync(0, handle_));
}

bool TabProxy::SavePage(const FilePath& file_name,
                        const FilePath& dir_path,
                        SavePackage::SavePackageType type) {
  if (!is_valid())
    return false;

  bool succeeded = false;
  sender_->Send(new AutomationMsg_SavePage(0, handle_, file_name, dir_path,
                                           static_cast<int>(type),
                                           &succeeded));
  return succeeded;
}

bool TabProxy::GetInfoBarCount(int* count) {
  if (!is_valid())
    return false;

  return sender_->Send(new AutomationMsg_GetInfoBarCount(0, handle_, count));
}

bool TabProxy::WaitForInfoBarCount(int target_count) {
  if (!is_valid())
    return false;

  bool success = false;
  return sender_->Send(new AutomationMsg_WaitForInfoBarCount(
                           0, handle_, target_count, &success)) && success;
}

bool TabProxy::ClickInfoBarAccept(int info_bar_index,
                                  bool wait_for_navigation) {
  if (!is_valid())
    return false;

  AutomationMsg_NavigationResponseValues result =
      AUTOMATION_MSG_NAVIGATION_ERROR;
  sender_->Send(new AutomationMsg_ClickInfoBarAccept(
      0, handle_, info_bar_index, wait_for_navigation, &result));
  return result == AUTOMATION_MSG_NAVIGATION_SUCCESS ||
      result == AUTOMATION_MSG_NAVIGATION_AUTH_NEEDED;
}

bool TabProxy::GetLastNavigationTime(int64* nav_time) {
  if (!is_valid())
    return false;

  bool success = false;
  success = sender_->Send(new AutomationMsg_GetLastNavigationTime(
      0, handle_, nav_time));
  return success;
}

bool TabProxy::WaitForNavigation(int64 last_navigation_time) {
  if (!is_valid())
    return false;

  AutomationMsg_NavigationResponseValues result =
      AUTOMATION_MSG_NAVIGATION_ERROR;
  sender_->Send(new AutomationMsg_WaitForNavigation(0, handle_,
                                                    last_navigation_time,
                                                    &result));
  return result == AUTOMATION_MSG_NAVIGATION_SUCCESS ||
      result == AUTOMATION_MSG_NAVIGATION_AUTH_NEEDED;
}

bool TabProxy::GetPageCurrentEncoding(std::string* encoding) {
  if (!is_valid())
    return false;

  bool succeeded = sender_->Send(
      new AutomationMsg_GetPageCurrentEncoding(0, handle_, encoding));
  return succeeded;
}

bool TabProxy::OverrideEncoding(const std::string& encoding) {
  if (!is_valid())
    return false;

  bool succeeded = false;
  sender_->Send(new AutomationMsg_OverrideEncoding(0, handle_, encoding,
                                                   &succeeded));
  return succeeded;
}

bool TabProxy::LoadBlockedPlugins() {
  if (!is_valid())
    return false;

  bool succeeded = false;
  sender_->Send(new AutomationMsg_LoadBlockedPlugins(0, handle_, &succeeded));
  return succeeded;
}

bool TabProxy::CaptureEntirePageAsPNG(const FilePath& path) {
  if (!is_valid())
    return false;

  bool succeeded = false;
  sender_->Send(new AutomationMsg_CaptureEntirePageAsPNG(0, handle_, path,
                                                         &succeeded));
  return succeeded;
}

#if defined(OS_WIN)
void TabProxy::Reposition(HWND window, HWND window_insert_after, int left,
                          int top, int width, int height, int flags,
                          HWND parent_window) {
  IPC::Reposition_Params params = {0};
  params.window = window;
  params.window_insert_after = window_insert_after;
  params.left = left;
  params.top = top;
  params.width = width;
  params.height = height;
  params.flags = flags;
  params.set_parent = (::IsWindow(parent_window) ? true : false);
  params.parent_window = parent_window;
  sender_->Send(new AutomationMsg_TabReposition(0, handle_, params));
}

void TabProxy::SendContextMenuCommand(int selected_command) {
  sender_->Send(new AutomationMsg_ForwardContextMenuCommandToChrome(
      0, handle_, selected_command));
}

void TabProxy::OnHostMoved() {
  sender_->Send(new AutomationMsg_BrowserMove(0, handle_));
}
#endif  // defined(OS_WIN)

void TabProxy::SelectAll() {
  sender_->Send(new AutomationMsg_SelectAll(0, handle_));
}

void TabProxy::Cut() {
  sender_->Send(new AutomationMsg_Cut(0, handle_));
}

void TabProxy::Copy() {
  sender_->Send(new AutomationMsg_Copy(0, handle_));
}

void TabProxy::Paste() {
  sender_->Send(new AutomationMsg_Paste(0, handle_));
}

void TabProxy::ReloadAsync() {
  sender_->Send(new AutomationMsg_ReloadAsync(0, handle_));
}

void TabProxy::StopAsync() {
  sender_->Send(new AutomationMsg_StopAsync(0, handle_));
}

void TabProxy::SaveAsAsync() {
  sender_->Send(new AutomationMsg_SaveAsAsync(0, handle_));
}

void TabProxy::AddObserver(TabProxyDelegate* observer) {
  AutoLock lock(list_lock_);
  observers_list_.AddObserver(observer);
}

void TabProxy::RemoveObserver(TabProxyDelegate* observer) {
  AutoLock lock(list_lock_);
  observers_list_.RemoveObserver(observer);
}

// Called on Channel background thread, if TabMessages filter is installed.
void TabProxy::OnMessageReceived(const IPC::Message& message) {
  AutoLock lock(list_lock_);
  FOR_EACH_OBSERVER(TabProxyDelegate, observers_list_,
                    OnMessageReceived(this, message));
}

void TabProxy::OnChannelError() {
  AutoLock lock(list_lock_);
  FOR_EACH_OBSERVER(TabProxyDelegate, observers_list_, OnChannelError(this));
}

TabProxy::~TabProxy() {}

bool TabProxy::ExecuteJavaScriptAndGetJSON(const std::string& script,
                                           std::string* json) {
  if (!is_valid())
    return false;
  if (!json) {
    NOTREACHED();
    return false;
  }
  return sender_->Send(new AutomationMsg_DomOperation(0, handle_, L"",
                                                      UTF8ToWide(script),
                                                      json));
}

void TabProxy::FirstObjectAdded() {
  AddRef();
}

void TabProxy::LastObjectRemoved() {
  Release();
}
