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

layout (std140) uniform Bone_block
{
	 mat4 boneTransforms[MAX_BONES];
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
	TexCoord = aTexCoord;

	vec4 totalPosition = vec4(0.0f);
	vec3 totalNormal = vec3(0.0);
    for(int i = 0 ; i < MAX_BONES_PER_VERTEX ; i++)
    {
		int id = int(boneIDs[i]);
        if(id == -1) 
            continue;
        if(id >=MAX_BONES) 
        {
            totalPosition = vec4(aPos,1.0f);
            break;
        }

        vec4 localPosition = boneTransforms[int(id)] * vec4(aPos,1.0f);
        totalPosition += localPosition * bone_weights[i];

        mat3 boneRot = mat3(boneTransforms[int(id)]); // only rotation+scale
		totalNormal += boneRot * aNormal * bone_weights[i];
    }

	Normal = transpose_inverse_model * normalize(totalNormal);

	vec4 worldPos = model * totalPosition;
	FragPos = vec3(worldPos);
	gl_Position = projectionXview * worldPos;
	
}