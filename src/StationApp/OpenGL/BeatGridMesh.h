#pragma once

#include "GUIToolkit/Consts.h"
#include "StationApp/OpenGL/GlMesh.h"
#include "Vertex.h"
#include "juce_opengl/opengl/juce_gl.h"
#include <vector>

#define KHOLORS_BACKGROUND_GRID_TEXTURE_WIDTH 256
#define KHOLORS_BACKGROUND_GRID_TEXTURE_HEIGHT 32
#define KHOLORS_BACKGROUND_GRID_TEXTURE_COLOR KHOLORS_COLOR_GRIDS_LEVEL_0

#define KHOLORS_BACKGROUND_GRID_LEVEL_0_WIDTH 4
#define KHOLORS_BACKGROUND_GRID_LEVEL_1_WIDTH 2

#define TEXTURE_PIXEL_FLOAT_LEN 4

/**
 * @brief An openGL mesh that sits at the background
 * and which holds the beat grid texture.
 */
class BeatGridMesh : public GlMesh
{
  public:
    /**
     * @brief Construct a new Beat Grid Mesh object
     *
     * @param superGrid if true, will make it 4x wider
     */
    BeatGridMesh(bool superGrid);
    ~BeatGridMesh();

    void registerGlObjects() override;
    void drawGlObjects() override;
    void freeGlObjects() override;

    /**
     * @brief Reupload the texture to the GPU.
     */
    void refreshGpuTexture();

    /**
     * @brief Fill the texture vector of float with a beat
     * grid with the requested time signature.
     *
     * @param timeSignatureNumerator upper part of the time signature.
     * Lower one is considered to be 4.
     */
    void generateBeatGridTexture(int timeSignatureNumerator);

    /**
     * @brief Update the texture coordinates for the grid to match the viewPosition
     * (view position in samples at VISUAL_FRAME_RATE) and the viewScale
     * (number of audio samples per pixels at VISUAL_FRAME_RATE).
     *
     * @param viewPosition view position in samples at VISUAL_FRAME_RATE
     * @param viewScale number of audio samples per pixels at VISUAL_FRAME_RATE
     * @param screenWidth fft view width in pixels
     * @param bpm beats per minutes
     */
    void updateGridPosition(int64_t viewPosition, int64_t viewScale, int64_t screenWidth, float bpm);

    /**
     * @brief Hide or show the mesh.
     * Meant to be called only from the OpenGL thread (with the right shader on).
     *
     * @param visible if the mesh should be visible.
     */
    void setVisible(bool visible);

    void setTimeSignatureNumerator(int numerator);

  private:
    std::vector<float> texture; /**< Raw intensities to use as texture */

    std::vector<Vertex> vertices;          /**< List of vertices with position, texture pos, and color */
    std::vector<unsigned int> triangleIds; /**< List of vertice ids to draw each triangle */

    GLuint vbo; /**< vertex buffer object identifier */
    GLuint ebo; /**< index buffer object identifier (ids of vertices for triangles to draw) */
    GLuint vao; /**< vertex array object identifier to draw with a oneliner */
    GLuint tbo; /**< Texture buffer object identifier */

    bool isShown;

    bool isSuperGrid;

    int lastTimeSignatureNumerator;
};