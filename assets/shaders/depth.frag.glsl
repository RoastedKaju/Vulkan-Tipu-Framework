#version 460
#extension GL_EXT_nonuniform_qualifier: require

layout (location = 0) in vec2 inUV;
layout (location = 1) in flat uint inAlbedo;

layout (set = 0, binding = 0) uniform sampler2D bindless_textures[];

void main() {
    // Alpha-test so cutout geometry (leaves, fences, etc.) still casts
    // correctly-shaped shadows instead of a solid quad silhouette.
    float alpha = texture(bindless_textures[nonuniformEXT(inAlbedo)], inUV).a;
    if (alpha < 0.1) discard;
}
