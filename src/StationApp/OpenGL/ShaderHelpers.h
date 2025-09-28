#pragma once

#include "juce_opengl/juce_opengl.h"
#include <memory>
#include <string>

class ShaderHelpers
{
  public:
    /**
     * @brief Build a shader program from vertex and fragment shader source.
     */
    static bool buildShader(std::unique_ptr<juce::OpenGLShaderProgram> &sh, 
                           const std::string& vertexShader, 
                           const std::string& fragmentShader);
};