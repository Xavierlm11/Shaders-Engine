#ifdef GRID_SHADER

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location = 0) in vec3 aPosition;

out vec2 vTexCoord;
void main()
{
	vTexCoord = aPosition.xy*0.5 + vec2(0.5);
	gl_Position = vec4(aPosition,1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////



in vec2 vTexCoord;

uniform float left;
uniform float right;
uniform float bottom;
uniform float top;
uniform float znear;
uniform float worldMatrix;
uniform float viewMatrix;


layout(location = 0) out vec4 oColor; // aqui se podria a�adir mas como onormals

float grid(vec3 worldPos, float gridStep)
{
	vec2 grid = fwidth(worldPos.xz)/mod(worldPos.xz,gridStep);
	float line = step(1.0,max(grid.x,grid.y) );
	return line;
}

void main()
{
	vec4 finalColor = vec4(1.0);
	vec3 eyeDirSpace;
	eyeDirSpace.x = left + vTexCoord.x * (right-left);
	eyeDirSpace.y = bottom + vTexCoord.y * (top-bottom);
	eyeDirSpace.z = -znear;
	vec3 eyeDirWorldSpace = normalize(mat3(worldMatrix)*);

	vec3 eyePosEyeSpace = vec3(0.0);
	vec3 eyePosWorldSpace = vec3(worldMatrix * vec4(eyePosEyeSpace));

	vec3 planeNormalWorldSpace = vec3(0.0,1.0,0.0);
	vec3 planePointWorldSpace = vec3(0.0);

	float numerator = dot(planePointWorldSpace,planeNormalWorldSpace);
	float denominator = dot(eyeDirWorldSpace,planeNormalWorldSpace);
	float t = numerator/denominator;

	if(t>0.0)
	{
	vec3 hitWorldSapce = eyePosWorldSpace + eyeDirWorldSpace * t;
	finalColor=vec4(grid(hitWorldSapce,1.0));

	}
	else
	{
		gl_FragDepth = 0.0;
		discard;
	}
}


#endif
#endif