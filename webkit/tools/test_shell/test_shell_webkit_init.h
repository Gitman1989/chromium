// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBKIT_TOOLS_TEST_SHELL_TEST_SHELL_WEBKIT_INIT_H_
#define WEBKIT_TOOLS_TEST_SHELL_TEST_SHELL_WEBKIT_INIT_H_

#include "base/utf_string_conversions.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebIDBFactory.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebIDBKey.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebIDBKeyPath.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebSerializedScriptValue.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebStorageNamespace.h"
#include "webkit/glue/webclipboard_impl.h"
#include "webkit/glue/webfileutilities_impl.h"
#include "webkit/glue/webkit_glue.h"
#include "webkit/glue/webkitclient_impl.h"
#include "webkit/gpu/webgraphicscontext3d_in_process_impl.h"
#include "webkit/tools/test_shell/mock_webclipboard_impl.h"
#include "webkit/tools/test_shell/simple_appcache_system.h"
#include "webkit/tools/test_shell/simple_database_system.h"
#include "webkit/tools/test_shell/simple_file_system.h"
#include "webkit/tools/test_shell/simple_resource_loader_bridge.h"
#include "webkit/tools/test_shell/simple_webcookiejar_impl.h"
#include "webkit/tools/test_shell/test_shell_webblobregistry_impl.h"
#include "webkit/tools/test_shell/test_shell_webmimeregistry_impl.h"

#if defined(OS_WIN)
#include "webkit/tools/test_shell/test_shell_webthemeengine.h"
#endif

class TestShellWebKitInit : public webkit_glue::WebKitClientImpl {
 public:
  explicit TestShellWebKitInit(bool layout_test_mode);
  ~TestShellWebKitInit();

  virtual WebKit::WebMimeRegistry* mimeRegistry() {
    return mime_registry_.get();
  }

  WebKit::WebClipboard* clipboard();

  virtual WebKit::WebFileUtilities* fileUtilities() {
    return &file_utilities_;
  }

  virtual WebKit::WebSandboxSupport* sandboxSupport() {
    return NULL;
  }

  virtual WebKit::WebCookieJar* cookieJar() {
    return &cookie_jar_;
  }

  virtual WebKit::WebBlobRegistry* blobRegistry() {
    return blob_registry_.get();
  }

  virtual WebKit::WebFileSystem* fileSystem() {
    return &file_system_;
  }

  virtual bool sandboxEnabled() {
    return true;
  }

  virtual WebKit::WebKitClient::FileHandle databaseOpenFile(
      const WebKit::WebString& vfs_file_name, int desired_flags) {
    return SimpleDatabaseSystem::GetInstance()->OpenFile(
        vfs_file_name, desired_flags);
  }

  virtual int databaseDeleteFile(const WebKit::WebString& vfs_file_name,
                                 bool sync_dir) {
    return SimpleDatabaseSystem::GetInstance()->DeleteFile(
        vfs_file_name, sync_dir);
  }

  virtual long databaseGetFileAttributes(
      const WebKit::WebString& vfs_file_name) {
    return SimpleDatabaseSystem::GetInstance()->GetFileAttributes(
        vfs_file_name);
  }

  virtual long long databaseGetFileSize(
      const WebKit::WebString& vfs_file_name) {
    return SimpleDatabaseSystem::GetInstance()->GetFileSize(vfs_file_name);
  }

  virtual unsigned long long visitedLinkHash(const char* canonicalURL,
                                             size_t length) {
    return 0;
  }

  virtual bool isLinkVisited(unsigned long long linkHash) {
    return false;
  }

  virtual WebKit::WebMessagePortChannel* createMessagePortChannel() {
    return NULL;
  }

  virtual void prefetchHostName(const WebKit::WebString&) {
  }

  virtual WebKit::WebData loadResource(const char* name);
  virtual WebKit::WebString queryLocalizedString(
      WebKit::WebLocalizedString::Name name);
  virtual WebKit::WebString queryLocalizedString(
      WebKit::WebLocalizedString::Name name, const WebKit::WebString& value);
  virtual WebKit::WebString queryLocalizedString(
      WebKit::WebLocalizedString::Name name,
      const WebKit::WebString& value1, const WebKit::WebString& value2);

  virtual WebKit::WebString defaultLocale() {
    return ASCIIToUTF16("en-US");
  }

  virtual WebKit::WebStorageNamespace* createLocalStorageNamespace(
      const WebKit::WebString& path, unsigned quota) {
    // Enforce quota here, ignoring the value from the renderer as in Chrome.
    return WebKit::WebStorageNamespace::createLocalStorageNamespace(path,
        WebKit::WebStorageNamespace::m_localStorageQuota);
  }

  void dispatchStorageEvent(const WebKit::WebString& key,
      const WebKit::WebString& old_value, const WebKit::WebString& new_value,
      const WebKit::WebString& origin, const WebKit::WebURL& url,
      bool is_local_storage) {
    // The event is dispatched by the proxy.
  }

  virtual WebKit::WebIDBFactory* idbFactory() {
    return WebKit::WebIDBFactory::create();
  }

  virtual void createIDBKeysFromSerializedValuesAndKeyPath(
      const WebKit::WebVector<WebKit::WebSerializedScriptValue>& values,
      const WebKit::WebString& keyPath,
      WebKit::WebVector<WebKit::WebIDBKey>& keys_out) {
    WebKit::WebVector<WebKit::WebIDBKey> keys(values.size());
    for (size_t i = 0; i < values.size(); ++i) {
      keys[i] = WebKit::WebIDBKey::createFromValueAndKeyPath(
          values[i], WebKit::WebIDBKeyPath::create(keyPath));
    }
    keys_out.swap(keys);
  }

#if defined(OS_WIN)
  void SetThemeEngine(WebKit::WebThemeEngine* engine) {
    active_theme_engine_ = engine ? engine : WebKitClientImpl::themeEngine();
  }

  virtual WebKit::WebThemeEngine *themeEngine() {
    return active_theme_engine_;
  }
#endif

  virtual WebKit::WebSharedWorkerRepository* sharedWorkerRepository() {
      return NULL;
  }

  virtual WebKit::WebGraphicsContext3D* createGraphicsContext3D() {
    return new webkit::gpu::WebGraphicsContext3DInProcessImpl();
  }

 private:
  scoped_ptr<webkit_glue::SimpleWebMimeRegistryImpl> mime_registry_;
  MockWebClipboardImpl mock_clipboard_;
  webkit_glue::WebClipboardImpl real_clipboard_;
  webkit_glue::WebFileUtilitiesImpl file_utilities_;
  ScopedTempDir appcache_dir_;
  SimpleAppCacheSystem appcache_system_;
  SimpleDatabaseSystem database_system_;
  SimpleWebCookieJarImpl cookie_jar_;
  scoped_refptr<TestShellWebBlobRegistryImpl> blob_registry_;
  SimpleFileSystem file_system_;

#if defined(OS_WIN)
  WebKit::WebThemeEngine* active_theme_engine_;
#endif
};

#endif  // WEBKIT_TOOLS_TEST_SHELL_TEST_SHELL_WEBKIT_INIT_H_
