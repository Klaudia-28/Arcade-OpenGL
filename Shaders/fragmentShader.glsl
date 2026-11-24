
#version 330 core

in vec2 TexCoord;
in vec3 FragPos;
in vec3 Normal;
in vec4 VertexColor;

out vec4 FragColor;

uniform sampler2D texColor;
uniform sampler2D texEmisiva;
uniform bool tieneEmisiva;
uniform vec4 colorBase;
uniform vec3 viewPos;
uniform vec3 lightPos[4];
uniform vec3 lightColor[4];

void main()
{
    vec4 base = texture(texColor, TexCoord) * colorBase * VertexColor;
    vec3 normal = normalize(Normal);
    vec3 iluminacion = base.rgb * 0.28;

for (int i = 0; i < 4; i++)
{
    vec3 haciaLuz = lightPos[i] - FragPos;
    float distancia = length(haciaLuz);
    vec3 direccionLuz = normalize(haciaLuz);

    // Iluminación difusa
    float difusa = max(dot(normal, direccionLuz), 0.0);

    // Iluminación especular (Phong)
    vec3 vista = normalize(viewPos - FragPos);
    vec3 reflexion = reflect(-direccionLuz, normal);

    float especular = pow(
        max(dot(vista, reflexion), 0.0),
        32.0
    );

    // Atenuación según la distancia
    float atenuacion = 1.0 / (
        1.0 + 0.12 * distancia
        + 0.035 * distancia * distancia
    );

    iluminacion += base.rgb * lightColor[i] * difusa * atenuacion * 1.8;
    iluminacion += lightColor[i] * especular * atenuacion * 0.22;
}

    vec3 color = iluminacion;

    if (tieneEmisiva)
        color += texture(texEmisiva, TexCoord).rgb;

    FragColor = vec4(color, base.a);
}

