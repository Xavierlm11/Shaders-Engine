#ifdef Blur

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;

out vec2 vTexCoord;
void main()
{
    vTexCoord = aTexCoord;
    gl_Position = vec4(aPosition,1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

in vec2 vTexCoord;
uniform sampler2D ssaoTexture;

layout(location = 0) out vec4 outColor;

void main()
{
    vec2 texelSize = 1.0 / vec2(textureSize(ssaoTexture, 0));
    float result = 0.0;
    for (int x = -2; x < 2; ++x) 
    {
        for (int y = -2; y < 2; ++y) 
        {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            result += texture(ssaoTexture, vTexCoord + offset).z;
        }
    }
    outColor = vec4(result / (4.0 * 4.0));
}

#endif
#endif