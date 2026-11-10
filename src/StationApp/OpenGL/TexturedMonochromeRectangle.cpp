#include "TexturedMonochromeRectangle.h"

#include <algorithm>

#include "OpenGLHelpers.h"
#include "juce_opengl/opengl/juce_gl.h"
#include "spdlog/spdlog.h"

TexturedMonochromeRectangle::TexturedMonochromeRectangle(int64_t width, int64_t height, juce::Colour col)
    : textureWidth(width), textureHeight(height)
{
    vertices.reserve(4);

    halfTextureHeight = static_cast<size_t>(textureHeight >> 1);
    rowWidth = static_cast<size_t>(textureWidth);
    sideStrafeStep = 2 * static_cast<size_t>(textureWidth);

    // TODO: set proper position

    // upper left corner 0
    vertices.push_back(
        {{-1.0f, -1.0f, 0.0f}, {col.getFloatRed(), col.getFloatGreen(), col.getFloatBlue()}, {0.0f, 1.0f}});

    // upper right corner 1
    vertices.push_back(
        {{1.0f, -1.0f, 0.0f}, {col.getFloatRed(), col.getFloatGreen(), col.getFloatBlue()}, {1.0f, 1.0f}});

    // lower right corner 2
    vertices.push_back(
        {{1.0f, 1.0f, 0.0f}, {col.getFloatRed(), col.getFloatGreen(), col.getFloatBlue()}, {1.0f, 0.0f}});

    // lower left corner 3
    vertices.push_back(
        {{-1.0f, 1.0f, 0.0f}, {col.getFloatRed(), col.getFloatGreen(), col.getFloatBlue()}, {0.0f, 0.0f}});

    // lower left triangle
    triangleIds.push_back(0);
    triangleIds.push_back(2);
    triangleIds.push_back(3);

    // upper right triangle
    triangleIds.push_back(0);
    triangleIds.push_back(1);
    triangleIds.push_back(2);

    texture.resize(static_cast<size_t>(width * height));
    std::ranges::fill(texture, 0.0f);
}

TexturedMonochromeRectangle::~TexturedMonochromeRectangle() = default;

void TexturedMonochromeRectangle::registerGlObjects()
{
    spdlog::debug("Registering an OpenGL textured mesh");

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
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // set the filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, textureWidth, textureHeight, 0, GL_RED, GL_FLOAT, texture.data());

    OpenGLHelpers::printAllOpenGlError();

    // after everything was uplaoded we can reset the nonce
    int64_t const newNonce = textureNonce;
    lastUploadedTextureNonce = newNonce;
}

void TexturedMonochromeRectangle::drawGlObjects()
{
    glActiveTexture(GL_TEXTURE0); // <- might only be necessary on some GPUs
    glBindTexture(GL_TEXTURE_2D, tbo);
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, triangleIds.size(), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void TexturedMonochromeRectangle::freeGlObjects()
{
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    glDeleteTextures(1, &tbo);
}

void TexturedMonochromeRectangle::refreshGpuTextureIfChanged()
{
    if (textureNonce != lastUploadedTextureNonce)
    {
        lastUploadedTextureNonce = textureNonce;
        glBindTexture(GL_TEXTURE_2D, tbo);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, textureWidth, textureHeight, GL_RED, GL_FLOAT, texture.data());
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

void TexturedMonochromeRectangle::changeColor(juce::Colour newColor)
{
    for (size_t i = 0; i < 4; i++)
    {
        vertices[i].colour[0] = newColor.getFloatRed();
        vertices[i].colour[1] = newColor.getFloatGreen();
        vertices[i].colour[2] = newColor.getFloatBlue();
    }

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<long>(sizeof(Vertex) * vertices.size()), vertices.data(), GL_STATIC_DRAW);
    OpenGLHelpers::printAllOpenGlError();
}

void TexturedMonochromeRectangle::setPosition(int64_t viewPositionSamples, int64_t width, uint64_t trackIdentifier)
{
    uint64_t maxUint64 = 0;
    maxUint64 -= 1;

    // upper left corner 0
    vertices[0].position[0] = viewPositionSamples;
    vertices[0].position[2] = static_cast<float>(trackIdentifier) / static_cast<float>(maxUint64);

    // upper right corner 1
    vertices[1].position[0] = viewPositionSamples + width;
    vertices[1].position[2] = static_cast<float>(trackIdentifier) / static_cast<float>(maxUint64);

    // lower right corner 2
    vertices[2].position[0] = viewPositionSamples + width;
    vertices[2].position[2] = static_cast<float>(trackIdentifier) / static_cast<float>(maxUint64);

    // lower left corner 3
    vertices[3].position[0] = viewPositionSamples;
    vertices[3].position[2] = static_cast<float>(trackIdentifier) / static_cast<float>(maxUint64);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<long>(sizeof(Vertex) * vertices.size()), vertices.data(), GL_STATIC_DRAW);
    OpenGLHelpers::printAllOpenGlError();
}

void TexturedMonochromeRectangle::setPixelAt(int x, int y, float intensity)
{
    float icorr = intensity;
    icorr = std::max(icorr, 0.0f);
    icorr = std::min(icorr, 1.0f);
    texture[(static_cast<size_t>((y * textureWidth) + x))] = icorr;
    textureNonce++;
}

void TexturedMonochromeRectangle::setRepeatedVerticalHalfLine(int channel, size_t startX, size_t endX,
                                                              const float *intensities)
{
    size_t const widthX = (1 + (endX - startX));

    // we start to draw in dst at the top x pixel
    // we start to read from src at the last (highest frequency) intensity
    float const *srcIntensityPtr = intensities + (halfTextureHeight - 1);
    float *dstTexelPtr = texture.data() + startX;

    sideStrafe = (static_cast<size_t>(textureHeight) - 1) * static_cast<size_t>(textureWidth);

    for (size_t i = 0; i < halfTextureHeight; i++)
    {
        if (channel == 0 || channel == 2)
        {
            for (size_t x = startX; x <= endX; x++)
            {
                *dstTexelPtr = *srcIntensityPtr;
                dstTexelPtr += 1;
            }
            dstTexelPtr -= widthX;
        }

        if (channel == 1 || channel == 2)
        {
            dstTexelPtr += sideStrafe;

            for (size_t x = startX; x <= endX; x++)
            {
                *dstTexelPtr = *srcIntensityPtr;
                dstTexelPtr += 1;
            }
            dstTexelPtr -= widthX;

            dstTexelPtr -= sideStrafe;
        }

        sideStrafe -= sideStrafeStep;
        dstTexelPtr += rowWidth;
        srcIntensityPtr--;
    }
    textureNonce++;
}

void TexturedMonochromeRectangle::clearAllData()
{
    std::ranges::fill(texture, 0.0f);
    textureNonce++;
}
