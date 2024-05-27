#ifdef FB_TO_BB

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

struct Light
{
    uint type;
    vec3 color;
    vec3 direction;
    vec3 position;
};

layout(binding = 0, std140) uniform GlobalsParams
{
    vec3 uCamPosition;
    uint uLightCount;
    Light uLight[16];
};

in vec2 vTexCoord;

uniform sampler2D uAlbedo;
uniform sampler2D uNormals;
uniform sampler2D uPosition;
uniform sampler2D uViewDir;

uniform vec3 ssaoSamples[64];
uniform float sampleRadius;
uniform mat4 projectionMatrix;
uniform vec2 viewportSize;
uniform float ssaoBias;

layout(location = 0) out vec4 oColor; // aqui se podria añadir mas como onormals

void CalculateBlitVars(in Light light, out vec3 ambient, out vec3 diffuse, out vec3 specular)
{
    vec3 vNormal = texture(uNormals, vTexCoord).xyz;
    vec3 vViewDir = texture(uViewDir, vTexCoord).xyz;
    vec3 lightDir = normalize(light.direction);

    float ambientStrenght = 0.2;
    ambient = ambientStrenght * light.color;
			
    float diff = max(dot(vNormal,lightDir),0.0f);
    diffuse = diff * light.color;

    float specularStrenght = 0.1f;
    vec3 reflectDir = reflect(-lightDir,vNormal);
    vec3 normalViewDir = normalize(vViewDir);
    float spec = pow(max(dot(normalViewDir, reflectDir),0.0f),32);
    specular = specularStrenght * spec * light.color;

}

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
    vec4 textureColor = texture(uAlbedo, vTexCoord);
    vec4 finalColor = vec4(0.0f);

    for(int i = 0; i < uLightCount; ++i)
    {
        Light light = uLight[i];
        vec3 lightResult = vec3(0.0f);

        vec3 ambient = vec3(0.0);
        vec3 diffuse = vec3(0.0);
        vec3 specular = vec3(0.0);
        if(uLight[i].type == 0) //directional light
        {
        CalculateBlitVars(light, ambient, diffuse, specular);

        lightResult = ambient + diffuse + specular;
        finalColor += vec4(lightResult, 1.0) * textureColor;
			
        }
        else //point light, Todo podria ser una funcio
        {
            //Light light = uLight[i];

            float constant = 1.0f;
            float lineal = 0.09f;
            float quadratic = 0.032f;
            float distance = length(light.position - texture(uPosition, vTexCoord).xyz);
            float attenuation = 1.0 / (constant + lineal * distance + quadratic * (distance * distance));

            CalculateBlitVars(light, ambient, diffuse, specular);

            lightResult = (ambient * attenuation) + (diffuse * attenuation) + (specular * attenuation);
            finalColor += vec4(lightResult, 1.0) * textureColor;
        }
    }

    float occlusion = 0.0;

    vec3 vNormal = texture(uNormals, vTexCoord).xyz;
    vec3 tangent = cross(vNormal, vec3(0, 1, 0));
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

        occlusion += (samplePosView.z < sampledPosView.z - ssaoBias ? 1.0 : 0.0);
    }
    
    //oColor = finalColor;
    oColor = vec4(1.0 - occlusion / 64.0);
}

#endif
#endif