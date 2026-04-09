#version 330 core
#define MAX_TEX_PER_TYPE 16
#define MAX_MATERIALS 128

in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;

uniform vec3 viewPos;
uniform int TEX_COUNTS[];

uniform sampler2D DIFFUSE[MAX_TEX_PER_TYPE];
uniform sampler2D SPECULAR[MAX_TEX_PER_TYPE];

uniform int material_index;
struct Material
    {
	    // every vec3 needs a float pad after it in std140
	    vec3 ambient;
	    float shininess;

	    vec3 diffuse;
	    float opacity;

	    vec3 specular;
	    float index_of_refraction;//how much light bends when entering the material

	    vec3 emission;//object emits light		
	    float illumination_model;//illumination model used by the material 
    };
layout(std140) uniform Material_block
{
    Material materials[MAX_MATERIALS];
};

struct Light{
    bool has_a_source;
    vec3 light_pos;
    vec3 light_target;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float cos_soft_cut_off_angle;
    float cos_hard_cut_off_angle;

    float constant;
    float linear;
    float quadratic;
};

uniform int num_of_lights;
uniform Light lights[16];

out vec4 FragColor;
void main()
{
     vec3 diff_map_val;
     vec3 spec_val;
     float shininess;
    if(material_index < 0 || material_index >= MAX_MATERIALS)
    {
        
        diff_map_val = vec3(texture(DIFFUSE[0], TexCoord));
        spec_val = vec3(texture(SPECULAR[0],TexCoord));
        shininess = 1;
    }
    else
    {
        Material material = materials[material_index];
        diff_map_val = material.diffuse;
        spec_val = material.specular;
        shininess = material.shininess;
    }
    
	vec3 norm = normalize(Normal);
    vec3 total_light = vec3(0.0, 0.0, 0.0);
    

    for(int i = 0; i < min(num_of_lights,16); i++)
    {
        vec3 lightDir = lights[i].has_a_source ? normalize(lights[i].light_pos - FragPos) :  normalize(-(lights[i].light_target)); 

        float light_power = 1.0f;
        if(lights[i].has_a_source)
        {
            float cos_theta = dot(-lightDir, normalize(lights[i].light_target));
            if(cos_theta > lights[i].cos_hard_cut_off_angle)
            {
                float distance = length(lights[i].light_pos - FragPos);
                light_power = 1.0 / (lights[i].constant + lights[i].linear * distance +
                            lights[i].quadratic * (distance * distance));
                if(cos_theta < lights[i].cos_soft_cut_off_angle)
                {
                    float epsilon = lights[i].cos_soft_cut_off_angle - lights[i].cos_hard_cut_off_angle;
                    light_power *= clamp((cos_theta - lights[i].cos_hard_cut_off_angle) / epsilon, 0.0, 1.0);
                }
            }
            else
            {
                light_power = 0.0;
            }
        }

        vec3 ambient_calc = lights[i].ambient * diff_map_val;

        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diff_calc = lights[i].diffuse * diff * diff_map_val;

        vec3 spec_calc = vec3(0,0,0);
        if(spec_val.x + spec_val.y + spec_val.z > 0)
        {
            vec3 reflectDir = reflect(-lightDir, norm);
            vec3 viewDir = normalize(viewPos - FragPos);
            float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
            spec_calc = lights[i].specular * spec_val * spec;
        }

        total_light += (ambient_calc + (diff_calc + spec_calc) * light_power);
    }

    FragColor = vec4(total_light, 1.0f);
}