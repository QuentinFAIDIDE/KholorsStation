#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_opengl/juce_opengl.h>
#include <spdlog/spdlog.h>

// helper from this kind sir:
// https://forum.juce.com/t/just-a-quick-gl-info-logger-func-for-any-of-you/32082/2
inline void logOpenGLInfoCallback(juce::OpenGLContext &)
{
    int major = 0, minor = 0;
    juce::gl::glGetIntegerv(juce::gl::GL_MAJOR_VERSION, &major);
    juce::gl::glGetIntegerv(juce::gl::GL_MINOR_VERSION, &minor);

    juce::String stats;
    stats << "---------------------------" << juce::newLine << "=== OpenGL/GPU Information ===" << juce::newLine
          << "Vendor: " << juce::String((const char *)juce::gl::glGetString(juce::gl::GL_VENDOR)) << juce::newLine
          << "Renderer: " << juce::String((const char *)juce::gl::glGetString(juce::gl::GL_RENDERER)) << juce::newLine
          << "OpenGL Version: " << juce::String((const char *)juce::gl::glGetString(juce::gl::GL_VERSION))
          << juce::newLine << "OpenGL Major: " << juce::String(major) << juce::newLine
          << "OpenGL Minor: " << juce::String(minor) << juce::newLine << "OpenGL Shading Language Version: "
          << juce::String((const char *)juce::gl::glGetString(juce::gl::GL_SHADING_LANGUAGE_VERSION)) << juce::newLine
          << "---------------------------" << juce::newLine;

    spdlog::debug(stats.toStdString());
}

inline void logOpenGLErrorCallback(GLenum, GLenum type, GLuint, GLenum severity, GLsizei, const GLchar *message,
                                   const void *)
{
    // as instructed in: https://www.khronos.org/opengl/wiki/OpenGL_Error
    // NOTE: errors are defined in juce_gl.h.
    // NOTE: it shouldn't disturb the legacy openGL error stack
    char s[1024];
    snprintf(s, 1024, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
             (type == juce::gl::GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""), type, severity, message);
    spdlog::error(std::string(s));
}

inline void enableOpenGLErrorLogging()
{
    juce::gl::glEnable(juce::gl::GL_DEBUG_OUTPUT);
    juce::gl::glDebugMessageCallback(logOpenGLErrorCallback, nullptr);
}

inline void printAllOpenGlError()
{
    GLenum err;
    while ((err = juce::gl::glGetError()) != juce::gl::GL_NO_ERROR)
    {
        spdlog::error(std::string("got following open gl error code after registering vertices data: ") +
                      std::to_string(err));
    }
}