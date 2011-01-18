// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBKIT_PLUGINS_PPAPI_MOCK_PLUGIN_DELEGATE_H_
#define WEBKIT_PLUGINS_PPAPI_MOCK_PLUGIN_DELEGATE_H_

#include "webkit/plugins/ppapi/plugin_delegate.h"

namespace webkit {
namespace ppapi {

class MockPluginDelegate : public PluginDelegate {
 public:
  MockPluginDelegate();
  ~MockPluginDelegate();

  virtual void InstanceCreated(PluginInstance* instance);
  virtual void InstanceDeleted(PluginInstance* instance);
  virtual PlatformImage2D* CreateImage2D(int width, int height);
  virtual PlatformContext3D* CreateContext3D();
  virtual PlatformVideoDecoder* CreateVideoDecoder(
      const PP_VideoDecoderConfig_Dev& decoder_config);
  virtual PlatformAudio* CreateAudio(uint32_t sample_rate,
                                     uint32_t sample_count,
                                     PlatformAudio::Client* client);
  virtual void NumberOfFindResultsChanged(int identifier,
                                          int total,
                                          bool final_result);
  virtual void SelectedFindResultChanged(int identifier, int index);
  virtual bool RunFileChooser(
      const WebKit::WebFileChooserParams& params,
      WebKit::WebFileChooserCompletion* chooser_completion);
  virtual bool AsyncOpenFile(const FilePath& path,
                             int flags,
                             AsyncOpenFileCallback* callback);
  virtual bool OpenFileSystem(
      const GURL& url,
      fileapi::FileSystemType type,
      long long size,
      fileapi::FileSystemCallbackDispatcher* dispatcher);
  virtual bool MakeDirectory(
      const FilePath& path,
      bool recursive,
      fileapi::FileSystemCallbackDispatcher* dispatcher);
  virtual bool Query(const FilePath& path,
                     fileapi::FileSystemCallbackDispatcher* dispatcher);
  virtual bool Touch(const FilePath& path,
                     const base::Time& last_access_time,
                     const base::Time& last_modified_time,
                     fileapi::FileSystemCallbackDispatcher* dispatcher);
  virtual bool Delete(const FilePath& path,
                      fileapi::FileSystemCallbackDispatcher* dispatcher);
  virtual bool Rename(const FilePath& file_path,
                      const FilePath& new_file_path,
                      fileapi::FileSystemCallbackDispatcher* dispatcher);
  virtual bool ReadDirectory(
      const FilePath& directory_path,
      fileapi::FileSystemCallbackDispatcher* dispatcher);
  virtual base::PlatformFileError OpenModuleLocalFile(
      const std::string& module_name,
      const FilePath& path,
      int flags,
      base::PlatformFile* file);
  virtual base::PlatformFileError RenameModuleLocalFile(
      const std::string& module_name,
      const FilePath& path_from,
      const FilePath& path_to);
  virtual base::PlatformFileError DeleteModuleLocalFileOrDir(
      const std::string& module_name,
      const FilePath& path,
      bool recursive);
  virtual base::PlatformFileError CreateModuleLocalDir(
      const std::string& module_name,
      const FilePath& path);
  virtual base::PlatformFileError QueryModuleLocalFile(
      const std::string& module_name,
      const FilePath& path,
      base::PlatformFileInfo* info);
  virtual base::PlatformFileError GetModuleLocalDirContents(
      const std::string& module_name,
      const FilePath& path,
      DirContents* contents);
  virtual scoped_refptr<base::MessageLoopProxy>
      GetFileThreadMessageLoopProxy();
  virtual int32_t ConnectTcp(
      webkit::ppapi::PPB_Flash_NetConnector_Impl* connector,
      const char* host,
      uint16_t port);
  virtual int32_t ConnectTcpAddress(
      webkit::ppapi::PPB_Flash_NetConnector_Impl* connector,
      const struct PP_Flash_NetAddress* addr);
  virtual FullscreenContainer* CreateFullscreenContainer(
      PluginInstance* instance);
  virtual std::string GetDefaultEncoding();
  virtual void ZoomLimitsChanged(double minimum_factor,
                                 double maximum_factor);
  virtual std::string ResolveProxy(const GURL& url);
  virtual void DidStartLoading();
  virtual void DidStopLoading();
  virtual void SetContentRestriction(int restrictions);
};

}  // namespace ppapi
}  // namespace webkit

#endif  // WEBKIT_PLUGINS_PPAPI_MOCK_PLUGIN_DELEGATE_H_
