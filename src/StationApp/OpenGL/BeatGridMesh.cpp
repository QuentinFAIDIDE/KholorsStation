#include "BeatGridMesh.h"

#include "OpenGLHelpers.h"
#include "StationApp/GUI/AudioConstants.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

BeatGridMesh::BeatGridMesh(bool superGrid) : isSuperGrid(superGrid)
{
    vertices.reserve(4);

    juce::Colour const col = KHOLORS_BACKGROUND_GRID_TEXTURE_COLOR;

    // upper left corner 0
    vertices.push_back(
        {{-1.0f, -1.0f, -1.0f}, {col.getFloatRed(), col.getFloatGreen(), col.getFloatBlue()}, {0.0f, 1.0f}});

    // upper right corner 1
    vertices.push_back(
        {{1.0f, -1.0f, -1.0f}, {col.getFloatRed(), col.getFloatGreen(), col.getFloatBlue()}, {1.0f, 1.0f}});

    // lower right corner 2
    vertices.push_back(
        {{1.0f, 1.0f, -1.0f}, {col.getFloatRed(), col.getFloatGreen(), col.getFloatBlue()}, {1.0f, 0.0f}});

    // lower left corner 3
    vertices.push_back(
        {{-1.0f, 1.0f, -1.0f}, {col.getFloatRed(), col.getFloatGreen(), col.getFloatBlue()}, {0.0f, 0.0f}});

    // lower left triangle
    triangleIds.push_back(0);
    triangleIds.push_back(2);
    triangleIds.push_back(3);

    // upper right triangle
    triangleIds.push_back(0);
    triangleIds.push_back(1);
    triangleIds.push_back(2);

    texture.resize(static_cast<size_t>(KHOLORS_BACKGROUND_GRID_TEXTURE_WIDTH * KHOLORS_BACKGROUND_GRID_TEXTURE_HEIGHT *
                                       TEXTURE_PIXEL_FLOAT_LEN));
    std::ranges::fill(texture, 0.0f);
}

BeatGridMesh::~BeatGridMesh() = default;

void BeatGridMesh::setTimeSignatureNumerator(int numerator)
{
    lastTimeSignatureNumerator = numerator;
}

void BeatGridMesh::registerGlObjects()
{
    // generate objects
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    OpenGLHelpers::printAllOpenGlError();

    glBindVertexArray(vao);

    // register and upload the vertices data
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(Vertex) * vertices.size()), vertices.data(),
                 GL_STATIC_DRAW);
    // register and upload indices of the vertices to form the triangles

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(unsigned int) * triangleIds.size()),
                 triangleIds.data(), GL_STATIC_DRAW);

    // register the vertex attribute format
    Vertex::registerVertexFormat();
    OpenGLHelpers::printAllOpenGlError();

    // register the texture
    glGenTextures(1, &tbo);
    glBindTexture(GL_TEXTURE_2D, tbo);
    // set the texture wrapping
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // set the filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // send the texture to the gpu
    glTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA, KHOLORS_BACKGROUND_GRID_TEXTURE_WIDTH,
                 KHOLORS_BACKGROUND_GRID_TEXTURE_HEIGHT, 0, GL_RGBA, GL_FLOAT, texture.data());

    OpenGLHelpers::printAllOpenGlError();
}

void BeatGridMesh::drawGlObjects()
{
    glActiveTexture(GL_TEXTURE0); // <- might only be necessary on some GPUs
    glBindTexture(GL_TEXTURE_2D, tbo);
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, triangleIds.size(), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void BeatGridMesh::freeGlObjects()
{
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    glDeleteTextures(1, &tbo);
}

void BeatGridMesh::refreshGpuTexture()
{
    glBindTexture(GL_TEXTURE_2D, tbo);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, KHOLORS_BACKGROUND_GRID_TEXTURE_WIDTH,
                    KHOLORS_BACKGROUND_GRID_TEXTURE_HEIGHT, GL_RGBA, GL_FLOAT, texture.data());
    glBindTexture(GL_TEXTURE_2D, 0);
}

void BeatGridMesh::generateBeatGridTexture(int timeSignatureNumerator)
{
    float intensity = NAN;
    int const subdivWidth = KHOLORS_BACKGROUND_GRID_TEXTURE_WIDTH / timeSignatureNumerator;
    for (size_t x = 0; x < KHOLORS_BACKGROUND_GRID_TEXTURE_WIDTH; x++)
    {
        if (x < KHOLORS_BACKGROUND_GRID_LEVEL_0_WIDTH)
        {
            intensity = 1.0f;
        }
        else if ((x % static_cast<size_t>(subdivWidth)) < KHOLORS_BACKGROUND_GRID_LEVEL_1_WIDTH)
        {
            intensity = 0.75f;
        }
        else
        {
            intensity = 0.0f;
        }

        for (size_t y = 0; y < KHOLORS_BACKGROUND_GRID_TEXTURE_HEIGHT; y++)
        {
            auto const openGlTexelIndex = ((y * KHOLORS_BACKGROUND_GRID_TEXTURE_WIDTH) + x);
            texture[((openGlTexelIndex * TEXTURE_PIXEL_FLOAT_LEN) + 3)] = intensity;
        }
    }
}

void BeatGridMesh::updateGridPosition(int64_t viewPosition, int64_t viewScale, int64_t screenWidth, float bpm)
{
    int64_t const viewSampleWidth = viewScale * screenWidth;

    int64_t beatSampleWidth = ((static_cast<float>(VISUAL_SAMPLE_RATE)) * 60.0f) / bpm;
    if (isSuperGrid)
    {
        beatSampleWidth = beatSampleWidth * lastTimeSignatureNumerator;
    }
    int64_t const samplesToNextBeatBar = beatSampleWidth - (viewPosition % beatSampleWidth);

    // position of the grid in mesh coordinates
    float const gridPosLeftX = static_cast<float>(samplesToNextBeatBar) / static_cast<float>(viewSampleWidth);
    float const gridPosRightX =
        gridPosLeftX + (static_cast<float>(beatSampleWidth) / static_cast<float>(viewSampleWidth));

    if (std::abs(gridPosLeftX - gridPosRightX) < std::numeric_limits<float>::epsilon())
    {
        throw std::runtime_error("Beat grid has zero length");
    }

    float const alpha = 1.0f / (gridPosRightX - gridPosLeftX);
    float const beta = -gridPosLeftX;

    // position of the mesh in the texture coordinates
    float const meshPosLeftX = alpha * beta;
    float const meshPosRightX = alpha * (1.0f + beta);

    // upper left corner 0
    vertices[0].texturePosition[0] = meshPosLeftX;
    vertices[1].texturePosition[0] = meshPosRightX;
    vertices[2].texturePosition[0] = meshPosRightX;
    vertices[3].texturePosition[0] = meshPosLeftX;

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<long>(sizeof(Vertex) * vertices.size()), vertices.data(), GL_STATIC_DRAW);
    OpenGLHelpers::printAllOpenGlError();
}

void BeatGridMesh::setVisible(bool visible)
{
    if (visible != isShown)
    {
        isShown = visible;
        if (visible)
        {
            // upper left corner 0
            vertices[0].position[0] = -1.0f;
            vertices[0].position[1] = -1.0f;
            // upper right corner 1
            vertices[1].position[0] = 1.0f;
            vertices[1].position[1] = -1.0f;
            // lower right corner 2
            vertices[2].position[0] = 1.0f;
            vertices[2].position[1] = 1.0f;
            // lower left corner 3
            vertices[3].position[0] = -1.0f;
            vertices[3].position[1] = 1.0f;
        }
        else
        {
            // upper left corner 0
            vertices[0].position[0] = -11.0f;
            vertices[0].position[1] = -1.0f;
            // upper right corner 1
            vertices[1].position[0] = -10.0f;
            vertices[1].position[1] = -1.0f;
            // lower right corner 2
            vertices[2].position[0] = -10.0f;
            vertices[2].position[1] = 1.0f;
            // lower left corner 3
            vertices[3].position[0] = -11.0f;
            vertices[3].position[1] = 1.0f;
        }
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, static_cast<long>(sizeof(Vertex) * vertices.size()), vertices.data(),
                     GL_STATIC_DRAW);
        OpenGLHelpers::printAllOpenGlError();
    }
}