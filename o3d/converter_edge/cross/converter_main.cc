/*
 * Copyright 2009, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


// This file contains the main routine for the converter that writes
// out a scene graph as a JSON file.

#include <string>
#include <iostream>
#include <vector>

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/file_path.h"
#include "base/string_util.h"
#include "converter_edge/cross/converter.h"
#include "utils/cross/file_path_utils.h"

using std::string;
using std::wstring;

#if defined(OS_WIN)
int wmain(int argc, wchar_t **argv) {
  // On Windows, CommandLine::Init ignores its arguments and uses
  // GetCommandLineW.
  CommandLine::Init(0, NULL);
#endif
#if defined(OS_LINUX)
int main(int argc, char **argv) {
  CommandLine::Init(argc, argv);
#endif
#if defined(OS_MACOSX)
// The "real" main on Mac is in mac/converter_main.mm, so we can get
// memory pool initialization for Cocoa.
int CrossMain(int argc, char**argv) {
  CommandLine::Init(argc, argv);
#endif
  // Create an at_exit_manager so that base singletons will get
  // deleted properly.
  base::AtExitManager at_exit_manager;
  const CommandLine* command_line = CommandLine::ForCurrentProcess();

  FilePath in_filename, out_filename;

  std::vector<std::wstring> values = command_line->GetLooseValues();
  if (values.size() == 1) {
    // If we're only given one argument, then construct the output
    // filename by substituting the extension on the filename (if any)
    // with .o3dtgz.
    in_filename = o3d::WideToFilePath(values[0]);
    out_filename = in_filename.ReplaceExtension(FILE_PATH_LITERAL(".o3dtgz"));
  } else if (values.size()== 2) {
    in_filename = o3d::WideToFilePath(values[0]);
    out_filename = o3d::WideToFilePath(values[1]);
  } else {
    std::cerr << "Usage: " << argv[0]
              << " [ options ] <infile.dae> [ <outfile> ]\n";
    std::cerr
        << "--no-condition\n"
        << "    Stops the converter from conditioning shaders.\n"
        << "--base-path=<path>\n"
        << "    Sets the path to remove from URIs of external files\n"
        << "--asset-paths=<comma separted list of paths>\n"
        << "    Sets the paths for finding textures and other external\n"
        << "    files.\n"
        << "--up-axis=x,y,z\n"
        << "    Converts the file to have this up axis.\n"
        << "--pretty-print\n"
        << "    Makes the exported JSON easier to read.\n"
        << "--keep-filters\n"
        << "    Stops the converter from forcing all texture samplers to use\n"
        << "    tri-linear filtering.\n"
        << "--keep-materials\n"
        << "    Stops the converter from changing materials to <constant> if\n"
        << "    they are used by a mesh that has no normals.\n"
        << "--sharp-edge-threshold=<threshold>\n"
        << "    Adds edges with normal angle larger than <threshold>.\n"
        << "--sharp-edge-color=r,g,b\n"
        << "    Specify color of the additional edges, (value 0 to 1).\n"
        << "    Default value is 1,0,0.\n";
    return EXIT_FAILURE;
  }

  o3d::converter::Options options;
  options.condition = !command_line->HasSwitch("no-condition");
  options.pretty_print = command_line->HasSwitch("pretty-print");
  if (command_line->HasSwitch("base-path")) {
    options.base_path = o3d::WideToFilePath(
        command_line->GetSwitchValue("base-path"));
  }
  if (command_line->HasSwitch("asset-paths")) {
    std::vector<std::wstring> paths;
    SplitString(command_line->GetSwitchValue("asset-paths"), ',', &paths);
    for (size_t ii = 0; ii < paths.size(); ++ii) {
      options.file_paths.push_back(o3d::WideToFilePath(paths[ii]));
    }
  }
  if (command_line->HasSwitch("up-axis")) {
    wstring up_axis_string = command_line->GetSwitchValue("up-axis");
    int x, y, z;
    if (swscanf(up_axis_string.c_str(), L"%d,%d,%d", &x, &y, &z) != 3) {
      std::cerr << "Invalid --up-axis value. Should be --up-axis=x,y,z\n";
      return EXIT_FAILURE;
    }
    options.up_axis = o3d::Vector3(static_cast<float>(x),
                                   static_cast<float>(y),
                                   static_cast<float>(z));
  }
  if (command_line->HasSwitch("sharp-edge-threshold")) {
    wstring soften_edge_string =
      command_line->GetSwitchValue("sharp-edge-threshold");
    float soft_threshold;
    if (swscanf(soften_edge_string.c_str(), L"%f", &soft_threshold) != 1) {
      std::cerr << "Invalid --sharp-edges-threshold value.\n";
      return EXIT_FAILURE;
    }
    options.enable_add_sharp_edge = true;
    options.sharp_edge_threshold = soft_threshold;
  }
  if (command_line->HasSwitch("sharp-edge-color")) {
    wstring edge_color_str =
      command_line->GetSwitchValue("sharp-edge-color");
    int r, g, b;
    if (swscanf(edge_color_str.c_str(), L"%d,%d,%d", &r, &g, &b) != 3) {
      std::cerr << "Invalid --sharp-edge-color value. Should be "
                << "--sharp-edge-color=r,g,b\n";
      return EXIT_FAILURE;
    }
    options.sharp_edge_color = o3d::Vector3(static_cast<float>(r),
                                            static_cast<float>(g),
                                            static_cast<float>(b));
  }

  o3d::String error_messages;
  bool result = o3d::converter::Convert(in_filename,
                                        out_filename,
                                        options,
                                        &error_messages);
  if (result) {
    std::cerr << "Converted '" << o3d::FilePathToUTF8(in_filename).c_str()
              << "' to '" << o3d::FilePathToUTF8(out_filename).c_str()
              << "'." << std::endl;
    return EXIT_SUCCESS;
  } else {
    std::cerr << error_messages.c_str() << std::endl;
    std::cerr << "FAILED to convert '"
              << o3d::FilePathToUTF8(in_filename).c_str()
              << "' to '" << o3d::FilePathToUTF8(out_filename).c_str()
              << "'." << std::endl;
    return EXIT_FAILURE;
  }
}
