// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/filters.h"

#include "base/logging.h"

namespace media {

Filter::Filter() : host_(NULL) {}

Filter::~Filter() {}

const char* Filter::major_mime_type() const {
  return "";
}

void Filter::set_host(FilterHost* host) {
  DCHECK(host);
  DCHECK(!host_);
  host_ = host;
}

FilterHost* Filter::host() {
  return host_;
}

void Filter::Play(FilterCallback* callback) {
  DCHECK(callback);
  if (callback) {
    callback->Run();
    delete callback;
  }
}

void Filter::Pause(FilterCallback* callback) {
  DCHECK(callback);
  if (callback) {
    callback->Run();
    delete callback;
  }
}

void Filter::Flush(FilterCallback* callback) {
  DCHECK(callback);
  if (callback) {
    callback->Run();
    delete callback;
  }
}

void Filter::Stop(FilterCallback* callback) {
  DCHECK(callback);
  if (callback) {
    callback->Run();
    delete callback;
  }
}

void Filter::SetPlaybackRate(float playback_rate) {}

void Filter::Seek(base::TimeDelta time, FilterCallback* callback) {
  scoped_ptr<FilterCallback> seek_callback(callback);
  if (seek_callback.get()) {
    seek_callback->Run();
  }
}

void Filter::OnAudioRendererDisabled() {
}

bool DataSource::IsUrlSupported(const std::string& url) {
  return true;
}

const char* AudioDecoder::major_mime_type() const {
  return mime_type::kMajorTypeAudio;
}

const char* AudioRenderer::major_mime_type() const {
  return mime_type::kMajorTypeAudio;
}

const char* VideoDecoder::major_mime_type() const {
  return mime_type::kMajorTypeVideo;
}

const char* VideoRenderer::major_mime_type() const {
  return mime_type::kMajorTypeVideo;
}

void* DemuxerStream::QueryInterface(const char* interface_id) {
  return NULL;
}

DemuxerStream::~DemuxerStream() {}

VideoDecoder::VideoDecoder() {}

VideoDecoder::~VideoDecoder() {}

AudioDecoder::AudioDecoder() {}

AudioDecoder::~AudioDecoder() {}

}  // namespace media
