#include "ShaderHelpers.h"

bool ShaderHelpers::buildShader(std::unique_ptr<juce::OpenGLShaderProgram> &sh, 
                                const std::string& vertexShader, 
                                const std::string& fragmentShader)
{
    return sh->addVertexShader(vertexShader) && sh->addFragmentShader(fragmentShader) && sh->link();
}