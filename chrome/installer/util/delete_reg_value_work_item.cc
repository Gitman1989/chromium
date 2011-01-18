// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/util/delete_reg_value_work_item.h"

#include "base/logging.h"
#include "base/win/registry.h"
#include "chrome/installer/util/logging_installer.h"

using base::win::RegKey;

DeleteRegValueWorkItem::DeleteRegValueWorkItem(HKEY predefined_root,
                                               const std::wstring& key_path,
                                               const std::wstring& value_name,
                                               DWORD type)
    : predefined_root_(predefined_root),
      key_path_(key_path),
      value_name_(value_name),
      type_(type),
      status_(DELETE_VALUE),
      old_dw_(0),
      old_qword_(0) {
  DCHECK(type_ == REG_QWORD || type_ == REG_DWORD || type == REG_SZ);
}

DeleteRegValueWorkItem::~DeleteRegValueWorkItem() {
}

bool DeleteRegValueWorkItem::Do() {
  if (status_ != DELETE_VALUE) {
    // we already did something.
    LOG(ERROR) << "multiple calls to Do()";
    return false;
  }

  status_ = VALUE_UNCHANGED;

  // A big flaw in the RegKey implementation is that all error information
  // (besides success/failure) is lost in the translation from LSTATUS to bool.
  // So, we resort to direct API calls here. :-/
  HKEY raw_key = NULL;
  LSTATUS err = RegOpenKeyEx(predefined_root_, key_path_.c_str(), 0,
                             KEY_READ, &raw_key);
  if (err != ERROR_SUCCESS) {
    if (err == ERROR_FILE_NOT_FOUND) {
      LOG(INFO) << "(delete value) can not open " << key_path_;
      status_ = VALUE_NOT_FOUND;
      return true;
    }
  } else {
    ::RegCloseKey(raw_key);
  }

  RegKey key;
  bool result = false;

  // Used in REG_QWORD case only
  DWORD value_size = sizeof(old_qword_);
  DWORD type = 0;

  if (!key.Open(predefined_root_, key_path_.c_str(), KEY_READ | KEY_WRITE)) {
    LOG(ERROR) << "can not open " << key_path_;
  } else if (!key.ValueExists(value_name_.c_str())) {
    status_ = VALUE_NOT_FOUND;
    result = true;
  // Read previous value for rollback and delete
  } else if (((type_ == REG_SZ && key.ReadValue(value_name_.c_str(),
                                                &old_str_)) ||
              (type_ == REG_DWORD && key.ReadValueDW(value_name_.c_str(),
                                                     &old_dw_)) ||
              (type_ == REG_QWORD && key.ReadValue(value_name_.c_str(),
                                                   &old_qword_,
                                                   &value_size, &type) &&
                  type == REG_QWORD && value_size == sizeof(old_qword_))) &&
             (key.DeleteValue(value_name_.c_str()))) {
    status_ = VALUE_DELETED;
    result = true;
  } else {
    LOG(ERROR) << "failed to read/delete value " << value_name_;
  }

  return result;
}

void DeleteRegValueWorkItem::Rollback() {
  if (status_ == DELETE_VALUE || status_ == VALUE_ROLLED_BACK)
    return;
  if (status_ == VALUE_UNCHANGED || status_ == VALUE_NOT_FOUND) {
    status_ = VALUE_ROLLED_BACK;
    VLOG(1) << "rollback: setting unchanged, nothing to do";
    return;
  }

  // At this point only possible state is VALUE_DELETED.
  RegKey key;
  if (!key.Open(predefined_root_, key_path_.c_str(), KEY_READ | KEY_WRITE)) {
    LOG(ERROR) << "rollback: can not open " << key_path_;
  // try to restore the previous value
  } else if ((type_ == REG_SZ && key.WriteValue(value_name_.c_str(),
                                                old_str_.c_str())) ||
             (type_ == REG_DWORD && key.WriteValue(value_name_.c_str(),
                                                   old_dw_)) ||
             (type_ == REG_QWORD && key.WriteValue(value_name_.c_str(),
                                                   &old_qword_,
                                                   sizeof(old_qword_),
                                                   REG_QWORD))) {
    status_ = VALUE_ROLLED_BACK;
    VLOG(1) << "rollback: restored " << value_name_;
  } else {
    LOG(ERROR) << "failed to restore value " << value_name_;
  }

  key.Close();
  return;
}
