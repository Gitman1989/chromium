// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync/syncable/syncable_id.h"

#include <iosfwd>

#include "base/string_util.h"

using std::ostream;
using std::string;

namespace syncable {
const Id kNullId;  // Currently == root.

ostream& operator<<(ostream& out, const Id& id) {
  out << id.s_;
  return out;
}

using browser_sync::FastDump;
FastDump& operator<<(FastDump& dump, const Id& id) {
  dump.out_->sputn(id.s_.data(), id.s_.size());
  return dump;
}

string Id::GetServerId() const {
  // Currently root is the string "0". We need to decide on a true value.
  // "" would be convenient here, as the IsRoot call would not be needed.
  if (IsRoot())
    return "0";
  return s_.substr(1);
}

Id Id::CreateFromServerId(const string& server_id) {
  Id id;
  if (server_id == "0")
    id.s_ = "r";
  else
    id.s_ = string("s") + server_id;
  return id;
}

Id Id::CreateFromClientString(const string& local_id) {
  Id id;
  if (local_id == "0")
    id.s_ = "r";
  else
    id.s_ = string("c") + local_id;
  return id;
}

}  // namespace syncable
