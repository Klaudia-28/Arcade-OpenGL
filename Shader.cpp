
#include "Shader.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

// Lee el contenido de un archivo GLSL
std::string leerArchivo(const char* ruta)
{
    std::ifstream archivo(ruta);

    if (!archivo.is_open())
    {
        std::cout << "No se pudo abrir el archivo: " << ruta << std::endl;
        return "";
    }

    std::stringstream contenido;
    contenido << archivo.rdbuf();

    return contenido.str();
}

// Comprueba si el shader se compiló correctamente
bool comprobarCompilacion(GLuint shader, const char* nombre)
{
    GLint correcto;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &correcto);

    if (!correcto)
    {
        char log[1024];
        glGetShaderInfoLog(shader, 1024, NULL, log);

        std::cout << "Error compilando " << nombre << ":\n"
            << log << std::endl;

        return false;
    }

    return true;
}

// Constructor de la clase Shader
Shader::Shader(const char* vertexPath, const char* fragmentPath)
{
    // Leer los dos archivos
    std::string codigoVertex = leerArchivo(vertexPath);
    std::string codigoFragment = leerArchivo(fragmentPath);

    if (codigoVertex.empty() || codigoFragment.empty())
    {
        ID = 0;
        return;
    }

    const char* vertexSource = codigoVertex.c_str();
    const char* fragmentSource = codigoFragment.c_str();

    // Crear y compilar el vertex shader
    GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vertexSource, NULL);
    glCompileShader(vertex);

    bool vertexCorrecto = comprobarCompilacion(vertex, "Vertex Shader");

    // Crear y compilar el fragment shader
    GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fragmentSource, NULL);
    glCompileShader(fragment);

    bool fragmentCorrecto = comprobarCompilacion(fragment, "Fragment Shader");

    if (!vertexCorrecto || !fragmentCorrecto)
    {
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        ID = 0;
        return;
    }

    // Unir los dos shaders en un programa
    ID = glCreateProgram();

    glAttachShader(ID, vertex);
    glAttachShader(ID, fragment);
    glLinkProgram(ID);

    // Comprobar si el programa se enlazó correctamente
    GLint correcto;
    glGetProgramiv(ID, GL_LINK_STATUS, &correcto);

    if (!correcto)
    {
        char log[1024];
        glGetProgramInfoLog(ID, 1024, NULL, log);

        std::cout << "Error enlazando el programa de shaders:\n"
            << log << std::endl;

        glDeleteProgram(ID);
        ID = 0;
    }

    // Ya no necesitamos los shaders individuales
    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

// Activa el programa de shaders
void Shader::usar()
{
    glUseProgram(ID);
}
