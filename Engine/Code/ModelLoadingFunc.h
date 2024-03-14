#ifndef MODEL_LOADING_FUNC

#define MODEL_LOADING_FUNC

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <vector>
#include "Globals.h"



namespace ModelLoader
{
	struct VertexBufferAttribute
	{
		u8 location;
		u8 componentCount;
		u8 offset;
	};

	struct VertexBufferLayaout
	{
		std::vector<VertexBufferAttribute> attributes ;
		u8 stride;
	};

	struct VertexShaderAttribute
	{
		u8 location;
		u8 componentCount;
	};
	struct VertexShaderLayaout
	{
		std::vector<VertexShaderAttribute> attributes;
		
	};
	//VAO LINKA UNA COSA CON LA OTRA
	struct VAO
	{
		GLuint handle;
		GLuint programHandle;
	};
}


#endif // !MODEL_LOADING_FUNC
