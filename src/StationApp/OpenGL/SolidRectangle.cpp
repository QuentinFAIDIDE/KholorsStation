#include "SolidRectangle.h"

#include "GUIToolkit/Consts.h"
#include "StationApp/OpenGL/GLInfoLogger.h"
#include "juce_opengl/juce_opengl.h"
#include <spdlog/spdlog.h>

using namespace juce::gl;

SolidRectangle::SolidRectangle()
{
    vertices.reserve(4);
    juce::Colour col = COLOR_BACKGROUND;

    // NOTE: we use a unified vertex format that includes
    // texture coordinates because we can affort to send
    // it for the few solid mesh that don't use it

    // upper left corner 0
    vertices.push_back(
        {{-1.0f, -1.0f}, {col.getFloatRed(), col.getFloatGreen(), col.getFloatBlue(), 1.0f}, {0.0f, 1.0f}});

    // upper right corner 1
    vertices.push_back(
        {{1.0f, -1.0f}, {col.getFloatRed(), col.getFloatGreen(), col.getFloatBlue(), 1.0f}, {1.0f, 1.0f}});

    // lower right corner 2
    vertices.push_back(
        {{1.0f, 1.0f}, {col.getFloatRed(), col.getFloatGreen(), col.getFloatBlue(), 1.0f}, {1.0f, 0.0f}});

    // lower left corner 3
    vertices.push_back(
        {{-1.0f, 1.0f}, {col.getFloatRed(), col.getFloatGreen(), col.getFloatBlue(), 1.0f}, {0.0f, 0.0f}});

    // lower left triangle
    triangleIds.push_back(0);
    triangleIds.push_back(2);
    triangleIds.push_back(3);

    // upper right triangle
    triangleIds.push_back(0);
    triangleIds.push_back(1);
    triangleIds.push_back(2);
}

void SolidRectangle::registerGlObjects()
{
    // generate openGL objects ids
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    // bind to the vao that hold vbo and ebo together
    glBindVertexArray(vao);

    // register and upload the vertices data (vbo)
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(sizeof(Vertex) * vertices.size()), vertices.data(), GL_STATIC_DRAW);
    // register and upload indices of the vertices to form the triangles (ebo)
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(sizeof(unsigned int) * triangleIds.size()), triangleIds.data(),
                 GL_STATIC_DRAW);

    Vertex::registerVertexFormat();
    printAllOpenGlError();
}

void SolidRectangle::drawGlObjects()
{
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, triangleIds.size(), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

void SolidRectangle::freeGlObjects()
{
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
}