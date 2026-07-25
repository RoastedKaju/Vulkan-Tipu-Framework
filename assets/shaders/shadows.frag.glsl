#version 460
#extension GL_EXT_nonuniform_qualifier: require

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec2 inUV;
layout (location = 2) in flat uint inAlbedo;
layout (location = 3) in vec3 inLightDirView;
layout (location = 4) in vec3 inFragPosView;
layout (location = 5) in vec4 inFragPosLightSpace;
layout (location = 6) in flat uint inShadowMap;

layout (set = 0, binding = 0) uniform sampler2D bindless_textures[];

layout (location = 0) out vec4 outColor;

const vec3 LIGHT_COLOR = vec3(1.0, 0.98, 0.95);
const vec3 AMBIENT_COLOR = vec3(0.15, 0.15, 0.18);
const float AMBIENT_STRENGTH = 0.25;
const float SPECULAR_STRENGTH = 0.5;
const float SHININESS = 32.0;

float calculate_shadow(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir) {
    // Perspective divide -> NDC in [-1, 1] on xy, [0, 1] on z (Vulkan depth range).
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords.xy = projCoords.xy * 0.5 + 0.5;

    // Outside the light's frustum: treat as lit.
    if (projCoords.x < 0.0 || projCoords.x > 1.0 ||
    projCoords.y < 0.0 || projCoords.y > 1.0 ||
    projCoords.z < 0.0 || projCoords.z > 1.0) {
        return 0.0;
    }

    // Slope-scaled bias to fight shadow acne.
    float bias = max(0.0025 * (1.0 - dot(normal, lightDir)), 0.0005);

    float shadow = 0.0;
    vec2 texel_size = 1.0 / vec2(textureSize(bindless_textures[nonuniformEXT(inShadowMap)], 0));
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float closest_depth = texture(bindless_textures[nonuniformEXT(inShadowMap)],
                                          projCoords.xy + vec2(x, y) * texel_size).r;
            shadow += (projCoords.z - bias) > closest_depth ? 1.0 : 0.0;
        }
    }
    return shadow / 9.0;
}

void main() {
    vec4 tex_color = texture(bindless_textures[nonuniformEXT(inAlbedo)], inUV);
    if(tex_color.a < 0.1) discard;

    vec3 normal = normalize(inNormal);
    vec3 lightDir = normalize(-inLightDirView);

    vec3 ambient = AMBIENT_COLOR * AMBIENT_STRENGTH;

    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * LIGHT_COLOR;

    vec3 viewDir = normalize(-inFragPosView);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), SHININESS);
    vec3 specular = SPECULAR_STRENGTH * spec * LIGHT_COLOR;

    float shadow = calculate_shadow(inFragPosLightSpace, normal, lightDir);

    vec3 lighting = ambient + (1.0 - shadow) * (diffuse + specular);

    // Final color output
    vec3 finalColor = tex_color.rgb * lighting * 2.0;
    outColor = vec4(finalColor, tex_color.a);
}