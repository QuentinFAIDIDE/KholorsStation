#pragma once

#include "juce_opengl/juce_opengl.h"
using namespace juce::gl;

// a structure to pass data to openGL as buffers
struct Vertex
{
    GLfloat position[3];
    GLfloat colour[3];
    GLfloat texturePosition[2];

    /**
     * @brief Register the format of this Vertex object
     * against openGL. Must be called after glBindBuffer/glBufferData
     * on the OpenGL thread.
     */
    static void registerVertexFormat()
    {
        // position
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)(offsetof(Vertex, position)));
        glEnableVertexAttribArray(0);
        // color
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)(offsetof(Vertex, colour)));
        glEnableVertexAttribArray(1);
        // texture
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)(offsetof(Vertex, texturePosition)));
        glEnableVertexAttribArray(2);
    }
};