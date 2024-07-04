#pragma once

#include "juce_opengl/juce_opengl.h"
using namespace juce::gl;

// a structure to pass data to openGL as buffers
struct Vertex
{
    float position[3];
    float colour[3];
    float texturePosition[2];

    /**
     * @brief Register the format of this Vertex object
     * against openGL. Must be called after glBindBuffer/glBufferData
     * on the OpenGL thread.
     */
    static void registerVertexFormat()
    {
        // position
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
        glEnableVertexAttribArray(0);
        // color
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        // texture
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);
    }
};