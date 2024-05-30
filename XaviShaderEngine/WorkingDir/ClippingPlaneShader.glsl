#ifdef ClippingPlaneShader

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;

layout(binding=1,std140) uniform localParams
{
    mat4 uWorldMatrix;
    mat4 uWorldViewProjectionMatrix;
};

uniform vec4 clippingPlane;
uniform mat4 projectionMatrix;

out vec2 vTexCoord;
void main()
{
    vTexCoord = aTexCoord;
    gl_Position = vec4(aPosition,1.0);
    gl_ClipDistance[0] = dot(uWorldViewProjectionMatrix * vec4(aPosition, 1.0), clippingPlane);
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
    oColor = finalColor;
}

#endif
#endif