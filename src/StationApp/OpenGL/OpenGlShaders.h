#pragma once

#include <string>

std::string fftVertexShader =
    R"(
#version 330 core
layout (location = 0) in vec4 aPos;
layout (location = 1) in vec4 aColor;
layout (location = 2) in vec2 aTexCoord;

out vec4 ourColor;
out vec2 TexCoord;

uniform float viewPosition;
uniform float viewWidth;

void main()
{
    gl_Position = vec4( (2.0*((aPos.x-viewPosition)/viewWidth))-1.0, aPos.y, aPos.z, aPos.w);
    ourColor = aColor;
    TexCoord = aTexCoord;
}
)";

std::string fftFragmentShader =
    R"(
#version 330 core
out vec4 FragColor;
  
in vec4 ourColor;
in vec2 TexCoord;

uniform sampler2D sfftTexture;
uniform int convolutionId;

// Define kernels
#define identity mat3(0, 0, 0, 0, 1, 0, 0, 0, 0)
#define edge0 mat3(1, 0, -1, 0, 0, 0, -1, 0, 1)
#define edge1 mat3(0, 1, 0, 1, -4, 1, 0, 1, 0)
#define edge2 mat3(-1, -1, -1, -1, 8, -1, -1, -1, -1)
#define sharpen mat3(0, -1, 0, -1, 5, -1, 0, -1, 0)
#define box_blur mat3(1, 1, 1, 1, 1, 1, 1, 1, 1) * 0.1111
#define gaussian_blur mat3(1, 2, 1, 2, 4, 2, 1, 2, 1) * 0.0625
#define emboss mat3(-2, -1, 0, -1, 1, 1, 0, 1, 2)

// Find coordinate of matrix element from index
vec2 kpos(int index)
{
    return vec2[9] (
        vec2(-1, -1), vec2(0, -1), vec2(1, -1),
        vec2(-1, 0), vec2(0, 0), vec2(1, 0), 
        vec2(-1, 1), vec2(0, 1), vec2(1, 1)
    )[index] / vec2(64, 512).xy;
}

// Extract region of dimension 3x3 from sampler centered in uv
// sampler : texture sampler
// uv : current coordinates on sampler
// return : a mat3 with intensities
mat3 region3x3(sampler2D sampler, vec2 uv)
{
    // Create each pixels for region
    vec4[9] region;
    
    for (int i = 0; i < 9; i++)
        region[i] = texture(sampler, uv + kpos(i));

    // Create 3x3 region
    mat3 mRegion;
    
    mRegion = mat3(
        region[0][3], region[1][3], region[2][3],
        region[3][3], region[4][3], region[5][3],
        region[6][3], region[7][3], region[8][3]
    );
    
    return mRegion;
}

// Convolve a texture with kernel
// kernel : kernel used for convolution
// sampler : texture sampler
// uv : current coordinates on sampler
float convolution(mat3 kernel, sampler2D sampler, vec2 uv)
{
    float fragment;
    
    // Extract a 3x3 region centered in uv
    mat3 region = region3x3(sampler, uv);
    
    // component wise multiplication of kernel by region
    mat3 c = matrixCompMult(kernel, region);
    // add each component of matrix
    float r = c[0][0] + c[1][0] + c[2][0]
            + c[0][1] + c[1][1] + c[2][1]
            + c[0][2] + c[1][2] + c[2][2];
    return r;
}

void main()
{
    if (convolutionId == 0)
    {
        float intensity = texture(sfftTexture, TexCoord).a;
        FragColor = vec4(ourColor.x, ourColor.y, ourColor.z, intensity);
    }
    else
    {
        mat3 convolutionMat;
        switch (convolutionId) {
            case 1:
              convolutionMat = edge0;
              break;
            case 2:
              convolutionMat = edge1;
              break;
            case 3:
              convolutionMat = edge2;
              break;
            case 4:
              convolutionMat = sharpen;
              break;
            case 5:
              convolutionMat = box_blur;
              break;
            case 6:
              convolutionMat = gaussian_blur;
              break;
            case 7:
              convolutionMat = emboss;
              break;
        }
        float intensity = convolution(convolutionMat, sfftTexture, TexCoord);
        FragColor = vec4(ourColor.x, ourColor.y, ourColor.z, intensity);
    }
}
)";

std::string gridBackgroundVertexShader =
    R"(
#version 330 core
layout (location = 0) in vec4 aPos;
layout (location = 1) in vec4 aColor;
layout (location = 2) in vec2 aTexCoord;

out vec4 outColor;

void main()
{
    gl_Position = vec4(aPos.x, aPos.y, aPos.z, aPos.w);
    outColor = aColor;
}
)";

std::string gridBackgroundFragmentShader =
    R"(
#version 330 core
out vec4 FragColor;
  
in vec4 outColor;

uniform float viewHeightPixels;
uniform float grid0PixelWidth;
uniform int grid0PixelShift;
uniform float grid1PixelWidth;
uniform int grid1PixelShift;
uniform float grid2PixelWidth;
uniform int grid2PixelShift;

void main()
{
    float grid0position = (gl_FragCoord.x - float(grid0PixelShift)) / grid0PixelWidth;
    float grid1position = (gl_FragCoord.x - float(grid1PixelShift)) / grid1PixelWidth;
    float grid2position = (gl_FragCoord.x - float(grid2PixelShift)) / grid2PixelWidth;


    // vertical grid tempo bars
    if ( abs( grid0position - round(grid0position) )*grid0PixelWidth < 1.5 && grid0PixelWidth > 25 ) {
        FragColor = vec4(0.25,0.25,0.25,1.0);
    } else if ( abs( grid1position - round(grid1position) )*grid1PixelWidth < 1.5 && grid1PixelWidth > 25 ) {
        FragColor = vec4(0.15,0.15,0.15,1.0);
    } else if ( abs( grid2position - round(grid2position) )*grid2PixelWidth < 1.5 && grid2PixelWidth > 25 ) {
        FragColor = vec4(0.10,0.10,0.10,1.0);

    // background
    } else {
        FragColor = outColor;
    }
}
)";