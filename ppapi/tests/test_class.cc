// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/tests/test_class.h"

#include <limits>

#include "ppapi/c/dev/ppb_testing_dev.h"
#include "ppapi/c/pp_var.h"
#include "ppapi/c/ppb_class.h"
#include "ppapi/c/ppb_var.h"
#include "ppapi/cpp/instance.h"
#include "ppapi/cpp/module.h"
#include "ppapi/cpp/var.h"
#include "ppapi/tests/testing_instance.h"

REGISTER_TEST_CASE(Class);

bool TestClass::Init() {
  class_interface_ = reinterpret_cast<const PPB_Class*>(
      pp::Module::Get()->GetBrowserInterface(PPB_CLASS_INTERFACE));
  testing_interface_ = reinterpret_cast<const PPB_Testing_Dev*>(
      pp::Module::Get()->GetBrowserInterface(PPB_TESTING_DEV_INTERFACE));
  if (!testing_interface_) {
    // Give a more helpful error message for the testing interface being gone
    // since that needs special enabling in Chrome.
    instance_->AppendError("This test needs the testing interface, which is "
        "not currently available. In Chrome, use --enable-pepper-testing when "
        "launching.");
  }
  return class_interface_ && testing_interface_;
}

void TestClass::RunTest() {
  RUN_TEST(ConstructEmptyObject);
}

std::string TestClass::TestConstructEmptyObject() {
  PP_ClassProperty properties[] = { { NULL } };
  PP_Resource object_class = class_interface_->Create(
      pp::Module::Get()->pp_module(), NULL, NULL, properties);
  ASSERT_TRUE(object_class != 0);

  pp::Var instance(pp::Var::PassRef(),
                   class_interface_->Instantiate(object_class, NULL, NULL));
  ASSERT_TRUE(instance.is_object());
  return std::string();
}

