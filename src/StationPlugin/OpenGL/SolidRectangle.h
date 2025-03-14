#pragma once

#include "StationPlugin/OpenGL/GlMesh.h"
#include "Vertex.h"
#include "juce_opengl/opengl/juce_gl.h"
#include <vector>

/**
 * @brief An opengl mesh for a rectangle without texture.
 */
class SolidRectangle : public GlMesh
{
  public:
    SolidRectangle();
    void registerGlObjects() override;
    void drawGlObjects() override;
    void freeGlObjects() override;

  private:
    std::vector<Vertex> vertices;          /**< List of vertices with position, texture pos, and color */
    std::vector<unsigned int> triangleIds; /**< List of vertice ids to draw each triangle */

    GLuint vbo; /**< vertex buffer object identifier */
    GLuint ebo; /**< index buffer object identifier (ids of vertices for triangles to draw) */
    GLuint vao; /**< vertex array object to draw with a oneliner */
};