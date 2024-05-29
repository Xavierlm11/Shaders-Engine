#ifdef SSAO

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
uniform sampler2D uNormals;
uniform sampler2D uPosition;

uniform vec3 ssaoSamples[64];
uniform float sampleRadius;
uniform mat4 projectionMatrix;
uniform vec2 viewportSize;
uniform float ssaoBias;
uniform sampler2D noiseTexture;
uniform bool useRangeCheck;

layout(location = 0) out vec4 oColor;

vec3 reconstructPixelPosition(float d, vec2 v)
{
    mat4 projectionMatrixInv = inverse(projectionMatrix);
    float xndc = gl_FragCoord.x / v.x * 2.0 - 1.0;
    float yndc = gl_FragCoord.y / v.y * 2.0 - 1.0;
    float zndc = d * 2.0 - 1.0;
    vec4 posNDC = vec4(xndc, yndc, xndc, 1.0);
    vec4 posView = projectionMatrixInv * posNDC;
    return posView.xyz / posView.w;
}

void main()
{
    float occlusion = 0.0;

    vec2 noiseScale = viewportSize / textureSize(noiseTexture, 0);
    vec3 randomVec = texture(noiseTexture, vTexCoord * noiseScale).xyz;

    vec3 vNormal = texture(uNormals, vTexCoord).xyz;
    vec3 tangent = normalize(randomVec - vNormal * dot(randomVec, vNormal));
    //vec3 tangent = cross(vNormal, vec3(0, 1, 0));
    vec3 bitangent = cross(vNormal, tangent);
    mat3 TBN = mat3(tangent, bitangent, vNormal);

    for (int i = 0; i < 64; i++)
    {
        vec3 offsetView = TBN * ssaoSamples[i];
        vec3 samplePosView = texture(uPosition, vTexCoord).xyz + offsetView * sampleRadius;
            
        vec4 sampleTexCoord = projectionMatrix * vec4(samplePosView, 1.0);
        sampleTexCoord.xyz /= sampleTexCoord.w;
        sampleTexCoord.xyz = sampleTexCoord.xyz * 0.5 + 0.5;

        float sampleDepth = texture(uPosition, sampleTexCoord.xy).z;
        vec3 sampledPosView = reconstructPixelPosition(sampleDepth, viewportSize);

        float rangeCheck = smoothstep(0.0, 1.0, sampleRadius / abs(samplePosView.z - sampledPosView.z));
        rangeCheck *= rangeCheck;
        if (!useRangeCheck) rangeCheck = 1.0;
        occlusion += (samplePosView.z < sampledPosView.z - ssaoBias ? 1.0 : 0.0) * rangeCheck;
    }
    
    oColor = vec4(1.0 - occlusion / 64.0);
}

#endif
#endif