#version 330 core
#define MAX_TEX_PER_TYPE 4

out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D DIFFUSE[MAX_TEX_PER_TYPE];
uniform sampler2D SPECULAR[MAX_TEX_PER_TYPE];

uniform vec4 delete_colors[8];
uniform vec4 replace_colors[8];
uniform float tolerances[8];
uniform int num_colors;

void main()
{
    vec4 texColor = texture(DIFFUSE[0], TexCoord);
    vec4 finalColor = texColor;
    
   for(int i = 0; i < num_colors && i < 8; i++)
    {
        float dist = distance(texColor.rgb, delete_colors[i].rgb);
        float alpha_mask = smoothstep(0.0, tolerances[i], dist);
        
        finalColor = mix( replace_colors[i], finalColor, alpha_mask);
    }
    
    FragColor = finalColor;
}