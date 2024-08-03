#pragma once

#include "StationApp/OpenGL/GlMesh.h"
#include "Vertex.h"
#include "juce_opengl/opengl/juce_gl.h"
#include <vector>

// Number of floats that makes a pixel in a texture.
// There are 4 floats for RGBA. We only use alpha for intensity but
// with the texture compression that will follow,
// the difficulty of maintaining code with 4 bytes alignments
// and the restricted formats of OpenGL, the pain may not
// be worth the gain for now.
#define TEXTURE_PIXEL_FLOAT_LEN 4

/**
 * @brief An opengl mesh for a rectangle without texture.
 * This object should only be used within the OpenGL renderer thread.
 */
class TexturedRectangle : public GlMesh
{
  public:
    TexturedRectangle(int64_t width, int64_t height, juce::Colour col);
    ~TexturedRectangle();

    void registerGlObjects() override;
    void drawGlObjects() override;
    void freeGlObjects() override;

    /**
     * @brief If the texture has been changed since last upload
     * will reupload the texture to GPU.
     */
    void refreshGpuTextureIfChanged();

    /**
     * @brief Change the vertice colors inside the GPU.
     * @param col The color to apply (alpha channel is ignored).
     */
    void changeColor(juce::Colour col);

    /**
     * @brief Set a pixel inside the texture.
     * It just write to a buffer and we bulk upload on refreshGpuTextureIfChanged.
     *
     * @param x horizontal position in the texture
     * @param y vertical position in the texture
     * @param intensity intensity of the pixel between 0 and 1
     */
    void setPixelAt(int x, int y, float intensity);

    /**
     * @brief Write all intensities of the provided vector (range [0, 1]) on the
     * channel (left/right/both) and repeat it between startX and endX.
     * It's an ultra optimized version of the previous pattern of repeatedly calling setPixelAt.
     *
     *
     * @param channel 0 for left (top), 1 for right (bottom), 2 for both
     * @param startX first horizontal pixel to copy fft at
     * @param endX last horizontal pixel to copy fft at
     * @param intensities intensities of the vertical line, must be sized of half texture height
     */
    void setRepeatedVerticalHalfLine(int channel, size_t startX, size_t endX, float *intensities);

    /**
     * @brief Clears all the texture data. It just clear the buffer
     * so calling refreshGpuTextureIfChanged is necessary after.
     */
    void clearAllData();

    /**
     * @brief Set position of the rectangle and its width.
     * Its height is assumed to be full height.
     * This is the height in audio samples given the VISUAL_SAMPLE_RATE.
     * The trackIdentifier will serve as a height to decide on drawing order.
     *
     * @param viewPositionSamples samples the objects starts at
     * @param width width of the object in audio samples
     * @param trackIdentifier identifier of the track to draw
     */
    void setPosition(int64_t viewPositionSamples, int64_t width, uint64_t trackIdentifer);

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

    size_t halfTextureHeight, rowWidth, sideStrafeStep, sideStrafe; /**< used for drawing whole ffts */
};