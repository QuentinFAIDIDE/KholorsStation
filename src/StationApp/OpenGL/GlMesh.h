#pragma once

// Number of floats that makes a pixel in a texture.
// There are 4 floats for RGBA. Monochrome textures only use alpha
// for intensity but
// with the texture compression that will follow,
// the difficulty of maintaining code with 4 bytes alignments
// and the restricted formats of OpenGL, the pain may not
// be worth the gain for now.
inline constexpr int TEXTURE_PIXEL_FLOAT_LEN = 4;

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