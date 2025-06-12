#version 450

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;
layout(location = 3) in uint textureidx;

layout(binding = 0) uniform UBO {
    mat4 model;
    mat4 view;
    mat4 projection;
};

layout(location = 0) out vec4 fragpos;
layout(location = 1) out vec3 fragnormal;
layout(location = 2) out vec2 fraguv;
layout(location = 3) out flat uint fragtextureidx;

void main() {
    fragpos = model * vec4(pos, 1.0);
    gl_Position = projection * view * fragpos;

    //mat3 normalmatrix = transpose(inverse(mat3(view * model)));
    fragnormal = /*normalmatrix **/ normal;

    fraguv = uv;
    fragtextureidx = textureidx;
}