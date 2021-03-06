// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webkit/plugins/ppapi/plugin_object.h"

#include "base/logging.h"
#include "base/ref_counted.h"
#include "base/scoped_ptr.h"
#include "base/string_number_conversions.h"
#include "base/string_util.h"
#include "third_party/npapi/bindings/npapi.h"
#include "third_party/npapi/bindings/npruntime.h"
#include "ppapi/c/dev/ppb_var_deprecated.h"
#include "ppapi/c/dev/ppp_class_deprecated.h"
#include "ppapi/c/pp_resource.h"
#include "ppapi/c/pp_var.h"
#include "ppapi/c/ppb_class.h"
#include "third_party/WebKit/WebKit/chromium/public/WebBindings.h"
#include "webkit/plugins/ppapi/npapi_glue.h"
#include "webkit/plugins/ppapi/plugin_module.h"
#include "webkit/plugins/ppapi/resource.h"
#include "webkit/plugins/ppapi/string.h"
#include "webkit/plugins/ppapi/var.h"
#include "webkit/plugins/ppapi/var_object_class.h"

using WebKit::WebBindings;

namespace webkit {
namespace ppapi {

namespace {

const char kInvalidValueException[] = "Error: Invalid value";

// NPObject implementation in terms of PPP_Class_Deprecated --------------------

NPObject* WrapperClass_Allocate(NPP npp, NPClass* unused) {
  return PluginObject::AllocateObjectWrapper();
}

void WrapperClass_Deallocate(NPObject* np_object) {
  PluginObject* plugin_object = PluginObject::FromNPObject(np_object);
  if (!plugin_object)
    return;
  plugin_object->ppp_class()->Deallocate(plugin_object->ppp_class_data());
  delete plugin_object;
}

void WrapperClass_Invalidate(NPObject* object) {
}

bool WrapperClass_HasMethod(NPObject* object, NPIdentifier method_name) {
  NPObjectAccessorWithIdentifier accessor(object, method_name, false);
  if (!accessor.is_valid())
    return false;

  PPResultAndExceptionToNPResult result_converter(
      accessor.object()->GetNPObject(), NULL);
  bool rv = accessor.object()->ppp_class()->HasMethod(
      accessor.object()->ppp_class_data(), accessor.identifier(),
      result_converter.exception());
  result_converter.CheckExceptionForNoResult();
  return rv;
}

bool WrapperClass_Invoke(NPObject* object, NPIdentifier method_name,
                         const NPVariant* argv, uint32_t argc,
                         NPVariant* result) {
  NPObjectAccessorWithIdentifier accessor(object, method_name, false);
  if (!accessor.is_valid())
    return false;

  PPResultAndExceptionToNPResult result_converter(
      accessor.object()->GetNPObject(), result);
  PPVarArrayFromNPVariantArray args(accessor.object()->module(), argc, argv);

  return result_converter.SetResult(accessor.object()->ppp_class()->Call(
      accessor.object()->ppp_class_data(), accessor.identifier(),
      argc, args.array(), result_converter.exception()));
}

bool WrapperClass_InvokeDefault(NPObject* np_object, const NPVariant* argv,
                                uint32_t argc, NPVariant* result) {
  PluginObject* obj = PluginObject::FromNPObject(np_object);
  if (!obj)
    return false;

  PPVarArrayFromNPVariantArray args(obj->module(), argc, argv);
  PPResultAndExceptionToNPResult result_converter(obj->GetNPObject(), result);

  result_converter.SetResult(obj->ppp_class()->Call(
      obj->ppp_class_data(), PP_MakeUndefined(), argc, args.array(),
      result_converter.exception()));
  return result_converter.success();
}

bool WrapperClass_HasProperty(NPObject* object, NPIdentifier property_name) {
  NPObjectAccessorWithIdentifier accessor(object, property_name, true);
  if (!accessor.is_valid())
    return false;

  PPResultAndExceptionToNPResult result_converter(
      accessor.object()->GetNPObject(), NULL);
  bool rv = accessor.object()->ppp_class()->HasProperty(
      accessor.object()->ppp_class_data(), accessor.identifier(),
      result_converter.exception());
  result_converter.CheckExceptionForNoResult();
  return rv;
}

bool WrapperClass_GetProperty(NPObject* object, NPIdentifier property_name,
                              NPVariant* result) {
  NPObjectAccessorWithIdentifier accessor(object, property_name, true);
  if (!accessor.is_valid())
    return false;

  PPResultAndExceptionToNPResult result_converter(
      accessor.object()->GetNPObject(), result);
  return result_converter.SetResult(accessor.object()->ppp_class()->GetProperty(
      accessor.object()->ppp_class_data(), accessor.identifier(),
      result_converter.exception()));
}

bool WrapperClass_SetProperty(NPObject* object, NPIdentifier property_name,
                              const NPVariant* value) {
  NPObjectAccessorWithIdentifier accessor(object, property_name, true);
  if (!accessor.is_valid())
    return false;

  PPResultAndExceptionToNPResult result_converter(
      accessor.object()->GetNPObject(), NULL);
  PP_Var value_var = Var::NPVariantToPPVar(accessor.object()->module(), value);
  accessor.object()->ppp_class()->SetProperty(
      accessor.object()->ppp_class_data(), accessor.identifier(), value_var,
      result_converter.exception());
  Var::PluginReleasePPVar(value_var);
  return result_converter.CheckExceptionForNoResult();
}

bool WrapperClass_RemoveProperty(NPObject* object, NPIdentifier property_name) {
  NPObjectAccessorWithIdentifier accessor(object, property_name, true);
  if (!accessor.is_valid())
    return false;

  PPResultAndExceptionToNPResult result_converter(
      accessor.object()->GetNPObject(), NULL);
  accessor.object()->ppp_class()->RemoveProperty(
      accessor.object()->ppp_class_data(), accessor.identifier(),
      result_converter.exception());
  return result_converter.CheckExceptionForNoResult();
}

bool WrapperClass_Enumerate(NPObject* object, NPIdentifier** values,
                            uint32_t* count) {
  *values = NULL;
  *count = 0;
  PluginObject* obj = PluginObject::FromNPObject(object);
  if (!obj)
    return false;

  uint32_t property_count = 0;
  PP_Var* properties = NULL;  // Must be freed!
  PPResultAndExceptionToNPResult result_converter(obj->GetNPObject(), NULL);
  obj->ppp_class()->GetAllPropertyNames(obj->ppp_class_data(),
                                        &property_count, &properties,
                                        result_converter.exception());

  // Convert the array of PP_Var to an array of NPIdentifiers. If any
  // conversions fail, we will set the exception.
  if (!result_converter.has_exception()) {
    if (property_count > 0) {
      *values = static_cast<NPIdentifier*>(
          malloc(sizeof(NPIdentifier) * property_count));
      *count = 0;  // Will be the number of items successfully converted.
      for (uint32_t i = 0; i < property_count; ++i) {
        if (!((*values)[i] = Var::PPVarToNPIdentifier(properties[i]))) {
          // Throw an exception for the failed convertion.
          *result_converter.exception() = StringVar::StringToPPVar(
              obj->module(), kInvalidValueException);
          break;
        }
        (*count)++;
      }

      if (result_converter.has_exception()) {
        // We don't actually have to free the identifiers we converted since
        // all identifiers leak anyway :( .
        free(*values);
        *values = NULL;
        *count = 0;
      }
    }
  }

  // This will actually throw the exception, either from GetAllPropertyNames,
  // or if anything was set during the conversion process.
  result_converter.CheckExceptionForNoResult();

  // Release the PP_Var that the plugin allocated. On success, they will all
  // be converted to NPVariants, and on failure, we want them to just go away.
  for (uint32_t i = 0; i < property_count; ++i)
    Var::PluginReleasePPVar(properties[i]);
  free(properties);
  return result_converter.success();
}

bool WrapperClass_Construct(NPObject* object, const NPVariant* argv,
                            uint32_t argc, NPVariant* result) {
  PluginObject* obj = PluginObject::FromNPObject(object);
  if (!obj)
    return false;

  PPVarArrayFromNPVariantArray args(obj->module(), argc, argv);
  PPResultAndExceptionToNPResult result_converter(obj->GetNPObject(), result);
  return result_converter.SetResult(obj->ppp_class()->Construct(
      obj->ppp_class_data(), argc, args.array(),
      result_converter.exception()));
}

const NPClass wrapper_class = {
  NP_CLASS_STRUCT_VERSION,
  WrapperClass_Allocate,
  WrapperClass_Deallocate,
  WrapperClass_Invalidate,
  WrapperClass_HasMethod,
  WrapperClass_Invoke,
  WrapperClass_InvokeDefault,
  WrapperClass_HasProperty,
  WrapperClass_GetProperty,
  WrapperClass_SetProperty,
  WrapperClass_RemoveProperty,
  WrapperClass_Enumerate,
  WrapperClass_Construct
};

}  // namespace

// PluginObject ----------------------------------------------------------------

struct PluginObject::NPObjectWrapper : public NPObject {
  // Points to the var object that owns this wrapper. This value may be NULL
  // if there is no var owning this wrapper. This can happen if the plugin
  // releases all references to the var, but a reference to the underlying
  // NPObject is still held by script on the page.
  PluginObject* obj;
};

PluginObject::PluginObject(PluginModule* module,
                           NPObjectWrapper* object_wrapper,
                           const PPP_Class_Deprecated* ppp_class,
                           void* ppp_class_data)
    : module_(module),
      object_wrapper_(object_wrapper),
      ppp_class_(ppp_class),
      ppp_class_data_(ppp_class_data) {
  // Make the object wrapper refer back to this class so our NPObject
  // implementation can call back into the Pepper layer.
  object_wrapper_->obj = this;
  module_->AddPluginObject(this);
}

PluginObject::~PluginObject() {
  // The wrapper we made for this NPObject may still have a reference to it
  // from JavaScript, so we clear out its ObjectVar back pointer which will
  // cause all calls "up" to the plugin to become NOPs. Our ObjectVar base
  // class will release our reference to the object, which may or may not
  // delete the NPObject.
  DCHECK(object_wrapper_->obj == this);
  object_wrapper_->obj = NULL;
  module_->RemovePluginObject(this);
}

PP_Var PluginObject::Create(PluginModule* module,
                            const PPP_Class_Deprecated* ppp_class,
                            void* ppp_class_data) {
  // This will internally end up calling our AllocateObjectWrapper via the
  // WrapperClass_Allocated function which will have created an object wrapper
  // appropriate for this class (derived from NPObject).
  NPObjectWrapper* wrapper = static_cast<NPObjectWrapper*>(
      WebBindings::createObject(NULL, const_cast<NPClass*>(&wrapper_class)));

  // This object will register itself both with the NPObject and with the
  // PluginModule. The NPObject will normally handle its lifetime, and it
  // will get deleted in the destroy method. It may also get deleted when the
  // plugin module is deallocated.
  new PluginObject(module, wrapper, ppp_class, ppp_class_data);

  // We can just use a normal ObjectVar to refer to this object from the
  // plugin. It will hold a ref to the underlying NPObject which will in turn
  // hold our pluginObject.
  return ObjectVar::NPObjectToPPVar(module, wrapper);
}

NPObject* PluginObject::GetNPObject() const {
  return object_wrapper_;
}

// static
bool PluginObject::IsInstanceOf(NPObject* np_object,
                                const PPP_Class_Deprecated* ppp_class,
                                void** ppp_class_data) {
  // Validate that this object is implemented by our wrapper class before
  // trying to get the PluginObject.
  if (np_object->_class != &wrapper_class)
    return false;

  PluginObject* plugin_object = FromNPObject(np_object);
  if (!plugin_object)
    return false;  // Object is no longer alive.

  if (plugin_object->ppp_class() != ppp_class)
    return false;
  if (ppp_class_data)
    *ppp_class_data = plugin_object->ppp_class_data();
  return true;
}

// static
PluginObject* PluginObject::FromNPObject(NPObject* object) {
  return static_cast<NPObjectWrapper*>(object)->obj;
}

// static
NPObject* PluginObject::AllocateObjectWrapper() {
  NPObjectWrapper* wrapper = new NPObjectWrapper;
  memset(wrapper, 0, sizeof(NPObjectWrapper));
  return wrapper;
}

}  // namespace ppapi
}  // namespace webkit

