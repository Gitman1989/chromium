// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This class describe a keyboard accelerator (or keyboard shortcut).
// Keyboard accelerators are registered with the FocusManager.
// It has a copy constructor and assignment operator so that it can be copied.
// It also defines the < operator so that it can be used as a key in a std::map.
//

#ifndef VIEWS_ACCELERATOR_H_
#define VIEWS_ACCELERATOR_H_
#pragma once

#include <string>

#include "app/menus/accelerator.h"
#include "views/event.h"

namespace views {

class Accelerator : public menus::Accelerator {
 public:
  Accelerator() : menus::Accelerator() {}

  Accelerator(app::KeyboardCode keycode, int modifiers)
      : menus::Accelerator(keycode, modifiers) {}

  Accelerator(app::KeyboardCode keycode,
              bool shift_pressed, bool ctrl_pressed, bool alt_pressed) {
    key_code_ = keycode;
    modifiers_ = 0;
    if (shift_pressed)
      modifiers_ |= Event::EF_SHIFT_DOWN;
    if (ctrl_pressed)
      modifiers_ |= Event::EF_CONTROL_DOWN;
    if (alt_pressed)
      modifiers_ |= Event::EF_ALT_DOWN;
  }

  virtual ~Accelerator() {}

  bool IsShiftDown() const {
    return (modifiers_ & Event::EF_SHIFT_DOWN) == Event::EF_SHIFT_DOWN;
  }

  bool IsCtrlDown() const {
    return (modifiers_ & Event::EF_CONTROL_DOWN) == Event::EF_CONTROL_DOWN;
  }

  bool IsAltDown() const {
    return (modifiers_ & Event::EF_ALT_DOWN) == Event::EF_ALT_DOWN;
  }

  // Returns a string with the localized shortcut if any.
  std::wstring GetShortcutText() const;
};

// An interface that classes that want to register for keyboard accelerators
// should implement.
class AcceleratorTarget {
 public:
  // This method should return true if the accelerator was processed.
  virtual bool AcceleratorPressed(const Accelerator& accelerator) = 0;

 protected:
  virtual ~AcceleratorTarget() {}
};
}

#endif  // VIEWS_ACCELERATOR_H_
