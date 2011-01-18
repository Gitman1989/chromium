// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// This file contains the declarations of the installer functions that build
// the WorkItemList used to install the application.

#ifndef CHROME_INSTALLER_SETUP_INSTALL_WORKER_H_
#define CHROME_INSTALLER_SETUP_INSTALL_WORKER_H_
#pragma once

#include <vector>

#include "base/scoped_ptr.h"

class CommandLine;
class FilePath;
class Version;
class WorkItemList;

namespace installer {

class InstallationState;
class InstallerState;
class Package;
class Product;

// Builds the complete WorkItemList used to build the set of installation steps
// needed to lay down one or more installed products.
//
// setup_path: Path to the executable (setup.exe) as it will be copied
//           to Chrome install folder after install is complete
// archive_path: Path to the archive (chrome.7z) as it will be copied
//               to Chrome install folder after install is complete
// src_path: the path that contains a complete and unpacked Chrome package
//           to be installed.
// temp_dir: the path of working directory used during installation. This path
//           does not need to exist.
void AddInstallWorkItems(const InstallationState& original_state,
                         const InstallerState& installer_state,
                         bool multi_install,
                         const FilePath& setup_path,
                         const FilePath& archive_path,
                         const FilePath& src_path,
                         const FilePath& temp_dir,
                         const Version& new_version,
                         scoped_ptr<Version>* current_version,
                         const Package& package,
                         WorkItemList* install_list);

// Appends registration or unregistration work items to |work_item_list| for the
// COM DLLs whose file names are given in |dll_files| and which reside in the
// path |dll_folder|.
// |system_level| specifies whether to call the system or user level DLL
// registration entry points.
// |do_register| says whether to register or unregister.
// |may_fail| states whether this is best effort or not. If |may_fail| is true
// then |work_item_list| will still succeed if the registration fails and
// no registration rollback will be performed.
void AddRegisterComDllWorkItems(const FilePath& dll_folder,
                                const std::vector<FilePath>& dll_files,
                                bool system_level,
                                bool do_register,
                                bool ignore_failures,
                                WorkItemList* work_item_list);

void AddSetMsiMarkerWorkItem(const Product& product,
                             bool set,
                             WorkItemList* work_item_list);

// Called for either installation or uninstallation. This method updates the
// registry according to Chrome Frame specific options for the current
// installation.  This includes handling of the ready-mode option.
void AddChromeFrameWorkItems(bool install, const FilePath& setup_path,
                             const Version& new_version, const Product& product,
                             WorkItemList* list);


// This method adds work items to create (or update) Chrome uninstall entry in
// either the Control Panel->Add/Remove Programs list or in the Omaha client
// state key if running under an MSI installer.
void AddUninstallShortcutWorkItems(const FilePath& setup_path,
                                   const Version& new_version,
                                   WorkItemList* install_list,
                                   const Product& product);

void AddUninstallShortcutWorkItems(const FilePath& setup_path,
                                   const Version& new_version,
                                   WorkItemList* install_list,
                                   const Product& product);

// Utility method currently shared between install.cc and install_worker.cc
void AppendUninstallCommandLineFlags(CommandLine* uninstall_cmd,
                                     const Product& product);

}  // namespace installer

#endif  // CHROME_INSTALLER_SETUP_INSTALL_WORKER_H_
