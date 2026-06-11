#version 330 core
#define MAX_TEX_PER_TYPE 4
#define MAX_MATERIALS 128
#define MAX_LIGHTS 16

in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;
in vec3 Tangent; // optional: if your vertex shader provides tangent, use it for proper normal mapping
in vec3 Bitangent;

uniform vec3 viewPos;
uniform int TEX_COUNTS[];

// legacy
uniform sampler2D DIFFUSE[MAX_TEX_PER_TYPE];
uniform sampler2D NORMAL[MAX_TEX_PER_TYPE];
uniform sampler2D SPECULAR[MAX_TEX_PER_TYPE];

// PBR
uniform sampler2D BASECOLOR[MAX_TEX_PER_TYPE];
uniform sampler2D METALLICROUGHNESS[MAX_TEX_PER_TYPE]; // B=metallic, G=roughness (glTF packing)
uniform sampler2D OCCLUSION[MAX_TEX_PER_TYPE];
uniform sampler2D EMISSIVE[MAX_TEX_PER_TYPE];

uniform int material_index;
struct Material
{
    vec3 ambient;
    float shininess;

    vec3 diffuse;
    float opacity;

    vec3 specular;
    float index_of_refraction;

    vec3 emission;
    float illumination_model;
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
uniform Light lights[MAX_LIGHTS];

out vec4 FragColor;

// --- helper constants for TEX_COUNTS indices (must match CPU)
const int DIFFUSE_IDX = 0;
const int NORMAL_IDX = 1;
const int SPECULAR_IDX = 2;
const int BASECOLOR_IDX = 3;
const int METALLICROUGHNESS_IDX = 4;
const int OCCLUSION_IDX = 5;
const int EMISSIVE_IDX = 6;

// --- PBR helper functions (GGX / Schlick)
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N,H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = 3.14159265 * denom * denom;
    return a2 / max(denom, 1e-6);
}

float GeometrySchlickGGX(float NdotV, float k)
{
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float k)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx1 = GeometrySchlickGGX(NdotV, k);
    float ggx2 = GeometrySchlickGGX(NdotL, k);
    return ggx1 * ggx2;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

// --- utility: average up to MAX_TEX_PER_TYPE textures of a sampler array
vec4 sampleAndAverage(sampler2D arr[MAX_TEX_PER_TYPE], int count, vec2 uv)
{
    if(count <= 0) return vec4(0.0);
    vec4 acc = vec4(0.0);
    for(int i = 0; i < count; ++i)
        acc += texture(arr[i], uv);
    return acc / float(count);
}

void main()
{
    // --- gather material factors and textures
    Material material = (material_index >= 0 && material_index < MAX_MATERIALS) ? materials[material_index] : Material(vec3(0.03), 32.0, vec3(1.0), 1.0, vec3(1.0), 1.0, vec3(0.0), 0.0);

    // legacy maps
    vec3 legacy_diff = vec3(0.0);
    if(TEX_COUNTS[DIFFUSE_IDX] > 0) legacy_diff = vec3(sampleAndAverage(DIFFUSE, TEX_COUNTS[DIFFUSE_IDX], TexCoord));
    vec3 legacy_spec = vec3(0.0);
    if(TEX_COUNTS[SPECULAR_IDX] > 0) legacy_spec = vec3(sampleAndAverage(SPECULAR, TEX_COUNTS[SPECULAR_IDX], TexCoord));
    vec3 legacy_normal_map = vec3(0.0);
    if(TEX_COUNTS[NORMAL_IDX] > 0) legacy_normal_map = vec3(sampleAndAverage(NORMAL, TEX_COUNTS[NORMAL_IDX], TexCoord));

    // PBR maps
    vec4 baseColorTex = vec4(1.0);
    if(TEX_COUNTS[BASECOLOR_IDX] > 0) baseColorTex = sampleAndAverage(BASECOLOR, TEX_COUNTS[BASECOLOR_IDX], TexCoord);
    vec3 mrTex = vec3(0.0); // r unused, g=roughness, b=metallic (common glTF packing)
    if(TEX_COUNTS[METALLICROUGHNESS_IDX] > 0) mrTex = sampleAndAverage(METALLICROUGHNESS, TEX_COUNTS[METALLICROUGHNESS_IDX], TexCoord).rgb;
    float ao = 1.0;
    if(TEX_COUNTS[OCCLUSION_IDX] > 0) ao = sampleAndAverage(OCCLUSION, TEX_COUNTS[OCCLUSION_IDX], TexCoord).r;
    vec3 emissive = vec3(0.0);
    if(TEX_COUNTS[EMISSIVE_IDX] > 0) emissive = sampleAndAverage(EMISSIVE, TEX_COUNTS[EMISSIVE_IDX], TexCoord).rgb;

    // --- derive final material parameters (use PBR if baseColor or MR present)
    bool hasPBR = (TEX_COUNTS[BASECOLOR_IDX] > 0) || (TEX_COUNTS[METALLICROUGHNESS_IDX] > 0);
    vec3 baseColor = hasPBR ? baseColorTex.rgb * material.diffuse : (TEX_COUNTS[DIFFUSE_IDX] > 0 ? legacy_diff * material.diffuse : material.diffuse);
    float opacity = hasPBR ? baseColorTex.a * material.opacity : material.opacity;
    float metallic = hasPBR ? mrTex.b : 0.0;
    float roughness = hasPBR ? mrTex.g : clamp(1.0 - material.shininess / 256.0, 0.04, 1.0); // fallback mapping
    roughness = clamp(roughness, 0.04, 1.0);

    // --- normal: prefer tangent-space normal map if tangent provided, otherwise fallback to interpolated normal
    vec3 N = normalize(Normal);
    if(TEX_COUNTS[NORMAL_IDX] > 0)
    {
        vec3 nmap = legacy_normal_map;
        // decode normal map from [0,1] to [-1,1]
        vec3 n_tangent = normalize(nmap * 2.0 - 1.0);
        // if TBN available, transform
        #ifdef HAS_TBN
        mat3 TBN = mat3(normalize(Tangent), normalize(Bitangent), normalize(Normal));
        N = normalize(TBN * n_tangent);
        #else
        // approximate: perturb normal in object space using tangent/bitangent if available; otherwise blend
        // if Tangent/Bitangent are zero, fallback to using interpolated normal
        if(length(Tangent) > 0.0)
        {
            mat3 TBN = mat3(normalize(Tangent), normalize(Bitangent), normalize(Normal));
            N = normalize(TBN * n_tangent);
        }
        else
        {
            // fallback: treat normal map as a small perturbation
            N = normalize(mix(N, n_tangent, 0.5));
        }
        #endif
    }

    // --- reflectance at normal incidence
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, baseColor, metallic);

    vec3 V = normalize(viewPos - FragPos);

    // --- lighting loop (PBR)
    vec3 Lo = vec3(0.0);
    for(int i = 0; i < min(num_of_lights, MAX_LIGHTS); ++i)
    {
        // compute light direction and attenuation
        vec3 L;
        float attenuation = 1.0;
        if(lights[i].has_a_source)
        {
            L = normalize(lights[i].light_pos - FragPos);
            float distance = length(lights[i].light_pos - FragPos);
            attenuation = 1.0 / max(0.0001, lights[i].constant + lights[i].linear * distance + lights[i].quadratic * distance * distance);

            float cos_theta = dot(-L, normalize(lights[i].light_target));
            if(cos_theta <= lights[i].cos_hard_cut_off_angle) { continue; }
            if(cos_theta < lights[i].cos_soft_cut_off_angle)
            {
                float eps = lights[i].cos_soft_cut_off_angle - lights[i].cos_hard_cut_off_angle;
                attenuation *= clamp((cos_theta - lights[i].cos_hard_cut_off_angle) / eps, 0.0, 1.0);
            }
        }
        else
        {
            // directional: use target as direction
            L = normalize(-lights[i].light_target);
        }

        vec3 H = normalize(V + L);
        float NdotL = max(dot(N, L), 0.0);
        float NdotV = max(dot(N, V), 0.0);
        float NdotH = max(dot(N, H), 0.0);
        float VdotH = max(dot(V, H), 0.0);

        // Cook-Torrance BRDF
        float D = DistributionGGX(N, H, roughness);
        float k = (roughness + 1.0) * (roughness + 1.0) / 8.0; // UE4 remap
        float G = GeometrySmith(N, V, L, k);
        vec3 F = FresnelSchlick(VdotH, F0);

        vec3 numerator = D * G * F;
        float denom = 4.0 * max(0.001, NdotV * NdotL);
        vec3 specular = numerator / max(denom, 1e-6);

        // kS is energy preserved specular, kD is diffuse component
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;

        // Lambertian diffuse (albedo / PI)
        vec3 diffuse = (baseColor / 3.14159265);

        vec3 radiance = lights[i].diffuse * attenuation;

        Lo += (kD * diffuse + specular) * radiance * NdotL;
    }

    // ambient + ao + emissive
    vec3 ambient = material.ambient * baseColor * 0.03;
    vec3 color = ambient + Lo;
    color = color * ao + emissive + material.emission;

    // legacy specular fallback (if no PBR maps and legacy spec present)
    if(!hasPBR && TEX_COUNTS[SPECULAR_IDX] > 0)
    {
        // simple Blinn-Phong add-on to keep compatibility
        vec3 legacy_specular = legacy_spec;
        vec3 norm = normalize(Normal);
        vec3 total_light = vec3(0.0);
        for(int i = 0; i < min(num_of_lights, MAX_LIGHTS); ++i)
        {
            vec3 L = lights[i].has_a_source ? normalize(lights[i].light_pos - FragPos) : normalize(-lights[i].light_target);
            float NdotL = max(dot(norm, L), 0.0);
            vec3 viewDir = normalize(viewPos - FragPos);
            vec3 reflectDir = reflect(-L, norm);
            float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
            vec3 diff = lights[i].diffuse * NdotL * (TEX_COUNTS[DIFFUSE_IDX] > 0 ? legacy_diff : material.diffuse);
            vec3 specC = lights[i].specular * legacy_specular * spec;
            total_light += diff + specC;
        }
        color = mix(color, total_light, 0.5);
    }

    // gamma correction (assume framebuffer expects sRGB)
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));

    FragColor = vec4(color, opacity);
}
