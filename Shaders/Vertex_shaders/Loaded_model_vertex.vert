#version 330 core
#define MAX_BONES 100
#define MAX_BONES_PER_VERTEX 4

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec3 aNormal;
layout (location = 3) in ivec4 boneIDs;
layout (location = 4) in vec4 bone_weights;

layout (location = 5) in mat4 model;// location 5,6,7,8 for mat4
layout (location = 9) in vec4 line1;// location 9,10,11 for mat3
layout (location = 10) in vec4 line2;// location 9,10,11 for mat3
layout (location = 11) in float line3;// location 9,10,11 for mat3

layout (std140) uniform projectionXview_block
{
	mat4 projectionXview;
};

out vec2 TexCoord;
out vec3 Normal;
out vec3 FragPos;

void main()
{	
	mat3 transpose_inverse_model = mat3(
		line1.x, line1.y, line1.z,
		line1.w, line2.x, line2.y,
		line2.z, line2.w, line3
	);

	Normal =  transpose_inverse_model * aNormal;
	TexCoord = aTexCoord;
	vec4 worldPos = model * vec4(aPos, 1.0);
	FragPos = vec3(worldPos);
	gl_Position = projectionXview * worldPos;
	
}