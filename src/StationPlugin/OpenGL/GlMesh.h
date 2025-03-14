#pragma once

/**
 * @brief An abstract class representing an OpenGL
 * mesh (vertices and texture).
 */
class GlMesh
{
  public:
    virtual void registerGlObjects() = 0;
    virtual void drawGlObjects() = 0;
    virtual void freeGlObjects() = 0;
};