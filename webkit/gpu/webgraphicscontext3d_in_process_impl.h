// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBKIT_GPU_WEBGRAPHICSCONTEXT3D_IN_PROCESS_IMPL_H_
#define WEBKIT_GPU_WEBGRAPHICSCONTEXT3D_IN_PROCESS_IMPL_H_

#include <list>
#include <set>

#include "base/hash_tables.h"
#include "base/scoped_ptr.h"
#include "third_party/angle/include/GLSLANG/ShaderLang.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebGraphicsContext3D.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebString.h"

#if !defined(OS_MACOSX)
#define FLIP_FRAMEBUFFER_VERTICALLY
#endif
namespace gfx {
class GLContext;
}

namespace webkit {
namespace gpu {

// The default implementation of WebGL. In Chromium, using this class
// requires the sandbox to be disabled, which is strongly discouraged.
// It is provided for support of test_shell and any Chromium ports
// where an in-renderer WebGL implementation would be helpful.

class WebGraphicsContext3DInProcessImpl : public WebKit::WebGraphicsContext3D {
 public:
  WebGraphicsContext3DInProcessImpl();
  virtual ~WebGraphicsContext3DInProcessImpl();

  //----------------------------------------------------------------------
  // WebGraphicsContext3D methods
  virtual bool initialize(
      WebGraphicsContext3D::Attributes attributes, WebKit::WebView*, bool);
  virtual bool makeContextCurrent();

  virtual int width();
  virtual int height();

  virtual int sizeInBytes(int type);

  virtual bool isGLES2Compliant();

  virtual void reshape(int width, int height);

  virtual bool readBackFramebuffer(unsigned char* pixels, size_t bufferSize);

  virtual unsigned int getPlatformTextureId();
  virtual void prepareTexture();

  virtual void synthesizeGLError(unsigned long error);
  virtual void* mapBufferSubDataCHROMIUM(
      unsigned target, int offset, int size, unsigned access);
  virtual void unmapBufferSubDataCHROMIUM(const void*);
  virtual void* mapTexSubImage2DCHROMIUM(
      unsigned target,
      int level,
      int xoffset,
      int yoffset,
      int width,
      int height,
      unsigned format,
      unsigned type,
      unsigned access);
  virtual void unmapTexSubImage2DCHROMIUM(const void*);
  virtual void copyTextureToParentTextureCHROMIUM(
      unsigned texture, unsigned parentTexture);

  virtual WebKit::WebString getRequestableExtensionsCHROMIUM();
  virtual void requestExtensionCHROMIUM(const char*);

  virtual void activeTexture(unsigned long texture);
  virtual void attachShader(WebKit::WebGLId program, WebKit::WebGLId shader);
  virtual void bindAttribLocation(
      WebKit::WebGLId program, unsigned long index, const char* name);
  virtual void bindBuffer(unsigned long target, WebKit::WebGLId buffer);
  virtual void bindFramebuffer(
      unsigned long target, WebKit::WebGLId framebuffer);
  virtual void bindRenderbuffer(
      unsigned long target, WebKit::WebGLId renderbuffer);
  virtual void bindTexture(unsigned long target, WebKit::WebGLId texture);
  virtual void blendColor(
      double red, double green, double blue, double alpha);
  virtual void blendEquation(unsigned long mode);
  virtual void blendEquationSeparate(
      unsigned long modeRGB, unsigned long modeAlpha);
  virtual void blendFunc(unsigned long sfactor, unsigned long dfactor);
  virtual void blendFuncSeparate(
      unsigned long srcRGB,
      unsigned long dstRGB,
      unsigned long srcAlpha,
      unsigned long dstAlpha);

  virtual void bufferData(
      unsigned long target, int size, const void* data, unsigned long usage);
  virtual void bufferSubData(
      unsigned long target, long offset, int size, const void* data);

  virtual unsigned long checkFramebufferStatus(unsigned long target);
  virtual void clear(unsigned long mask);
  virtual void clearColor(
      double red, double green, double blue, double alpha);
  virtual void clearDepth(double depth);
  virtual void clearStencil(long s);
  virtual void colorMask(bool red, bool green, bool blue, bool alpha);
  virtual void compileShader(WebKit::WebGLId shader);

  virtual void copyTexImage2D(
      unsigned long target,
      long level,
      unsigned long internalformat,
      long x,
      long y,
      unsigned long width,
      unsigned long height,
      long border);
  virtual void copyTexSubImage2D(
      unsigned long target,
      long level,
      long xoffset,
      long yoffset,
      long x,
      long y,
      unsigned long width,
      unsigned long height);
  virtual void cullFace(unsigned long mode);
  virtual void depthFunc(unsigned long func);
  virtual void depthMask(bool flag);
  virtual void depthRange(double zNear, double zFar);
  virtual void detachShader(WebKit::WebGLId program, WebKit::WebGLId shader);
  virtual void disable(unsigned long cap);
  virtual void disableVertexAttribArray(unsigned long index);
  virtual void drawArrays(unsigned long mode, long first, long count);
  virtual void drawElements(
      unsigned long mode,
      unsigned long count,
      unsigned long type,
      long offset);

  virtual void enable(unsigned long cap);
  virtual void enableVertexAttribArray(unsigned long index);
  virtual void finish();
  virtual void flush();
  virtual void framebufferRenderbuffer(
      unsigned long target,
      unsigned long attachment,
      unsigned long renderbuffertarget,
      WebKit::WebGLId renderbuffer);
  virtual void framebufferTexture2D(
      unsigned long target,
      unsigned long attachment,
      unsigned long textarget,
      WebKit::WebGLId texture,
      long level);
  virtual void frontFace(unsigned long mode);
  virtual void generateMipmap(unsigned long target);

  virtual bool getActiveAttrib(
      WebKit::WebGLId program, unsigned long index, ActiveInfo&);
  virtual bool getActiveUniform(
      WebKit::WebGLId program, unsigned long index, ActiveInfo&);

  virtual void getAttachedShaders(
      WebKit::WebGLId program, int maxCount, int* count, unsigned int* shaders);

  virtual int  getAttribLocation(WebKit::WebGLId program, const char* name);

  virtual void getBooleanv(unsigned long pname, unsigned char* value);

  virtual void getBufferParameteriv(
      unsigned long target, unsigned long pname, int* value);

  virtual Attributes getContextAttributes();

  virtual unsigned long getError();

  virtual bool isContextLost();

  virtual void getFloatv(unsigned long pname, float* value);

  virtual void getFramebufferAttachmentParameteriv(
      unsigned long target,
      unsigned long attachment,
      unsigned long pname,
      int* value);

  virtual void getIntegerv(unsigned long pname, int* value);

  virtual void getProgramiv(
      WebKit::WebGLId program, unsigned long pname, int* value);

  virtual WebKit::WebString getProgramInfoLog(WebKit::WebGLId program);

  virtual void getRenderbufferParameteriv(
      unsigned long target, unsigned long pname, int* value);

  virtual void getShaderiv(
      WebKit::WebGLId shader, unsigned long pname, int* value);

  virtual WebKit::WebString getShaderInfoLog(WebKit::WebGLId shader);

  // TBD
  // void glGetShaderPrecisionFormat(
  //     GLenum shadertype, GLenum precisiontype,
  //     GLint* range, GLint* precision);

  virtual WebKit::WebString getShaderSource(WebKit::WebGLId shader);
  virtual WebKit::WebString getString(unsigned long name);

  virtual void getTexParameterfv(
      unsigned long target, unsigned long pname, float* value);
  virtual void getTexParameteriv(
      unsigned long target, unsigned long pname, int* value);

  virtual void getUniformfv(
      WebKit::WebGLId program, long location, float* value);
  virtual void getUniformiv(
      WebKit::WebGLId program, long location, int* value);

  virtual long getUniformLocation(WebKit::WebGLId program, const char* name);

  virtual void getVertexAttribfv(
      unsigned long index, unsigned long pname, float* value);
  virtual void getVertexAttribiv(
      unsigned long index, unsigned long pname, int* value);

  virtual long getVertexAttribOffset(
      unsigned long index, unsigned long pname);

  virtual void hint(unsigned long target, unsigned long mode);
  virtual bool isBuffer(WebKit::WebGLId buffer);
  virtual bool isEnabled(unsigned long cap);
  virtual bool isFramebuffer(WebKit::WebGLId framebuffer);
  virtual bool isProgram(WebKit::WebGLId program);
  virtual bool isRenderbuffer(WebKit::WebGLId renderbuffer);
  virtual bool isShader(WebKit::WebGLId shader);
  virtual bool isTexture(WebKit::WebGLId texture);
  virtual void lineWidth(double width);
  virtual void linkProgram(WebKit::WebGLId program);
  virtual void pixelStorei(unsigned long pname, long param);
  virtual void polygonOffset(double factor, double units);

  virtual void readPixels(
      long x, long y,
      unsigned long width, unsigned long height,
      unsigned long format,
      unsigned long type,
      void* pixels);

  virtual void releaseShaderCompiler();
  virtual void renderbufferStorage(
      unsigned long target,
      unsigned long internalformat,
      unsigned long width,
      unsigned long height);
  virtual void sampleCoverage(double value, bool invert);
  virtual void scissor(
      long x, long y, unsigned long width, unsigned long height);
  virtual void shaderSource(WebKit::WebGLId shader, const char* source);
  virtual void stencilFunc(unsigned long func, long ref, unsigned long mask);
  virtual void stencilFuncSeparate(
      unsigned long face, unsigned long func, long ref, unsigned long mask);
  virtual void stencilMask(unsigned long mask);
  virtual void stencilMaskSeparate(unsigned long face, unsigned long mask);
  virtual void stencilOp(
      unsigned long fail, unsigned long zfail, unsigned long zpass);
  virtual void stencilOpSeparate(
      unsigned long face,
      unsigned long fail,
      unsigned long zfail,
      unsigned long zpass);

  virtual void texImage2D(
      unsigned target,
      unsigned level,
      unsigned internalformat,
      unsigned width,
      unsigned height,
      unsigned border,
      unsigned format,
      unsigned type,
      const void* pixels);

  virtual void texParameterf(unsigned target, unsigned pname, float param);
  virtual void texParameteri(unsigned target, unsigned pname, int param);

  virtual void texSubImage2D(
      unsigned target,
      unsigned level,
      unsigned xoffset,
      unsigned yoffset,
      unsigned width,
      unsigned height,
      unsigned format,
      unsigned type,
      const void* pixels);

  virtual void uniform1f(long location, float x);
  virtual void uniform1fv(long location, int count, float* v);
  virtual void uniform1i(long location, int x);
  virtual void uniform1iv(long location, int count, int* v);
  virtual void uniform2f(long location, float x, float y);
  virtual void uniform2fv(long location, int count, float* v);
  virtual void uniform2i(long location, int x, int y);
  virtual void uniform2iv(long location, int count, int* v);
  virtual void uniform3f(long location, float x, float y, float z);
  virtual void uniform3fv(long location, int count, float* v);
  virtual void uniform3i(long location, int x, int y, int z);
  virtual void uniform3iv(long location, int count, int* v);
  virtual void uniform4f(long location, float x, float y, float z, float w);
  virtual void uniform4fv(long location, int count, float* v);
  virtual void uniform4i(long location, int x, int y, int z, int w);
  virtual void uniform4iv(long location, int count, int* v);
  virtual void uniformMatrix2fv(
      long location, int count, bool transpose, const float* value);
  virtual void uniformMatrix3fv(
      long location, int count, bool transpose, const float* value);
  virtual void uniformMatrix4fv(
      long location, int count, bool transpose, const float* value);

  virtual void useProgram(WebKit::WebGLId program);
  virtual void validateProgram(WebKit::WebGLId program);

  virtual void vertexAttrib1f(unsigned long indx, float x);
  virtual void vertexAttrib1fv(unsigned long indx, const float* values);
  virtual void vertexAttrib2f(unsigned long indx, float x, float y);
  virtual void vertexAttrib2fv(unsigned long indx, const float* values);
  virtual void vertexAttrib3f(unsigned long indx, float x, float y, float z);
  virtual void vertexAttrib3fv(unsigned long indx, const float* values);
  virtual void vertexAttrib4f(
      unsigned long indx, float x, float y, float z, float w);
  virtual void vertexAttrib4fv(unsigned long indx, const float* values);
  virtual void vertexAttribPointer(
      unsigned long indx,
      int size,
      int type,
      bool normalized,
      unsigned long stride,
      unsigned long offset);

  virtual void viewport(
      long x, long y, unsigned long width, unsigned long height);

  // Support for buffer creation and deletion
  virtual unsigned createBuffer();
  virtual unsigned createFramebuffer();
  virtual unsigned createProgram();
  virtual unsigned createRenderbuffer();
  virtual unsigned createShader(unsigned long);
  virtual unsigned createTexture();

  virtual void deleteBuffer(unsigned);
  virtual void deleteFramebuffer(unsigned);
  virtual void deleteProgram(unsigned);
  virtual void deleteRenderbuffer(unsigned);
  virtual void deleteShader(unsigned);
  virtual void deleteTexture(unsigned);

 private:
  enum {
    kNumTrackedPointerStates = 2
  };

  // Note: we aren't currently using this information, but we will
  // need to in order to verify that all enabled vertex arrays have
  // a valid buffer bound -- to avoid crashes on certain cards.
  struct VertexAttribPointerState {
    VertexAttribPointerState();

    bool enabled;
    unsigned long buffer;
    unsigned long indx;
    int size;
    int type;
    bool normalized;
    unsigned long stride;
    unsigned long offset;
  };

  // ANGLE related.
  struct ShaderSourceEntry {
    explicit ShaderSourceEntry(unsigned long shader_type)
        : type(shader_type),
          is_valid(false) {
    }

    unsigned long type;
    scoped_array<char> source;
    scoped_array<char> log;
    scoped_array<char> translated_source;
    bool is_valid;
  };

  typedef base::hash_map<WebKit::WebGLId, ShaderSourceEntry*> ShaderSourceMap;

#ifdef FLIP_FRAMEBUFFER_VERTICALLY
  void FlipVertically(unsigned char* framebuffer,
                      unsigned int width,
                      unsigned int height);
#endif

  // Take into account the user's requested context creation attributes, in
  // particular stencil and antialias, and determine which could or could
  // not be honored based on the capabilities of the OpenGL implementation.
  void ValidateAttributes();

  // Resolve the given rectangle of the multisampled framebuffer if necessary.
  void ResolveMultisampledFramebuffer(
      unsigned x, unsigned y, unsigned width, unsigned height);

  bool AngleCreateCompilers();
  void AngleDestroyCompilers();
  bool AngleValidateShaderSource(ShaderSourceEntry* entry);

  WebGraphicsContext3D::Attributes attributes_;
  bool initialized_;
  bool render_directly_to_web_view_;
  bool is_gles2_;
  bool have_ext_framebuffer_object_;
  bool have_ext_framebuffer_multisample_;
  bool have_angle_framebuffer_multisample_;

  unsigned int texture_;
  unsigned int fbo_;
  unsigned int depth_stencil_buffer_;
  unsigned int cached_width_, cached_height_;

  // For multisampling
  unsigned int multisample_fbo_;
  unsigned int multisample_depth_stencil_buffer_;
  unsigned int multisample_color_buffer_;

  // For tracking which FBO is bound
  unsigned int bound_fbo_;

  // For tracking which texture is bound
  unsigned int bound_texture_;

  // FBO used for copying child texture to parent texture.
  unsigned copy_texture_to_parent_texture_fbo_;

  unsigned int bound_array_buffer_;

#ifdef FLIP_FRAMEBUFFER_VERTICALLY
  unsigned char* scanline_;
#endif

  VertexAttribPointerState vertex_attrib_pointer_state_[
      kNumTrackedPointerStates];

  // Errors raised by synthesizeGLError().
  std::list<unsigned long> synthetic_errors_list_;
  std::set<unsigned long> synthetic_errors_set_;

  scoped_ptr<gfx::GLContext> gl_context_;

  ShaderSourceMap shader_source_map_;

  ShHandle fragment_compiler_;
  ShHandle vertex_compiler_;
};

}  // namespace gpu
}  // namespace webkit

#endif  // WEBKIT_GPU_WEBGRAPHICSCONTEXT3D_IN_PROCESS_IMPL_H_

