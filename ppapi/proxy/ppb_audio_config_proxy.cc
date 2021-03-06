// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/proxy/ppb_audio_config_proxy.h"

#include "ppapi/c/dev/ppb_audio_config_dev.h"
#include "ppapi/proxy/plugin_dispatcher.h"
#include "ppapi/proxy/plugin_resource.h"
#include "ppapi/proxy/ppapi_messages.h"

namespace pp {
namespace proxy {

class AudioConfig : public PluginResource {
 public:
  AudioConfig(PP_AudioSampleRate_Dev sample_rate, uint32_t sample_frame_count)
      : sample_rate_(sample_rate),
        sample_frame_count_(sample_frame_count) {
  }
  virtual ~AudioConfig() {}

  // Resource overrides.
  virtual AudioConfig* AsAudioConfig() { return this; }

  PP_AudioSampleRate_Dev sample_rate() const { return sample_rate_; }
  uint32_t sample_frame_count() const { return sample_frame_count_; }

 private:
  PP_AudioSampleRate_Dev sample_rate_;
  uint32_t sample_frame_count_;

  DISALLOW_COPY_AND_ASSIGN(AudioConfig);
};

namespace {

PP_Resource CreateStereo16bit(PP_Module module_id,
                              PP_AudioSampleRate_Dev sample_rate,
                              uint32_t sample_frame_count) {
  PP_Resource result = 0;
  PluginDispatcher::Get()->Send(new PpapiHostMsg_PPBAudioConfig_Create(
      INTERFACE_ID_PPB_AUDIO_CONFIG, module_id,
      static_cast<int32_t>(sample_rate), sample_frame_count,
      &result));
  if (!result)
    return 0;

  linked_ptr<AudioConfig> object(
      new AudioConfig(sample_rate, sample_frame_count));
  PluginDispatcher::Get()->plugin_resource_tracker()->AddResource(
      result, object);
  return result;
}

uint32_t RecommendSampleFrameCount(uint32_t requested_sample_frame_count) {
  uint32_t result = 0;
  PluginDispatcher::Get()->Send(
      new PpapiHostMsg_PPBAudioConfig_RecommendSampleFrameCount(
          INTERFACE_ID_PPB_AUDIO_CONFIG, requested_sample_frame_count,
          &result));
  return result;
}

PP_Bool IsAudioConfig(PP_Resource resource) {
  AudioConfig* object = PluginResource::GetAs<AudioConfig>(resource);
  return BoolToPPBool(!!object);
}

PP_AudioSampleRate_Dev GetSampleRate(PP_Resource config_id) {
  AudioConfig* object = PluginResource::GetAs<AudioConfig>(config_id);
  if (!object)
    return PP_AUDIOSAMPLERATE_NONE;
  return object->sample_rate();
}

uint32_t GetSampleFrameCount(PP_Resource config_id) {
  AudioConfig* object = PluginResource::GetAs<AudioConfig>(config_id);
  if (!object)
    return 0;
  return object->sample_frame_count();
}

const PPB_AudioConfig_Dev audio_config_interface = {
  &CreateStereo16bit,
  &RecommendSampleFrameCount,
  &IsAudioConfig,
  &GetSampleRate,
  &GetSampleFrameCount
};

}  // namespace

PPB_AudioConfig_Proxy::PPB_AudioConfig_Proxy(Dispatcher* dispatcher,
                                             const void* target_interface)
    : InterfaceProxy(dispatcher, target_interface) {
}

PPB_AudioConfig_Proxy::~PPB_AudioConfig_Proxy() {
}

const void* PPB_AudioConfig_Proxy::GetSourceInterface() const {
  return &audio_config_interface;
}

InterfaceID PPB_AudioConfig_Proxy::GetInterfaceId() const {
  return INTERFACE_ID_PPB_AUDIO_CONFIG;
}

void PPB_AudioConfig_Proxy::OnMessageReceived(const IPC::Message& msg) {
  IPC_BEGIN_MESSAGE_MAP(PPB_AudioConfig_Proxy, msg)
    IPC_MESSAGE_HANDLER(PpapiHostMsg_PPBAudioConfig_Create,
                        OnMsgCreateStereo16Bit)
    IPC_MESSAGE_HANDLER(PpapiHostMsg_PPBAudioConfig_RecommendSampleFrameCount,
                        OnMsgRecommendSampleFrameCount)
  IPC_END_MESSAGE_MAP()
}

void PPB_AudioConfig_Proxy::OnMsgCreateStereo16Bit(PP_Module module,
                                                   int32_t sample_rate,
                                                   uint32_t sample_frame_count,
                                                   PP_Resource* result) {
  *result = ppb_audio_config_target()->CreateStereo16Bit(
      module, static_cast<PP_AudioSampleRate_Dev>(sample_rate),
      sample_frame_count);
}

void PPB_AudioConfig_Proxy::OnMsgRecommendSampleFrameCount(
    uint32_t requested,
    uint32_t* result) {
  *result = ppb_audio_config_target()->RecommendSampleFrameCount(requested);
}

}  // namespace proxy
}  // namespace pp
