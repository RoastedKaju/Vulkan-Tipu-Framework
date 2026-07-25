#version 460
#extension GL_EXT_scalar_block_layout: require

layout (push_constant) uniform PushConstants {
    mat4 mvp;
    uint albedo;
};

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec4 inTangent;

layout (location = 0) out vec2 outUV;
layout (location = 1) out flat uint outAlbedo;

void main() {
    gl_Position = mvp * vec4(inPosition, 1.0);
    outUV = inUV;
    outAlbedo = albedo;
}
