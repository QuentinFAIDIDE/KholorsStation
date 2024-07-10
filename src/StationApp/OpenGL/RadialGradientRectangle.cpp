#include "RadialGradientRectangle.h"

#include "StationApp/OpenGL/GLInfoLogger.h"

#include <spdlog/spdlog.h>

using namespace juce::gl;

RadialGradientRectangle::RadialGradientRectangle(juce::Colour centerCol, juce::Colour borderCol)
{
    vertices.reserve(5);

    // NOTE: we use a unified vertex format that includes
    // texture coordinates because we can affort to send
    // it for the few solid mesh that don't use it

    // 0 (top left)
    vertices.push_back({{-1.0f, -1.0f, -1.0f},
                        {borderCol.getFloatRed(), borderCol.getFloatGreen(), borderCol.getFloatBlue()},
                        {0.0f, 1.0f}});

    // 1 (top middle)
    vertices.push_back({{0.0f, -1.0f, -1.0f},
                        {borderCol.getFloatRed(), borderCol.getFloatGreen(), borderCol.getFloatBlue()},
                        {0.0f, 1.0f}});

    // 2 (top right)
    vertices.push_back({{1.0f, -1.0f, -1.0f},
                        {borderCol.getFloatRed(), borderCol.getFloatGreen(), borderCol.getFloatBlue()},
                        {0.0f, 1.0f}});

    // 3 (middle left)
    vertices.push_back({{-1.0f, 0.0f, -1.0f},
                        {borderCol.getFloatRed(), borderCol.getFloatGreen(), borderCol.getFloatBlue()},
                        {0.0f, 1.0f}});

    // 4 (center)
    vertices.push_back({{0.0f, 0.0f, -1.0f},
                        {centerCol.getFloatRed(), centerCol.getFloatGreen(), centerCol.getFloatBlue()},
                        {0.0f, 1.0f}});

    // 5 (middle right)
    vertices.push_back({{1.0f, 0.0f, -1.0f},
                        {borderCol.getFloatRed(), borderCol.getFloatGreen(), borderCol.getFloatBlue()},
                        {0.0f, 1.0f}});

    // 6 (bottom left)
    vertices.push_back({{-1.0f, 1.0f, -1.0f},
                        {borderCol.getFloatRed(), borderCol.getFloatGreen(), borderCol.getFloatBlue()},
                        {0.0f, 1.0f}});

    // 7 (bottom middle)
    vertices.push_back({{0.0f, 1.0f, -1.0f},
                        {borderCol.getFloatRed(), borderCol.getFloatGreen(), borderCol.getFloatBlue()},
                        {0.0f, 1.0f}});

    // 8 (bottom right)
    vertices.push_back({{1.0f, 1.0f, -1.0f},
                        {borderCol.getFloatRed(), borderCol.getFloatGreen(), borderCol.getFloatBlue()},
                        {0.0f, 1.0f}});

    triangleIds.push_back(0);
    triangleIds.push_back(1);
    triangleIds.push_back(4);

    triangleIds.push_back(1);
    triangleIds.push_back(2);
    triangleIds.push_back(4);

    triangleIds.push_back(2);
    triangleIds.push_back(5);
    triangleIds.push_back(4);

    triangleIds.push_back(5);
    triangleIds.push_back(8);
    triangleIds.push_back(4);

    triangleIds.push_back(8);
    triangleIds.push_back(7);
    triangleIds.push_back(4);

    triangleIds.push_back(7);
    triangleIds.push_back(6);
    triangleIds.push_back(4);

    triangleIds.push_back(6);
    triangleIds.push_back(3);
    triangleIds.push_back(4);

    triangleIds.push_back(3);
    triangleIds.push_back(0);
    triangleIds.push_back(4);
}

void RadialGradientRectangle::registerGlObjects()
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

void RadialGradientRectangle::drawGlObjects()
{
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, triangleIds.size(), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

void RadialGradientRectangle::freeGlObjects()
{
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
}