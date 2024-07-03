#include "TexturedRectangle.h"
#include "StationApp/OpenGL/GLInfoLogger.h"
#include "juce_opengl/opengl/juce_gl.h"

TexturedRectangle::TexturedRectangle(int64_t width, int64_t height, juce::Colour col)
    : textureWidth(width), textureHeight(height)
{
    vertices.reserve(4);

    // TODO: set proper position

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

    texture.resize((size_t)(width * height * TEXTURE_PIXEL_FLOAT_LEN));
    std::fill(texture.begin(), texture.end(), 0.0f);
}

TexturedRectangle::~TexturedRectangle()
{
}

void TexturedRectangle::registerGlObjects()
{
    spdlog::debug("Registering an OpenGL textured mesh");

    // generate objects
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    printAllOpenGlError();

    glBindVertexArray(vao);

    // register and upload the vertices data
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(sizeof(Vertex) * vertices.size()), vertices.data(), GL_STATIC_DRAW);
    // register and upload indices of the vertices to form the triangles

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(sizeof(unsigned int) * triangleIds.size()), triangleIds.data(),
                 GL_STATIC_DRAW);

    // register the vertex attribute format
    Vertex::registerVertexFormat();
    printAllOpenGlError();

    // register the texture
    glGenTextures(1, &tbo);
    glBindTexture(GL_TEXTURE_2D, tbo);
    // set the texture wrapping
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // set the filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // send the texture to the gpu
    glTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA, textureWidth, textureHeight, 0, GL_RGBA, GL_FLOAT,
                 texture.data());

    printAllOpenGlError();

    // after everything was uplaoded we can reset the nonce
    int64_t newNonce = textureNonce;
    lastUploadedTextureNonce = newNonce;
}

void TexturedRectangle::drawGlObjects()
{
    glActiveTexture(GL_TEXTURE0); // <- might only be necessary on some GPUs
    glBindTexture(GL_TEXTURE_2D, tbo);
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, triangleIds.size(), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void TexturedRectangle::freeGlObjects()
{
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    glDeleteTextures(1, &tbo);
}

void TexturedRectangle::refreshGpuTextureIfChanged()
{
    if (textureNonce != lastUploadedTextureNonce)
    {
        lastUploadedTextureNonce = textureNonce;
        glBindTexture(GL_TEXTURE_2D, tbo);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, textureWidth, textureHeight, GL_RGBA, GL_FLOAT, texture.data());
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

void TexturedRectangle::changeColor(juce::Colour newColor)
{
    for (size_t i = 0; i < 4; i++)
    {
        vertices[i].colour[0] = newColor.getFloatRed();
        vertices[i].colour[1] = newColor.getFloatGreen();
        vertices[i].colour[2] = newColor.getFloatBlue();
        vertices[i].colour[3] = 0.0f;
    }

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, (long)(sizeof(Vertex) * vertices.size()), vertices.data(), GL_STATIC_DRAW);
    printAllOpenGlError();
}

void TexturedRectangle::setPosition(int64_t viewPositionSamples, int64_t width)
{
    // upper left corner 0
    vertices[0].position[0] = viewPositionSamples;

    // upper right corner 1
    vertices[1].position[0] = viewPositionSamples + width;

    // lower right corner 2
    vertices[2].position[0] = viewPositionSamples + width;

    // lower left corner 3
    vertices[3].position[0] = viewPositionSamples;

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, (long)(sizeof(Vertex) * vertices.size()), vertices.data(), GL_STATIC_DRAW);
    printAllOpenGlError();
}

void TexturedRectangle::setPixelAt(int x, int y, float intensity)
{
    size_t openGlTexelIndex = (size_t)((y * textureWidth) + x);
    // NOTE: we only modify the alpha value (last of the four float)
    texture[(size_t)((openGlTexelIndex * TEXTURE_PIXEL_FLOAT_LEN) + 3)] = juce::jlimit(0.0f, 1.0f, intensity);
    textureNonce++;
}

void TexturedRectangle::clearAllData()
{
    for (size_t i = 0; i < (size_t)textureWidth; i++)
    {
        for (size_t j = 0; j < (size_t)textureHeight; j++)
        {
            setPixelAt(i, j, 0.0f);
        }
    }
}