// Copyright (c) 2006-2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// This file declares Chrome uninstall related functions.

#ifndef CHROME_INSTALLER_SETUP_UNINSTALL_H_
#define CHROME_INSTALLER_SETUP_UNINSTALL_H_
#pragma once

#include <string>

#include <shlobj.h>

#include "base/command_line.h"
#include "chrome/installer/util/product.h"
#include "chrome/installer/util/util_constants.h"

namespace installer {
// This function removes all Chrome registration related keys. It returns true
// if successful, otherwise false. The error code is set in |exit_code|.
// |root| is the registry root (HKLM|HKCU) and |browser_entry_suffix| is the
// suffix for default browser entry name in the registry (optional).
bool DeleteChromeRegistrationKeys(BrowserDistribution* dist, HKEY root,
                                  const std::wstring& browser_entry_suffix,
                                  installer::InstallStatus& exit_code);

// Removes any legacy registry keys from earlier versions of Chrome that are no
// longer needed. This is used during autoupdate since we don't do full
// uninstalls/reinstalls to update.
void RemoveLegacyRegistryKeys(BrowserDistribution* dist);

// This function uninstalls Chrome.
//
// setup_path: Path to the executable (setup.exe) as it will be copied
//           to temp folder before deleting Chrome folder.
// dist: Represents the distribution to be uninstalled.
// remove_all: Remove all shared files, registry entries as well.
// force_uninstall: Uninstall without prompting for user confirmation or
//                  any checks for Chrome running.
// cmd_line: CommandLine that contains information about the command that
//           was used to launch current uninstaller.
installer::InstallStatus UninstallChrome(
    const FilePath& setup_path, const Product& dist, bool remove_all,
    bool force_uninstall, const CommandLine& cmd_line);

}  // namespace installer_setup

#endif  // CHROME_INSTALLER_SETUP_UNINSTALL_H_
