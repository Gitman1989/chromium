// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_MODELS_ACCELERATOR_COCOA_H_
#define UI_BASE_MODELS_ACCELERATOR_COCOA_H_
#pragma once

#include <Foundation/Foundation.h>

#include "base/scoped_nsobject.h"
#include "ui/base/models/accelerator.h"

namespace ui {

// This is a subclass of the cross-platform Accelerator, but with more direct
// support for Cocoa key equivalents. Note that the typical use case for this
// class is to initialize it with a string literal, which is why it sends
// |-copy| to the |key_code| paramater in the constructor.
class AcceleratorCocoa : public Accelerator {
 public:
  AcceleratorCocoa(NSString* key_code, NSUInteger mask)
      : Accelerator(ui::VKEY_UNKNOWN, mask),
        characters_([key_code copy]) {
  }

  AcceleratorCocoa(const AcceleratorCocoa& accelerator)
      : Accelerator(accelerator) {
    characters_.reset([accelerator.characters_ copy]);
  }

  AcceleratorCocoa() : Accelerator() {}
  virtual ~AcceleratorCocoa() {}

  AcceleratorCocoa& operator=(const AcceleratorCocoa& accelerator) {
    if (this != &accelerator) {
      *static_cast<Accelerator*>(this) = accelerator;
      characters_.reset([accelerator.characters_ copy]);
    }
    return *this;
  }

  bool operator==(const AcceleratorCocoa& rhs) const {
    return [characters_ isEqualToString:rhs.characters_.get()] &&
        (modifiers_ == rhs.modifiers_);
  }

  NSString* characters() const {
    return characters_.get();
  }

 private:
  // String of characters for the key equivalent.
  scoped_nsobject<NSString> characters_;
};

}  // namespace ui

#endif  // UI_BASE_MODELS_ACCELERATOR_COCOA_H_
