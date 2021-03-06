// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// TODO(scherkus): clean up PipelineImpl... too many crazy function names,
// potential deadlocks, etc...

#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/condition_variable.h"
#include "base/stl_util-inl.h"
#include "media/base/clock_impl.h"
#include "media/base/filter_collection.h"
#include "media/base/media_format.h"
#include "media/base/pipeline_impl.h"

namespace media {

class PipelineImpl::PipelineInitState {
 public:
  scoped_refptr<DataSource> data_source_;
  scoped_refptr<Demuxer> demuxer_;
  scoped_refptr<AudioDecoder> audio_decoder_;
  scoped_refptr<VideoDecoder> video_decoder_;
};

PipelineImpl::PipelineImpl(MessageLoop* message_loop)
    : message_loop_(message_loop),
      clock_(new ClockImpl(&base::Time::Now)),
      waiting_for_clock_update_(false),
      state_(kCreated),
      remaining_transitions_(0),
      current_bytes_(0) {
  ResetState();
}

PipelineImpl::~PipelineImpl() {
  AutoLock auto_lock(lock_);
  DCHECK(!running_) << "Stop() must complete before destroying object";
  DCHECK(!stop_pending_);
  DCHECK(!seek_pending_);
}

void PipelineImpl::Init(PipelineCallback* ended_callback,
                        PipelineCallback* error_callback,
                        PipelineCallback* network_callback) {
  DCHECK(!IsRunning())
      << "Init() should be called before the pipeline has started";
  ended_callback_.reset(ended_callback);
  error_callback_.reset(error_callback);
  network_callback_.reset(network_callback);
}

// Creates the PipelineInternal and calls it's start method.
bool PipelineImpl::Start(FilterCollection* collection,
                         const std::string& url,
                         PipelineCallback* start_callback) {
  AutoLock auto_lock(lock_);
  scoped_ptr<PipelineCallback> callback(start_callback);
  scoped_ptr<FilterCollection> filter_collection(collection);

  if (running_) {
    VLOG(1) << "Media pipeline is already running";
    return false;
  }

  if (collection->IsEmpty()) {
    return false;
  }

  // Kick off initialization!
  running_ = true;
  message_loop_->PostTask(
      FROM_HERE,
      NewRunnableMethod(this,
                        &PipelineImpl::StartTask,
                        filter_collection.release(),
                        url,
                        callback.release()));
  return true;
}

void PipelineImpl::Stop(PipelineCallback* stop_callback) {
  AutoLock auto_lock(lock_);
  scoped_ptr<PipelineCallback> callback(stop_callback);
  if (!running_) {
    VLOG(1) << "Media pipeline has already stopped";
    return;
  }

  // Stop the pipeline, which will set |running_| to false on behalf.
  message_loop_->PostTask(FROM_HERE,
      NewRunnableMethod(this, &PipelineImpl::StopTask, callback.release()));
}

void PipelineImpl::Seek(base::TimeDelta time,
                        PipelineCallback* seek_callback) {
  AutoLock auto_lock(lock_);
  scoped_ptr<PipelineCallback> callback(seek_callback);
  if (!running_) {
    VLOG(1) << "Media pipeline must be running";
    return;
  }

  message_loop_->PostTask(FROM_HERE,
      NewRunnableMethod(this, &PipelineImpl::SeekTask, time,
                        callback.release()));
}

bool PipelineImpl::IsRunning() const {
  AutoLock auto_lock(lock_);
  return running_;
}

bool PipelineImpl::IsInitialized() const {
  // TODO(scherkus): perhaps replace this with a bool that is set/get under the
  // lock, because this is breaching the contract that |state_| is only accessed
  // on |message_loop_|.
  AutoLock auto_lock(lock_);
  switch (state_) {
    case kPausing:
    case kFlushing:
    case kSeeking:
    case kStarting:
    case kStarted:
    case kEnded:
      return true;
    default:
      return false;
  }
}

bool PipelineImpl::IsNetworkActive() const {
  AutoLock auto_lock(lock_);
  return network_activity_;
}

bool PipelineImpl::IsRendered(const std::string& major_mime_type) const {
  AutoLock auto_lock(lock_);
  bool is_rendered = (rendered_mime_types_.find(major_mime_type) !=
                      rendered_mime_types_.end());
  return is_rendered;
}

float PipelineImpl::GetPlaybackRate() const {
  AutoLock auto_lock(lock_);
  return playback_rate_;
}

void PipelineImpl::SetPlaybackRate(float playback_rate) {
  if (playback_rate < 0.0f) {
    return;
  }

  AutoLock auto_lock(lock_);
  playback_rate_ = playback_rate;
  if (running_) {
    message_loop_->PostTask(FROM_HERE,
        NewRunnableMethod(this, &PipelineImpl::PlaybackRateChangedTask,
                          playback_rate));
  }
}

float PipelineImpl::GetVolume() const {
  AutoLock auto_lock(lock_);
  return volume_;
}

void PipelineImpl::SetVolume(float volume) {
  if (volume < 0.0f || volume > 1.0f) {
    return;
  }

  AutoLock auto_lock(lock_);
  volume_ = volume;
  if (running_) {
    message_loop_->PostTask(FROM_HERE,
        NewRunnableMethod(this, &PipelineImpl::VolumeChangedTask,
                          volume));
  }
}

base::TimeDelta PipelineImpl::GetCurrentTime() const {
  // TODO(scherkus): perhaps replace checking state_ == kEnded with a bool that
  // is set/get under the lock, because this is breaching the contract that
  // |state_| is only accessed on |message_loop_|.
  AutoLock auto_lock(lock_);
  return GetCurrentTime_Locked();
}

base::TimeDelta PipelineImpl::GetCurrentTime_Locked() const {
  base::TimeDelta elapsed = clock_->Elapsed();
  if (state_ == kEnded || elapsed > duration_) {
    return duration_;
  }
  return elapsed;
}

base::TimeDelta PipelineImpl::GetBufferedTime() {
  AutoLock auto_lock(lock_);

  // If media is fully loaded, then return duration.
  if (loaded_ || total_bytes_ == buffered_bytes_) {
    max_buffered_time_ = duration_;
    return duration_;
  }

  base::TimeDelta current_time = GetCurrentTime_Locked();

  // If buffered time was set, we report that value directly.
  if (buffered_time_.ToInternalValue() > 0) {
    return std::max(buffered_time_, current_time);
  }

  if (total_bytes_ == 0)
    return base::TimeDelta();

  // If buffered time was not set, we use current time, current bytes, and
  // buffered bytes to estimate the buffered time.
  double estimated_rate = duration_.InMillisecondsF() / total_bytes_;
  double estimated_current_time = estimated_rate * current_bytes_;
  DCHECK_GE(buffered_bytes_, current_bytes_);
  base::TimeDelta buffered_time = base::TimeDelta::FromMilliseconds(
      static_cast<int64>(estimated_rate * (buffered_bytes_ - current_bytes_) +
                         estimated_current_time));

  // Cap approximated buffered time at the length of the video.
  buffered_time = std::min(buffered_time, duration_);

  // Make sure buffered_time is at least the current time
  buffered_time = std::max(buffered_time, current_time);

  // Only print the max buffered time for smooth buffering.
  max_buffered_time_ = std::max(buffered_time, max_buffered_time_);

  return max_buffered_time_;
}

base::TimeDelta PipelineImpl::GetMediaDuration() const {
  AutoLock auto_lock(lock_);
  return duration_;
}

int64 PipelineImpl::GetBufferedBytes() const {
  AutoLock auto_lock(lock_);
  return buffered_bytes_;
}

int64 PipelineImpl::GetTotalBytes() const {
  AutoLock auto_lock(lock_);
  return total_bytes_;
}

void PipelineImpl::GetVideoSize(size_t* width_out, size_t* height_out) const {
  CHECK(width_out);
  CHECK(height_out);
  AutoLock auto_lock(lock_);
  *width_out = video_width_;
  *height_out = video_height_;
}

bool PipelineImpl::IsStreaming() const {
  AutoLock auto_lock(lock_);
  return streaming_;
}

bool PipelineImpl::IsLoaded() const {
  AutoLock auto_lock(lock_);
  return loaded_;
}

PipelineError PipelineImpl::GetError() const {
  AutoLock auto_lock(lock_);
  return error_;
}

void PipelineImpl::SetCurrentReadPosition(int64 offset) {
  AutoLock auto_lock(lock_);

  // The current read position should never be ahead of the buffered byte
  // position but threading issues between BufferedDataSource::DoneRead_Locked()
  // and BufferedDataSource::NetworkEventCallback() can cause them to be
  // temporarily out of sync. The easiest fix for this is to cap both
  // buffered_bytes_ and current_bytes_ to always be legal values in
  // SetCurrentReadPosition() and in SetBufferedBytes().
  if (offset > buffered_bytes_)
    buffered_bytes_ = offset;
  current_bytes_ = offset;
}

int64 PipelineImpl::GetCurrentReadPosition() {
  AutoLock auto_lock(lock_);
  return current_bytes_;
}

void PipelineImpl::ResetState() {
  AutoLock auto_lock(lock_);
  const base::TimeDelta kZero;
  running_          = false;
  stop_pending_     = false;
  seek_pending_     = false;
  tearing_down_     = false;
  duration_         = kZero;
  buffered_time_    = kZero;
  buffered_bytes_   = 0;
  streaming_        = false;
  loaded_           = false;
  total_bytes_      = 0;
  video_width_      = 0;
  video_height_     = 0;
  volume_           = 1.0f;
  playback_rate_    = 0.0f;
  error_            = PIPELINE_OK;
  waiting_for_clock_update_ = false;
  audio_disabled_   = false;
  clock_->SetTime(kZero);
  rendered_mime_types_.clear();
}

bool PipelineImpl::IsPipelineOk() {
  return PIPELINE_OK == GetError();
}

bool PipelineImpl::IsPipelineInitializing() {
  DCHECK_EQ(MessageLoop::current(), message_loop_);
  return state_ == kInitDataSource ||
         state_ == kInitDemuxer ||
         state_ == kInitAudioDecoder ||
         state_ == kInitAudioRenderer ||
         state_ == kInitVideoDecoder ||
         state_ == kInitVideoRenderer;
}

bool PipelineImpl::IsPipelineStopped() {
  DCHECK_EQ(MessageLoop::current(), message_loop_);
  return state_ == kStopped || state_ == kError;
}

bool PipelineImpl::IsPipelineTearingDown() {
  DCHECK_EQ(MessageLoop::current(), message_loop_);
  return tearing_down_;
}

bool PipelineImpl::IsPipelineStopPending() {
  DCHECK_EQ(MessageLoop::current(), message_loop_);
  return stop_pending_;
}

bool PipelineImpl::IsPipelineSeeking() {
  DCHECK_EQ(MessageLoop::current(), message_loop_);
  if (!seek_pending_)
    return false;
  DCHECK(kSeeking == state_ || kPausing == state_ ||
         kFlushing == state_ || kStarting == state_);
  return true;
}

void PipelineImpl::FinishInitialization() {
  DCHECK_EQ(MessageLoop::current(), message_loop_);
  // Execute the seek callback, if present.  Note that this might be the
  // initial callback passed into Start().
  if (seek_callback_.get()) {
    seek_callback_->Run();
    seek_callback_.reset();
  }
}

// static
bool PipelineImpl::TransientState(State state) {
  return state == kPausing ||
         state == kFlushing ||
         state == kSeeking ||
         state == kStarting ||
         state == kStopping;
}

// static
PipelineImpl::State PipelineImpl::FindNextState(State current) {
  // TODO(scherkus): refactor InitializeTask() to make use of this function.
  if (current == kPausing) {
    return kFlushing;
  } else if (current == kFlushing) {
    // We will always honor Seek() before Stop(). This is based on the
    // assumption that we never accept Seek() after Stop().
    DCHECK(IsPipelineSeeking() || IsPipelineStopPending());
    return IsPipelineSeeking() ? kSeeking : kStopping;
  } else if (current == kSeeking) {
    return kStarting;
  } else if (current == kStarting) {
    return kStarted;
  } else if (current == kStopping) {
    return kStopped;
  } else {
    return current;
  }
}

void PipelineImpl::SetError(PipelineError error) {
  DCHECK(IsRunning());
  DCHECK(error != PIPELINE_OK) << "PIPELINE_OK isn't an error!";
  VLOG(1) << "Media pipeline error: " << error;

  message_loop_->PostTask(FROM_HERE,
     NewRunnableMethod(this, &PipelineImpl::ErrorChangedTask, error));
}

base::TimeDelta PipelineImpl::GetTime() const {
  DCHECK(IsRunning());
  return GetCurrentTime();
}

base::TimeDelta PipelineImpl::GetDuration() const {
  DCHECK(IsRunning());
  return GetMediaDuration();
}

void PipelineImpl::SetTime(base::TimeDelta time) {
  DCHECK(IsRunning());
  AutoLock auto_lock(lock_);

  // If we were waiting for a valid timestamp and such timestamp arrives, we
  // need to clear the flag for waiting and start the clock.
  if (waiting_for_clock_update_) {
    if (time < clock_->Elapsed())
      return;
    waiting_for_clock_update_ = false;
    clock_->SetTime(time);
    clock_->Play();
    return;
  }
  clock_->SetTime(time);
}

void PipelineImpl::SetDuration(base::TimeDelta duration) {
  DCHECK(IsRunning());
  AutoLock auto_lock(lock_);
  duration_ = duration;
}

void PipelineImpl::SetBufferedTime(base::TimeDelta buffered_time) {
  DCHECK(IsRunning());
  AutoLock auto_lock(lock_);
  buffered_time_ = buffered_time;
}

void PipelineImpl::SetTotalBytes(int64 total_bytes) {
  DCHECK(IsRunning());
  AutoLock auto_lock(lock_);
  total_bytes_ = total_bytes;
}

void PipelineImpl::SetBufferedBytes(int64 buffered_bytes) {
  DCHECK(IsRunning());
  AutoLock auto_lock(lock_);

  // See comments in SetCurrentReadPosition() about capping.
  if (buffered_bytes < current_bytes_)
    current_bytes_ = buffered_bytes;
  buffered_bytes_ = buffered_bytes;
}

void PipelineImpl::SetVideoSize(size_t width, size_t height) {
  DCHECK(IsRunning());
  AutoLock auto_lock(lock_);
  video_width_ = width;
  video_height_ = height;
}

void PipelineImpl::SetStreaming(bool streaming) {
  DCHECK(IsRunning());
  AutoLock auto_lock(lock_);
  streaming_ = streaming;
}

void PipelineImpl::NotifyEnded() {
  DCHECK(IsRunning());
  message_loop_->PostTask(FROM_HERE,
      NewRunnableMethod(this, &PipelineImpl::NotifyEndedTask));
}

void PipelineImpl::SetLoaded(bool loaded) {
  DCHECK(IsRunning());
  AutoLock auto_lock(lock_);
  loaded_ = loaded;
}

void PipelineImpl::SetNetworkActivity(bool network_activity) {
  DCHECK(IsRunning());
  {
    AutoLock auto_lock(lock_);
    network_activity_ = network_activity;
  }
  message_loop_->PostTask(FROM_HERE,
      NewRunnableMethod(this, &PipelineImpl::NotifyNetworkEventTask));
}

void PipelineImpl::DisableAudioRenderer() {
  DCHECK(IsRunning());

  // Disable renderer on the message loop.
  message_loop_->PostTask(FROM_HERE,
      NewRunnableMethod(this, &PipelineImpl::DisableAudioRendererTask));
}

void PipelineImpl::InsertRenderedMimeType(const std::string& major_mime_type) {
  DCHECK(IsRunning());
  AutoLock auto_lock(lock_);
  rendered_mime_types_.insert(major_mime_type);
}

bool PipelineImpl::HasRenderedMimeTypes() const {
  DCHECK(IsRunning());
  AutoLock auto_lock(lock_);
  return !rendered_mime_types_.empty();
}

// Called from any thread.
void PipelineImpl::OnFilterInitialize() {
  // Continue the initialize task by proceeding to the next stage.
  message_loop_->PostTask(FROM_HERE,
      NewRunnableMethod(this, &PipelineImpl::InitializeTask));
}

// Called from any thread.
void PipelineImpl::OnFilterStateTransition() {
  // Continue walking down the filters.
  message_loop_->PostTask(FROM_HERE,
      NewRunnableMethod(this, &PipelineImpl::FilterStateTransitionTask));
}

void PipelineImpl::StartTask(FilterCollection* filter_collection,
                             const std::string& url,
                             PipelineCallback* start_callback) {
  DCHECK_EQ(MessageLoop::current(), message_loop_);
  DCHECK_EQ(kCreated, state_);
  filter_collection_.reset(filter_collection);
  url_ = url;
  seek_callback_.reset(start_callback);

  // Kick off initialization.
  InitializeTask();
}

// Main initialization method called on the pipeline thread.  This code attempts
// to use the specified filter factory to build a pipeline.
// Initialization step performed in this method depends on current state of this
// object, indicated by |state_|.  After each step of initialization, this
// object transits to the next stage.  It starts by creating a DataSource,
// connects it to a Demuxer, and then connects the Demuxer's audio stream to an
// AudioDecoder which is then connected to an AudioRenderer.  If the media has
// video, then it connects a VideoDecoder to the Demuxer's video stream, and
// then connects the VideoDecoder to a VideoRenderer.
//
// When all required filters have been created and have called their
// FilterHost's InitializationComplete() method, the pipeline will update its
// state to kStarted and |init_callback_|, will be executed.
//
// TODO(hclam): InitializeTask() is now starting the pipeline asynchronously. It
// works like a big state change table. If we no longer need to start filters
// in order, we need to get rid of all the state change.
void PipelineImpl::InitializeTask() {
  DCHECK_EQ(MessageLoop::current(), message_loop_);

  // If we have received the stop or error signal, return immediately.
  if (IsPipelineStopPending() ||
      IsPipelineStopped() ||
      PIPELINE_OK != GetError()) {
    return;
  }

  DCHECK(state_ == kCreated || IsPipelineInitializing());

  // Just created, create data source.
  if (state_ == kCreated) {
    state_ = kInitDataSource;
    pipeline_init_state_.reset(new PipelineInitState());
    InitializeDataSource();
    return;
  }

  // Data source created, create demuxer.
  if (state_ == kInitDataSource) {
    state_ = kInitDemuxer;
    InitializeDemuxer(pipeline_init_state_->data_source_);
    return;
  }

  // Demuxer created, create audio decoder.
  if (state_ == kInitDemuxer) {
    state_ = kInitAudioDecoder;
    // If this method returns false, then there's no audio stream.
    if (InitializeAudioDecoder(pipeline_init_state_->demuxer_))
      return;
  }

  // Assuming audio decoder was created, create audio renderer.
  if (state_ == kInitAudioDecoder) {
    state_ = kInitAudioRenderer;
    // Returns false if there's no audio stream.
    if (InitializeAudioRenderer(pipeline_init_state_->audio_decoder_)) {
      InsertRenderedMimeType(mime_type::kMajorTypeAudio);
      return;
    }
  }

  // Assuming audio renderer was created, create video decoder.
  if (state_ == kInitAudioRenderer) {
    // Then perform the stage of initialization, i.e. initialize video decoder.
    state_ = kInitVideoDecoder;
    if (InitializeVideoDecoder(pipeline_init_state_->demuxer_))
      return;
  }

  // Assuming video decoder was created, create video renderer.
  if (state_ == kInitVideoDecoder) {
    state_ = kInitVideoRenderer;
    if (InitializeVideoRenderer(pipeline_init_state_->video_decoder_)) {
      InsertRenderedMimeType(mime_type::kMajorTypeVideo);
      return;
    }
  }

  if (state_ == kInitVideoRenderer) {
    if (!IsPipelineOk() || !HasRenderedMimeTypes()) {
      SetError(PIPELINE_ERROR_COULD_NOT_RENDER);
      return;
    }

    // Clear the collection of filters.
    filter_collection_->Clear();

    // Clear init state since we're done initializing.
    pipeline_init_state_.reset();

    // Initialization was successful, we are now considered paused, so it's safe
    // to set the initial playback rate and volume.
    PlaybackRateChangedTask(GetPlaybackRate());
    VolumeChangedTask(GetVolume());

    // Fire the initial seek request to get the filters to preroll.
    seek_pending_ = true;
    state_ = kSeeking;
    remaining_transitions_ = filters_.size();
    seek_timestamp_ = base::TimeDelta();
    filters_.front()->Seek(seek_timestamp_,
        NewCallback(this, &PipelineImpl::OnFilterStateTransition));
  }
}

// This method is called as a result of the client calling Pipeline::Stop() or
// as the result of an error condition.
// We stop the filters in the reverse order.
//
// TODO(scherkus): beware!  this can get posted multiple times since we post
// Stop() tasks even if we've already stopped.  Perhaps this should no-op for
// additional calls, however most of this logic will be changing.
void PipelineImpl::StopTask(PipelineCallback* stop_callback) {
  DCHECK_EQ(MessageLoop::current(), message_loop_);
  PipelineError error = GetError();

  if (state_ == kStopped || (IsPipelineStopPending() && error == PIPELINE_OK)) {
    // If we are already stopped or stopping normally, return immediately.
    delete stop_callback;
    return;
  } else if (state_ == kError ||
             (IsPipelineStopPending() && error != PIPELINE_OK)) {
    // If we are stopping due to SetError(), stop normally instead of
    // going to error state.
    AutoLock auto_lock(lock_);
    error_ = PIPELINE_OK;
  }

  stop_callback_.reset(stop_callback);

  stop_pending_ = true;
  if (!IsPipelineSeeking()) {
    // We will tear down pipeline immediately when there is no seek operation
    // pending. This should include the case where we are partially initialized.
    // Ideally this case should use SetError() rather than Stop() to tear down.
    DCHECK(!IsPipelineTearingDown());
    TearDownPipeline();
  }
}

void PipelineImpl::ErrorChangedTask(PipelineError error) {
  DCHECK_EQ(MessageLoop::current(), message_loop_);
  DCHECK_NE(PIPELINE_OK, error) << "PIPELINE_OK isn't an error!";

  // Suppress executing additional error logic. Note that if we are currently
  // performing a normal stop, then we return immediately and continue the
  // normal stop.
  if (IsPipelineStopped() || IsPipelineTearingDown()) {
    return;
  }

  AutoLock auto_lock(lock_);
  error_ = error;

  TearDownPipeline();
}

void PipelineImpl::PlaybackRateChangedTask(float playback_rate) {
  DCHECK_EQ(MessageLoop::current(), message_loop_);
  {
    AutoLock auto_lock(lock_);
    clock_->SetPlaybackRate(playback_rate);
  }
  for (FilterVector::iterator iter = filters_.begin();
       iter != filters_.end();
       ++iter) {
    (*iter)->SetPlaybackRate(playback_rate);
  }
}

void PipelineImpl::VolumeChangedTask(float volume) {
  DCHECK_EQ(MessageLoop::current(), message_loop_);

  if (audio_renderer_) {
    audio_renderer_->SetVolume(volume);
  }
}

void PipelineImpl::SeekTask(base::TimeDelta time,
                            PipelineCallback* seek_callback) {
  DCHECK_EQ(MessageLoop::current(), message_loop_);
  DCHECK(!IsPipelineStopPending());

  // Suppress seeking if we're not fully started.
  if (state_ != kStarted && state_ != kEnded) {
    // TODO(scherkus): should we run the callback?  I'm tempted to say the API
    // will only execute the first Seek() request.
    VLOG(1) << "Media pipeline has not started, ignoring seek to "
              << time.InMicroseconds();
    delete seek_callback;
    return;
  }

  DCHECK(!seek_pending_);
  seek_pending_ = true;

  // We'll need to pause every filter before seeking.  The state transition
  // is as follows:
  //   kStarted/kEnded
  //   kPausing (for each filter)
  //   kSeeking (for each filter)
  //   kStarting (for each filter)
  //   kStarted
  state_ = kPausing;
  seek_timestamp_ = time;
  seek_callback_.reset(seek_callback);
  remaining_transitions_ = filters_.size();

  // Kick off seeking!
  {
    AutoLock auto_lock(lock_);
    // If we are waiting for a clock update, the clock hasn't been played yet.
    if (!waiting_for_clock_update_)
      clock_->Pause();
  }
  filters_.front()->Pause(
      NewCallback(this, &PipelineImpl::OnFilterStateTransition));
}

void PipelineImpl::NotifyEndedTask() {
  DCHECK_EQ(MessageLoop::current(), message_loop_);

  // We can only end if we were actually playing.
  if (state_ != kStarted) {
    return;
  }

  DCHECK(audio_renderer_ || video_renderer_);

  // Make sure every extant renderer has ended.
  if (audio_renderer_ && !audio_disabled_) {
    if (!audio_renderer_->HasEnded()) {
      return;
    }

    if (waiting_for_clock_update_) {
      // Start clock since there is no more audio to
      // trigger clock updates.
      waiting_for_clock_update_ = false;
      clock_->Play();
    }
  }

  if (video_renderer_ && !video_renderer_->HasEnded()) {
    return;
  }

  // Transition to ended, executing the callback if present.
  state_ = kEnded;
  if (ended_callback_.get()) {
    ended_callback_->Run();
  }
}

void PipelineImpl::NotifyNetworkEventTask() {
  DCHECK_EQ(MessageLoop::current(), message_loop_);
  if (network_callback_.get()) {
    network_callback_->Run();
  }
}

void PipelineImpl::DisableAudioRendererTask() {
  DCHECK_EQ(MessageLoop::current(), message_loop_);

  // |rendered_mime_types_| is read through public methods so we need to lock
  // this variable.
  AutoLock auto_lock(lock_);
  rendered_mime_types_.erase(mime_type::kMajorTypeAudio);

  audio_disabled_ = true;

  // Notify all filters of disabled audio renderer.
  for (FilterVector::iterator iter = filters_.begin();
       iter != filters_.end();
       ++iter) {
    (*iter)->OnAudioRendererDisabled();
  }
}

void PipelineImpl::FilterStateTransitionTask() {
  DCHECK_EQ(MessageLoop::current(), message_loop_);

  // No reason transitioning if we've errored or have stopped.
  if (IsPipelineStopped()) {
    return;
  }

  if (!TransientState(state_)) {
    NOTREACHED() << "Invalid current state: " << state_;
    SetError(PIPELINE_ERROR_ABORT);
    return;
  }

  // Decrement the number of remaining transitions, making sure to transition
  // to the next state if needed.
  DCHECK(remaining_transitions_ <= filters_.size());
  DCHECK(remaining_transitions_ > 0u);
  if (--remaining_transitions_ == 0) {
    state_ = FindNextState(state_);
    if (state_ == kSeeking) {
      AutoLock auto_lock(lock_);
      clock_->SetTime(seek_timestamp_);
    }

    if (TransientState(state_)) {
      remaining_transitions_ = filters_.size();
    }
  }

  // Carry out the action for the current state.
  if (TransientState(state_)) {
    Filter* filter = filters_[filters_.size() - remaining_transitions_];
    if (state_ == kPausing) {
      filter->Pause(NewCallback(this, &PipelineImpl::OnFilterStateTransition));
    } else if (state_ == kFlushing) {
      // We had to use parallel flushing all filters.
      if (remaining_transitions_ == filters_.size()) {
        for (size_t i = 0; i < filters_.size(); i++) {
          filters_[i]->Flush(
              NewCallback(this, &PipelineImpl::OnFilterStateTransition));
        }
      }
    } else if (state_ == kSeeking) {
      filter->Seek(seek_timestamp_,
          NewCallback(this, &PipelineImpl::OnFilterStateTransition));
    } else if (state_ == kStarting) {
      filter->Play(NewCallback(this, &PipelineImpl::OnFilterStateTransition));
    } else if (state_ == kStopping) {
      filter->Stop(NewCallback(this, &PipelineImpl::OnFilterStateTransition));
    } else {
      NOTREACHED();
    }
  } else if (state_ == kStarted) {
    FinishInitialization();

    // Finally, reset our seeking timestamp back to zero.
    seek_timestamp_ = base::TimeDelta();
    seek_pending_ = false;

    AutoLock auto_lock(lock_);
    // We use audio stream to update the clock. So if there is such a stream,
    // we pause the clock until we receive a valid timestamp.
    waiting_for_clock_update_ =
        rendered_mime_types_.find(mime_type::kMajorTypeAudio) !=
        rendered_mime_types_.end();
    if (!waiting_for_clock_update_)
      clock_->Play();

    if (IsPipelineStopPending()) {
      // We had a pending stop request need to be honored right now.
      TearDownPipeline();
    }
  } else if (IsPipelineStopped()) {
    FinishDestroyingFiltersTask();
  } else {
    NOTREACHED();
  }
}

void PipelineImpl::FinishDestroyingFiltersTask() {
  DCHECK_EQ(MessageLoop::current(), message_loop_);
  DCHECK(IsPipelineStopped());

  // Stop every running filter thread.
  //
  // TODO(scherkus): can we watchdog this section to detect wedged threads?
  for (FilterThreadVector::iterator iter = filter_threads_.begin();
       iter != filter_threads_.end();
       ++iter) {
    (*iter)->Stop();
  }

  // Clear renderer references.
  audio_renderer_ = NULL;
  video_renderer_ = NULL;

  // Reset the pipeline, which will decrement a reference to this object.
  // We will get destroyed as soon as the remaining tasks finish executing.
  // To be safe, we'll set our pipeline reference to NULL.
  filters_.clear();
  STLDeleteElements(&filter_threads_);

  stop_pending_ = false;
  tearing_down_ = false;

  if (PIPELINE_OK == GetError()) {
    // Destroying filters due to Stop().
    ResetState();

    // Notify the client that stopping has finished.
    if (stop_callback_.get()) {
      stop_callback_->Run();
      stop_callback_.reset();
    }
  } else {
    // Destroying filters due to SetError().
    state_ = kError;
    // If our owner has requested to be notified of an error.
    if (error_callback_.get()) {
      error_callback_->Run();
    }
  }
}

bool PipelineImpl::PrepareFilter(scoped_refptr<Filter> filter) {
  DCHECK_EQ(MessageLoop::current(), message_loop_);
  DCHECK(IsPipelineOk());

  // Create a dedicated thread for this filter if applicable.
  if (filter->requires_message_loop()) {
    scoped_ptr<base::Thread> thread(
        new base::Thread(filter->message_loop_name()));
    if (!thread.get() || !thread->Start()) {
      NOTREACHED() << "Could not start filter thread";
      SetError(PIPELINE_ERROR_INITIALIZATION_FAILED);
      return false;
    }

    filter->set_message_loop(thread->message_loop());
    filter_threads_.push_back(thread.release());
  }

  // Register ourselves as the filter's host.
  DCHECK(IsPipelineOk());
  filter->set_host(this);
  filters_.push_back(make_scoped_refptr(filter.get()));
  return true;
}

void PipelineImpl::InitializeDataSource() {
  DCHECK_EQ(MessageLoop::current(), message_loop_);
  DCHECK(IsPipelineOk());

  scoped_refptr<DataSource> data_source;
  while (true) {
    filter_collection_->SelectDataSource(&data_source);
    if (!data_source || data_source->IsUrlSupported(url_))
      break;
  }

  if (!data_source) {
    SetError(PIPELINE_ERROR_REQUIRED_FILTER_MISSING);
    return;
  }

  if (!PrepareFilter(data_source))
    return;

  pipeline_init_state_->data_source_ = data_source;
  data_source->Initialize(
      url_, NewCallback(this, &PipelineImpl::OnFilterInitialize));
}

void PipelineImpl::InitializeDemuxer(
    const scoped_refptr<DataSource>& data_source) {
  DCHECK_EQ(MessageLoop::current(), message_loop_);
  DCHECK(IsPipelineOk());

  scoped_refptr<Demuxer> demuxer;

  CHECK(data_source);

  filter_collection_->SelectDemuxer(&demuxer);
  if (!demuxer) {
    SetError(PIPELINE_ERROR_REQUIRED_FILTER_MISSING);
    return;
  }

  if (!PrepareFilter(demuxer))
    return;

  pipeline_init_state_->demuxer_ = demuxer;
  demuxer->Initialize(data_source,
                      NewCallback(this, &PipelineImpl::OnFilterInitialize));
}

bool PipelineImpl::InitializeAudioDecoder(
    const scoped_refptr<Demuxer>& demuxer) {
  DCHECK_EQ(MessageLoop::current(), message_loop_);
  DCHECK(IsPipelineOk());

  scoped_refptr<DemuxerStream> stream =
      FindDemuxerStream(demuxer, mime_type::kMajorTypeAudio);

  if (!stream)
    return false;

  scoped_refptr<AudioDecoder> audio_decoder;
  filter_collection_->SelectAudioDecoder(&audio_decoder);

  if (!audio_decoder) {
    SetError(PIPELINE_ERROR_REQUIRED_FILTER_MISSING);
    return false;
  }

  if (!PrepareFilter(audio_decoder))
    return false;

  pipeline_init_state_->audio_decoder_ = audio_decoder;
  audio_decoder->Initialize(
      stream,
      NewCallback(this, &PipelineImpl::OnFilterInitialize));
  return true;
}

bool PipelineImpl::InitializeVideoDecoder(
    const scoped_refptr<Demuxer>& demuxer) {
  DCHECK_EQ(MessageLoop::current(), message_loop_);
  DCHECK(IsPipelineOk());

  scoped_refptr<DemuxerStream> stream =
      FindDemuxerStream(demuxer, mime_type::kMajorTypeVideo);

  if (!stream)
    return false;

  scoped_refptr<VideoDecoder> video_decoder;
  filter_collection_->SelectVideoDecoder(&video_decoder);

  if (!video_decoder) {
    SetError(PIPELINE_ERROR_REQUIRED_FILTER_MISSING);
    return false;
  }

  if (!PrepareFilter(video_decoder))
    return false;

  pipeline_init_state_->video_decoder_ = video_decoder;
  video_decoder->Initialize(
      stream,
      NewCallback(this, &PipelineImpl::OnFilterInitialize));
  return true;
}

bool PipelineImpl::InitializeAudioRenderer(
    const scoped_refptr<AudioDecoder>& decoder) {
  DCHECK_EQ(MessageLoop::current(), message_loop_);
  DCHECK(IsPipelineOk());

  if (!decoder)
    return false;

  filter_collection_->SelectAudioRenderer(&audio_renderer_);
  if (!audio_renderer_) {
    SetError(PIPELINE_ERROR_REQUIRED_FILTER_MISSING);
    return false;
  }

  if (!PrepareFilter(audio_renderer_))
    return false;

  audio_renderer_->Initialize(
      decoder, NewCallback(this, &PipelineImpl::OnFilterInitialize));
  return true;
}

bool PipelineImpl::InitializeVideoRenderer(
    const scoped_refptr<VideoDecoder>& decoder) {
  DCHECK_EQ(MessageLoop::current(), message_loop_);
  DCHECK(IsPipelineOk());

  if (!decoder)
    return false;

  filter_collection_->SelectVideoRenderer(&video_renderer_);
  if (!video_renderer_) {
    SetError(PIPELINE_ERROR_REQUIRED_FILTER_MISSING);
    return false;
  }

  if (!PrepareFilter(video_renderer_))
    return false;

  video_renderer_->Initialize(
      decoder, NewCallback(this, &PipelineImpl::OnFilterInitialize));
  return true;
}

scoped_refptr<DemuxerStream> PipelineImpl::FindDemuxerStream(
    const scoped_refptr<Demuxer>& demuxer,
    std::string major_mime_type) {
  DCHECK(demuxer);

  const int num_outputs = demuxer->GetNumberOfStreams();
  for (int i = 0; i < num_outputs; ++i) {
    std::string value;
    if (demuxer->GetStream(i)->media_format().GetAsString(
        MediaFormat::kMimeType, &value) &&
        !value.compare(0, major_mime_type.length(), major_mime_type)) {
      return demuxer->GetStream(i);
    }
  }
  return NULL;
}

void PipelineImpl::TearDownPipeline() {
  DCHECK_EQ(MessageLoop::current(), message_loop_);
  DCHECK_NE(kStopped, state_);

  // Mark that we already start tearing down operation.
  tearing_down_ = true;

  if (IsPipelineInitializing()) {
    // Notify the client that starting did not complete, if necessary.
    FinishInitialization();
  }

  remaining_transitions_ = filters_.size();
  if (remaining_transitions_ > 0) {
    if (IsPipelineInitializing()) {
      state_ = kStopping;
      filters_.front()->Stop(NewCallback(
          this, &PipelineImpl::OnFilterStateTransition));
    } else {
      state_ = kPausing;
      filters_.front()->Pause(NewCallback(
          this, &PipelineImpl::OnFilterStateTransition));
    }
  } else {
    state_ = kStopped;
    message_loop_->PostTask(FROM_HERE,
        NewRunnableMethod(this, &PipelineImpl::FinishDestroyingFiltersTask));
  }
}

}  // namespace media
