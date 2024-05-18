#ifdef RENDER_TO_FB

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

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

layout(binding = 1,std140) uniform localParams
{
	mat4 uWorldMatrix;
	mat4 uWorldViewProjectionMatrix;
};

out vec2 vTexCoord;
out vec3 vPosition;
out vec3 vNormal;
out vec3 vViewDir;

void main()
{
	vTexCoord = aTexCoord;
	vPosition = vec3(uWorldMatrix * vec4(aPosition, 1.0));
	vNormal =  vec3(uWorldMatrix * vec4(aNormal, 0.0));
	vViewDir = uCamPosition - vPosition;
	gl_Position = uWorldViewProjectionMatrix * vec4(aPosition, 1.0);
	
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
in vec3 vPosition;
in vec3 vNormal;
in vec3 vViewDir;


uniform sampler2D uTexture;
layout(location = 0) out vec4 oAlbedo; // aqui se podria a�adir mas como onormals
layout(location = 1) out vec4 oNormals;
layout(location = 2) out vec4 oPosition;
layout(location = 3) out vec4 oViewDir;
layout(location = 4) out vec4 oDepth;

uniform float near = 0.1f;  // Plano cercano de la cámara
uniform float far = 100.0f;   // Plano lejano de la cámara
float LinearizeDepth(float depth)
{
    float z = depth * 2.0 - 1.0; 
    return (2.0 * near * far) / (far + near - z * (far - near));
}

void main()
{
	oAlbedo = texture(uTexture, vTexCoord);
	oNormals = vec4(vNormal, 1.0);
	oPosition = vec4(vPosition, 1.0);
	oViewDir = vec4(vViewDir,1.0);
	oDepth = vec4(vec3(LinearizeDepth(gl_FragCoord.z) / far), 1.0f);
}

#endif
#endif