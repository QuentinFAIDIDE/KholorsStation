#include "OpenGLHelpers.h"
#include "juce_opengl/opengl/juce_gl.h"
#include <spdlog/spdlog.h>

void OpenGLHelpers::clearGlView(const juce::Colour &backgroundColor)
{
    enableBlending();
    juce::gl::glClearColor(backgroundColor.getFloatRed(), backgroundColor.getFloatGreen(),
                           backgroundColor.getFloatBlue(), 1.0f);
    juce::gl::glClear(juce::gl::GL_COLOR_BUFFER_BIT);
}

void OpenGLHelpers::enableBlending()
{
    juce::gl::glEnable(juce::gl::GL_BLEND);
    juce::gl::glBlendFunc(juce::gl::GL_SRC_ALPHA, juce::gl::GL_ONE_MINUS_SRC_ALPHA);
}

void OpenGLHelpers::logOpenGLInfo(juce::OpenGLContext &)
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

void OpenGLHelpers::logOpenGLErrorCallback(GLenum, GLenum type, GLuint, GLenum severity, GLsizei, const GLchar *message,
                                           const void *)
{
    char s[1024];
    snprintf(s, 1024, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
             (type == juce::gl::GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""), type, severity, message);

    switch (severity)
    {
    case juce::gl::GL_DEBUG_SEVERITY_HIGH:
        spdlog::error(std::string(s));
        break;
    case juce::gl::GL_DEBUG_SEVERITY_MEDIUM:
        spdlog::warn(std::string(s));
        break;
    default:
        spdlog::debug(std::string(s));
        break;
    }
}

void OpenGLHelpers::enableOpenGLErrorLogging()
{
    juce::gl::glEnable(juce::gl::GL_DEBUG_OUTPUT);
    juce::gl::glDebugMessageCallback(logOpenGLErrorCallback, nullptr);
}

void OpenGLHelpers::printAllOpenGlError()
{
    GLenum err;
    while ((err = juce::gl::glGetError()) != juce::gl::GL_NO_ERROR)
    {
        spdlog::error(std::string("got following open gl error code after registering vertices data: ") +
                      std::to_string(err));
    }
}