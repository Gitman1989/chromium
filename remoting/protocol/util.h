// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_PROTOCOL_UTIL_H_
#define REMOTING_PROTOCOL_UTIL_H_

#include "google/protobuf/message_lite.h"
#include "net/base/io_buffer.h"
#include "remoting/base/protocol/chromotocol.pb.h"

// This file defines utility methods used for encoding and decoding the protocol
// used in Chromoting.
namespace remoting {

// Serialize the Protocol Buffer message and provide sufficient framing for
// sending it over the wire.
// This will provide sufficient prefix and suffix for the receiver side to
// decode the message.
scoped_refptr<net::IOBufferWithSize> SerializeAndFrameMessage(
    const google::protobuf::MessageLite& msg);

}  // namespace remoting

#endif  // REMOTING_PROTOCOL_UTIL_H_