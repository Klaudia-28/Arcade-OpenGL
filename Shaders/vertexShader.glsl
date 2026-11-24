
#version 330 core

layout (location=0) in vec3 apos;
layout (location=1) in vec3 aNormal;
layout (location=2) in vec2 aTexCoord;
layout (location=3) in vec4 aColor;

out vec2 TexCoord;
out vec3 FragPos;
out vec3 Normal;
out vec4 VertexColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec2 uvScale;

void main()
{
    vec4 posicionMundo = model * vec4(apos, 1.0);
    FragPos = posicionMundo.xyz;
    Normal = mat3(transpose(inverse(model))) * aNormal;
    gl_Position = projection * view * posicionMundo;
    TexCoord = aTexCoord * uvScale;

    VertexColor = aColor;
}

