#pragma once

#include "StationApp/OpenGL/GlMesh.h"
#include "Vertex.h"
#include "juce_opengl/opengl/juce_gl.h"
#include <vector>

// Number of floats that makes a pixel in a texture.
// There are 4 floats for RGBA.
#define TEXTURE_PIXEL_FLOAT_LEN 4

/**
 * @brief An opengl mesh for a rectangle with a RGBA texture.
 * This object should only be used within the OpenGL renderer thread.
 */
class TexturedMulticoloredRectangle : public GlMesh
{
  public:
    TexturedMulticoloredRectangle(int64_t width, int64_t height);
    ~TexturedMulticoloredRectangle();

    void registerGlObjects() override;
    void drawGlObjects() override;
    void freeGlObjects() override;

    /**
     * @brief If the texture has been changed since last upload
     * will reupload the texture to GPU.
     */
    void refreshGpuTextureIfChanged();

    /**
     * @brief Set a pixel inside the texture.
     * It just write to a buffer and we bulk upload on refreshGpuTextureIfChanged.
     *
     */
    void setPixelAt(int x, int y, float r, float g, float b, float a);

    /**
     * @brief Set a rectangle of pixels inside the texture.
     * It just write to a buffer and we bulk upload on refreshGpuTextureIfChanged.
     */
    void setRectangle(int x, int y, int width, int height, float r, float g, float b, float a);

    /**
     * @brief Set the position of the rectangle in the view.
     * It updates the openGl vertex buffer object.
     */
    void setPosition(int64_t viewPositionSamples);

    /**
     * @brief Clears all the texture data. It just clear the buffer
     * so calling refreshGpuTextureIfChanged is necessary after.
     */
    void clearAllData();

  private:
    int64_t textureWidth, textureHeight; /**< Dimensions of texture */
    std::vector<float> texture;          /**< Raw intensities to use as texture */
    int64_t textureNonce;                /**< A number changed everytime the texture gets modified */
    int64_t lastUploadedTextureNonce;    /**< Last nonce where the texture was uploaded to GPU */

    std::vector<Vertex> vertices;          /**< List of vertices with position, texture pos, and color */
    std::vector<unsigned int> triangleIds; /**< List of vertice ids to draw each triangle */

    GLuint vbo; /**< vertex buffer object identifier */
    GLuint ebo; /**< index buffer object identifier (ids of vertices for triangles to draw) */
    GLuint vao; /**< vertex array object identifier to draw with a oneliner */
    GLuint tbo; /**< Texture buffer object identifier */
};