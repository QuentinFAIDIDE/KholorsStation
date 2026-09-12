#include "ShaderHelpers.h"
#include <spdlog/spdlog.h>

bool ShaderHelpers::buildShader(std::unique_ptr<juce::OpenGLShaderProgram> &sh, 
                                const std::string& vertexShader, 
                                const std::string& fragmentShader)
{
    if (!sh->addVertexShader(vertexShader) || !sh->addFragmentShader(fragmentShader) || !sh->link())
    {
        spdlog::error("OpenGL shader compilation or linking failed: {}", sh->getLastError().toStdString());
        return false;
    }

    return true;
}
