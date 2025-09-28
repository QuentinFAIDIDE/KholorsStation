#pragma once

#include "juce_graphics/juce_graphics.h"
#include "juce_opengl/juce_opengl.h"

class OpenGLHelpers
{
  public:
    /**
     * @brief Enable blending and clear OpenGL view with background color.
     */
    static void clearGlView(const juce::Colour& backgroundColor);

    /**
     * @brief Setup standard OpenGL blending for transparency.
     */
    static void enableBlending();

    /**
     * @brief Log OpenGL and GPU information.
     */
    static void logOpenGLInfo(juce::OpenGLContext& context);

    /**
     * @brief Enable OpenGL error logging with debug callback.
     */
    static void enableOpenGLErrorLogging();

    /**
     * @brief Print all current OpenGL errors.
     */
    static void printAllOpenGlError();

  private:
    /**
     * @brief OpenGL error callback function.
     */
    static void logOpenGLErrorCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);
};