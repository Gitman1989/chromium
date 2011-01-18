// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/proxy/ppb_flash_proxy.h"

#include "base/logging.h"
#include "build/build_config.h"
#include "ppapi/c/dev/pp_file_info_dev.h"
#include "ppapi/c/dev/ppb_font_dev.h"
#include "ppapi/c/pp_completion_callback.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/c/pp_resource.h"
#include "ppapi/c/private/ppb_flash.h"
#include "ppapi/proxy/plugin_dispatcher.h"
#include "ppapi/proxy/plugin_resource.h"
#include "ppapi/proxy/ppapi_messages.h"
#include "ppapi/proxy/serialized_var.h"

namespace pp {
namespace proxy {

namespace {

// Given an error code and a handle result from a Pepper API call, converts
// to a PlatformFileForTransit, possibly also updating the error value if
// an error occurred.
IPC::PlatformFileForTransit PlatformFileToPlatformFileForTransit(
    int32_t* error,
    base::PlatformFile file) {
  if (*error != PP_OK)
    return IPC::InvalidPlatformFileForTransit();
#if defined(OS_WIN)
/* TODO(brettw): figure out how to get the target process handle.
  HANDLE result;
  if (!::DuplicateHandle(::GetCurrentProcess(), file,
                         target_process, &result, 0, false,
                         DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS)) {
    *error = PP_ERROR_NOACCESS;
    return INVALID_HANDLE_VALUE;
  }
  return result;
*/
  NOTIMPLEMENTED();
  *error = PP_ERROR_NOACCESS;
  return INVALID_HANDLE_VALUE;
#elif defined(OS_POSIX)
  return base::FileDescriptor(file, true);
#endif
}

void SetInstanceAlwaysOnTop(PP_Instance pp_instance, bool on_top) {
  PluginDispatcher::Get()->Send(
      new PpapiHostMsg_PPBFlash_SetInstanceAlwaysOnTop(
          INTERFACE_ID_PPB_FLASH, pp_instance, on_top));
}

bool DrawGlyphs(PP_Instance instance,
                PP_Resource pp_image_data,
                const PP_FontDescription_Dev* font_desc,
                uint32_t color,
                PP_Point position,
                PP_Rect clip,
                const float transformation[3][3],
                uint32_t glyph_count,
                const uint16_t glyph_indices[],
                const PP_Point glyph_advances[]) {
  Dispatcher* dispatcher = PluginDispatcher::Get();

  PPBFlash_DrawGlyphs_Params params;
  params.instance = instance,
  params.pp_image_data = pp_image_data;
  params.font_desc.SetFromPPFontDescription(dispatcher, *font_desc, true);
  params.color = color;
  params.position = position;
  params.clip = clip;
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++)
      params.transformation[i][j] = transformation[i][j];
  }

  params.glyph_indices.insert(params.glyph_indices.begin(),
                              &glyph_indices[0],
                              &glyph_indices[glyph_count]);
  params.glyph_advances.insert(params.glyph_advances.begin(),
                               &glyph_advances[0],
                               &glyph_advances[glyph_count]);

  bool result = false;
  dispatcher->Send(new PpapiHostMsg_PPBFlash_DrawGlyphs(
      INTERFACE_ID_PPB_FLASH, params, &result));
  return result;
}

PP_Var GetProxyForURL(PP_Instance instance, const char* url) {
  ReceiveSerializedVarReturnValue result;
  PluginDispatcher::Get()->Send(new PpapiHostMsg_PPBFlash_GetProxyForURL(
      INTERFACE_ID_PPB_FLASH, instance, url, &result));
  return result.Return(PluginDispatcher::Get());
}

int32_t OpenModuleLocalFile(PP_Instance instance,
                            const char* path,
                            int32_t mode,
                            PP_FileHandle* file) {
  int32_t result = PP_ERROR_FAILED;
  IPC::PlatformFileForTransit transit;
  PluginDispatcher::Get()->Send(new PpapiHostMsg_PPBFlash_OpenModuleLocalFile(
      INTERFACE_ID_PPB_FLASH, instance, path, mode, &transit, &result));
  *file = IPC::PlatformFileForTransitToPlatformFile(transit);
  return result;
}

int32_t RenameModuleLocalFile(PP_Instance instance,
                              const char* path_from,
                              const char* path_to) {
  int32_t result = PP_ERROR_FAILED;
  PluginDispatcher::Get()->Send(new PpapiHostMsg_PPBFlash_RenameModuleLocalFile(
      INTERFACE_ID_PPB_FLASH, instance, path_from, path_to, &result));
  return result;
}

int32_t DeleteModuleLocalFileOrDir(PP_Instance instance,
                                   const char* path,
                                   bool recursive) {
  int32_t result = PP_ERROR_FAILED;
  PluginDispatcher::Get()->Send(
      new PpapiHostMsg_PPBFlash_DeleteModuleLocalFileOrDir(
          INTERFACE_ID_PPB_FLASH, instance, path, recursive, &result));
  return result;
}

int32_t CreateModuleLocalDir(PP_Instance instance, const char* path) {
  int32_t result = PP_ERROR_FAILED;
  PluginDispatcher::Get()->Send(new PpapiHostMsg_PPBFlash_CreateModuleLocalDir(
      INTERFACE_ID_PPB_FLASH, instance, path, &result));
  return result;
}

int32_t QueryModuleLocalFile(PP_Instance instance,
                             const char* path,
                             PP_FileInfo_Dev* info) {
  int32_t result = PP_ERROR_FAILED;
  PluginDispatcher::Get()->Send(
      new PpapiHostMsg_PPBFlash_QueryModuleLocalFile(
          INTERFACE_ID_PPB_FLASH, instance, path, info, &result));
  return result;
}

int32_t GetModuleLocalDirContents(PP_Instance instance,
                                  const char* path,
                                  PP_DirContents_Dev** contents) {
  int32_t result = PP_ERROR_FAILED;
  std::vector<SerializedDirEntry> entries;
  PluginDispatcher::Get()->Send(
      new PpapiHostMsg_PPBFlash_GetModuleLocalDirContents(
          INTERFACE_ID_PPB_FLASH, instance, path, &entries, &result));

  if (result != PP_OK)
    return result;

  // Copy the serialized dir entries to the output struct.
  *contents = new PP_DirContents_Dev;
  (*contents)->count = static_cast<int32_t>(entries.size());
  (*contents)->entries = new PP_DirEntry_Dev[entries.size()];
  for (size_t i = 0; i < entries.size(); i++) {
    const SerializedDirEntry& source = entries[i];
    PP_DirEntry_Dev* dest = &(*contents)->entries[i];

    char* name_copy = new char[source.name.size() + 1];
    memcpy(name_copy, source.name.c_str(), source.name.size() + 1);
    dest->name = name_copy;
    dest->is_dir = source.is_dir;
  }

  return result;
}

void FreeModuleLocalDirContents(PP_Instance instance,
                                PP_DirContents_Dev* contents) {
  for (int32_t i = 0; i < contents->count; ++i)
    delete[] contents->entries[i].name;
  delete[] contents->entries;
  delete contents;
}

bool NavigateToURL(PP_Instance pp_instance,
                   const char* url,
                   const char* target) {
  bool result = false;
  PluginDispatcher::Get()->Send(
      new PpapiHostMsg_PPBFlash_NavigateToURL(
          INTERFACE_ID_PPB_FLASH, pp_instance, url, target, &result));
  return result;
}

const PPB_Flash ppb_flash = {
  &SetInstanceAlwaysOnTop,
  &DrawGlyphs,
  &GetProxyForURL,
  &OpenModuleLocalFile,
  &RenameModuleLocalFile,
  &DeleteModuleLocalFileOrDir,
  &CreateModuleLocalDir,
  &QueryModuleLocalFile,
  &GetModuleLocalDirContents,
  &FreeModuleLocalDirContents,
  &NavigateToURL,
};

}  // namespace

PPB_Flash_Proxy::PPB_Flash_Proxy(Dispatcher* dispatcher,
                                   const void* target_interface)
    : InterfaceProxy(dispatcher, target_interface) {
}

PPB_Flash_Proxy::~PPB_Flash_Proxy() {
}

const void* PPB_Flash_Proxy::GetSourceInterface() const {
  return &ppb_flash;
}

InterfaceID PPB_Flash_Proxy::GetInterfaceId() const {
  return INTERFACE_ID_PPB_FLASH;
}

bool PPB_Flash_Proxy::OnMessageReceived(const IPC::Message& msg) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(PPB_Flash_Proxy, msg)
    IPC_MESSAGE_HANDLER(PpapiHostMsg_PPBFlash_SetInstanceAlwaysOnTop,
                        OnMsgSetInstanceAlwaysOnTop)
    IPC_MESSAGE_HANDLER(PpapiHostMsg_PPBFlash_DrawGlyphs,
                        OnMsgDrawGlyphs)
    IPC_MESSAGE_HANDLER(PpapiHostMsg_PPBFlash_GetProxyForURL,
                        OnMsgGetProxyForURL)
    IPC_MESSAGE_HANDLER(PpapiHostMsg_PPBFlash_OpenModuleLocalFile,
                        OnMsgOpenModuleLocalFile)
    IPC_MESSAGE_HANDLER(PpapiHostMsg_PPBFlash_RenameModuleLocalFile,
                        OnMsgRenameModuleLocalFile)
    IPC_MESSAGE_HANDLER(PpapiHostMsg_PPBFlash_DeleteModuleLocalFileOrDir,
                        OnMsgDeleteModuleLocalFileOrDir)
    IPC_MESSAGE_HANDLER(PpapiHostMsg_PPBFlash_CreateModuleLocalDir,
                        OnMsgCreateModuleLocalDir)
    IPC_MESSAGE_HANDLER(PpapiHostMsg_PPBFlash_QueryModuleLocalFile,
                        OnMsgQueryModuleLocalFile)
    IPC_MESSAGE_HANDLER(PpapiHostMsg_PPBFlash_GetModuleLocalDirContents,
                        OnMsgGetModuleLocalDirContents)
    IPC_MESSAGE_HANDLER(PpapiHostMsg_PPBFlash_NavigateToURL, OnMsgNavigateToURL)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  // TODO(brettw) handle bad messages!
  return handled;
}

void PPB_Flash_Proxy::OnMsgSetInstanceAlwaysOnTop(
    PP_Instance instance,
    bool on_top) {
  ppb_flash_target()->SetInstanceAlwaysOnTop(instance, on_top);
}

void PPB_Flash_Proxy::OnMsgDrawGlyphs(
    const pp::proxy::PPBFlash_DrawGlyphs_Params& params,
    bool* result) {
  *result = false;

  PP_FontDescription_Dev font_desc;
  params.font_desc.SetToPPFontDescription(dispatcher(), &font_desc, false);

  if (params.glyph_indices.size() != params.glyph_advances.size() ||
      params.glyph_indices.empty())
    return;

  *result = ppb_flash_target()->DrawGlyphs(
      params.instance, params.pp_image_data, &font_desc, params.color,
      params.position, params.clip,
      const_cast<float(*)[3]>(params.transformation),
      static_cast<uint32_t>(params.glyph_indices.size()),
      const_cast<uint16_t*>(&params.glyph_indices[0]),
      const_cast<PP_Point*>(&params.glyph_advances[0]));
}

void PPB_Flash_Proxy::OnMsgGetProxyForURL(PP_Instance instance,
                                          const std::string& url,
                                          SerializedVarReturnValue result) {
  result.Return(dispatcher(), ppb_flash_target()->GetProxyForURL(
      instance, url.c_str()));
}

void PPB_Flash_Proxy::OnMsgOpenModuleLocalFile(
    PP_Instance instance,
    const std::string& path,
    int32_t mode,
    IPC::PlatformFileForTransit* file_handle,
    int32_t* result) {
  base::PlatformFile file;
  *result = ppb_flash_target()->OpenModuleLocalFile(instance, path.c_str(),
                                                    mode, &file);
  *file_handle = PlatformFileToPlatformFileForTransit(result, file);
}

void PPB_Flash_Proxy::OnMsgRenameModuleLocalFile(
    PP_Instance instance,
    const std::string& path_from,
    const std::string& path_to,
    int32_t* result) {
  *result = ppb_flash_target()->RenameModuleLocalFile(instance,
                                                      path_from.c_str(),
                                                      path_to.c_str());
}

void PPB_Flash_Proxy::OnMsgDeleteModuleLocalFileOrDir(
    PP_Instance instance,
    const std::string& path,
    bool recursive,
    int32_t* result) {
  *result = ppb_flash_target()->DeleteModuleLocalFileOrDir(instance,
                                                           path.c_str(),
                                                           recursive);
}

void PPB_Flash_Proxy::OnMsgCreateModuleLocalDir(PP_Instance instance,
                                                const std::string& path,
                                                int32_t* result) {
  *result = ppb_flash_target()->CreateModuleLocalDir(instance, path.c_str());
}

void PPB_Flash_Proxy::OnMsgQueryModuleLocalFile(PP_Instance instance,
                                                const std::string& path,
                                                PP_FileInfo_Dev* info,
                                                int32_t* result) {
  *result = ppb_flash_target()->QueryModuleLocalFile(instance, path.c_str(),
                                                     info);
}

void PPB_Flash_Proxy::OnMsgGetModuleLocalDirContents(
    PP_Instance instance,
    const std::string& path,
    std::vector<pp::proxy::SerializedDirEntry>* entries,
    int32_t* result) {
  PP_DirContents_Dev* contents = NULL;
  *result = ppb_flash_target()->GetModuleLocalDirContents(instance,
                                                          path.c_str(),
                                                          &contents);
  if (*result != PP_OK)
    return;

  // Convert the list of entries to the serialized version.
  entries->resize(contents->count);
  for (int32_t i = 0; i < contents->count; i++) {
    (*entries)[i].name.assign(contents->entries[i].name);
    (*entries)[i].is_dir = contents->entries[i].is_dir;
  }
  ppb_flash_target()->FreeModuleLocalDirContents(instance, contents);
}

void PPB_Flash_Proxy::OnMsgNavigateToURL(PP_Instance instance,
                                         const std::string& url,
                                         const std::string& target,
                                         bool* result) {
  *result = ppb_flash_target()->NavigateToURL(instance, url.c_str(),
                                              target.c_str());
}

}  // namespace proxy
}  // namespace pp
