// ==========================================================================
//                         PROYECTO FINAL - ARCADE
//
//  Crédito del modelo 3D "arcade machine": Logotyp (Sketchfab),
//  licencia CC-BY-4.0:
//  https://sketchfab.com/3d-models/arcade-machine-a914bf3651e3496ea708dc0782eca212
// ==========================================================================

#include <iostream>              // Salida de mensajes en consola
#include <vector>                // Contenedores para almacenar piezas, modelos y texturas
#include <string>
#include <glad/glad.h>           // Funciones de OpenGL
#include <GLFW/glfw3.h>          // Ventana y entrada de teclado y mouse

#define GLM_ENABLE_EXPERIMENTAL  // Habilita funciones experimentales de GLM
#include <glm/glm.hpp>                       // Vectores y matrices
#include <glm/gtc/matrix_transform.hpp>      // Operaciones de transformación y cámara
#include <glm/gtc/type_ptr.hpp>              // Conversión de matrices y vectores para enviarlos a OpenGL
#include <glm/gtc/quaternion.hpp>            // Cuaterniones para interpretar rotaciones de los nodos GLTF

// ---- Librería para cargar el modelo (tinygltf, versión 2.9.x) ----
// Estas líneas se escriben UNA sola vez en todo el proyecto, justo antes del include:
#define TINYGLTF_IMPLEMENTATION          // Implementación de TinyGLTF en este archivo
#define STB_IMAGE_IMPLEMENTATION         // Implementación de STB Image en este archivo
#define TINYGLTF_NO_STB_IMAGE_WRITE      // Deshabilita funciones de escritura de imágenes que no utilizo
#include "tiny_gltf.h"
#include "Clases/Shader.h"


// ==========================================================================
//                 MATRICES DE TRASLACIÓN DE LA CÁMARA
// ==========================================================================
// La posición y los vectores de orientación definen desde dónde observa
// la cámara y hacia qué dirección está mirando dentro de la habitación.
float anchoCuarto = 10.0f;   // tamaño de la habitación: TODO se calcula a partir de estos 3 números
float largoCuarto = 10.0f;
float altoCuarto = 3.6f;
float alturaOjos = 1.6f;     // la cámara siempre se queda a esta altura (como si caminara)

glm::vec3 cameraPos = glm::vec3(0.0f, 1.6f, 4.0f);     // dónde está la cámara
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);  // hacia dónde mira
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);      // vector "arriba"

// yaw controla el giro horizontal y pitch la inclinación vertical de la cámara.
float yaw = -90.0f;
float pitch = 0.0f;
float ultimoX = 500.0f;
float ultimoY = 350.0f;
bool primeraVezMouse = true;

// ==========================================================================
//  FUNCIONES DE APOYO
// ==========================================================================

// GLFW la llama cuando se cambia el tamaño de la ventana
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

// El movimiento del mouse modifica yaw y pitch; con esos ángulos se actualiza
// cameraFront para orientar la cámara sin cambiar su posición.
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (primeraVezMouse)
    {
        ultimoX = (float)xpos;
        ultimoY = (float)ypos;
        primeraVezMouse = false;
    }

    float xoffset = (float)xpos - ultimoX;
    float yoffset = ultimoY - (float)ypos;   // invertido: en pantalla Y crece hacia abajo
    ultimoX = (float)xpos;
    ultimoY = (float)ypos;

    float sensibilidad = 0.1f;
    yaw += xoffset * sensibilidad;
    pitch += yoffset * sensibilidad;

    if (pitch > 89.0f) pitch = 89.0f;        // para que la cámara no se voltee de cabeza
    if (pitch < -89.0f) pitch = -89.0f;

    // Se convierten los ángulos a una dirección en 3D y se normalizan para obtener
    // un vector unitario que indique hacia dónde está mirando la cámara
    glm::vec3 direccion;
    direccion.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direccion.y = sin(glm::radians(pitch));
    direccion.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(direccion);
}

// Revisa si un shader compiló bien, si no, imprime el error en la consola
void revisarShader(GLuint shader, const char* nombre)
{
    GLint ok;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok)
    {
        char log[1024];
        glGetShaderInfoLog(shader, 1024, NULL, log);
        std::cout << "Error compilando " << nombre << ":\n" << log << std::endl;
    }
}

// Sube una imagen (ya cargada en memoria) a la GPU y devuelve el número (ID) de la textura
GLuint subirTextura(const unsigned char* pixeles, int ancho, int alto, int canales)
{
    GLenum formato = GL_RGB;
    if (canales == 1) formato = GL_RED;
    else if (canales == 2) formato = GL_RG;
    else if (canales == 3) formato = GL_RGB;
    else if (canales == 4) formato = GL_RGBA;

    GLuint id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);   // evita errores con imágenes cuyo ancho no es múltiplo de 4
    glTexImage2D(GL_TEXTURE_2D, 0, formato, ancho, alto, 0, formato, GL_UNSIGNED_BYTE, pixeles);
    glGenerateMipmap(GL_TEXTURE_2D);         // versiones más pequeñas, para cuando se ve de lejos

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);   // si u,v pasan de 1, la imagen se repite
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    return id;
}

// Carga un archivo de imagen del disco (jpg/png) y lo sube a la GPU
GLuint cargarTextura(const char* ruta)
{
    int ancho, alto, canales;
    unsigned char* datos = stbi_load(ruta, &ancho, &alto, &canales, 0);
    if (!datos)
    {
        std::cout << "No se pudo cargar la imagen: " << ruta << std::endl;
        return 0;
    }
    GLuint id = subirTextura(datos, ancho, alto, canales);
    stbi_image_free(datos);   // ya está en la GPU, se libera de la RAM
    return id;
}

// ==========================================================================
//                         CARGA DE MODELOS GLTF
// ==========================================================================
// Un archivo GLTF organiza el modelo mediante nodos, mallas, primitivas y
// materiales. 
// Recorre esa estructura para separar cada pieza y preparar sus
// datos de geometría, sus texturas y sus transformaciones.
// Cada primitiva del modelo se guarda como una pieza independiente, con sus
// propios buffers, material y matriz dentro de la jerarquía del modelo.
struct Pieza
{
    GLuint VAO = 0, VBO = 0, EBO = 0;
    GLsizei numIndices = 0;
    glm::mat4 matriz = glm::mat4(1.0f);       // posición de esta pieza dentro del modelo
    GLuint texColor = 0;                      // textura de color (0 = no tiene)
    GLuint texEmisiva = 0;                    // textura que "brilla" (pantalla, neón) (0 = no tiene)
    glm::vec4 colorBase = glm::vec4(1.0f);    // color base del material (se multiplica con la textura)
    bool transparente = false;                // true para vidrios
};

// Un modelo reúne todas sus piezas y una matriz para centrarlo y ajustar su tamaño.
struct ModeloCargado
{
    std::vector<Pieza> piezas;
    glm::mat4 normalizacion = glm::mat4(1.0f);
};

std::vector<GLuint> cacheTexturas;   // evita subir dos veces la misma imagen del modelo
std::vector<GLuint> texturasCargadas; // guarda las texturas de todos los modelos para liberarlas al final

// Reutilizo las texturas que ya fueron cargadas para evitar subir varias veces
// la misma imagen del archivo GLTF a la memoria de la GPU.
GLuint texturaDelModelo(const tinygltf::Model& model, int idxTextura)
{
    if (idxTextura < 0) return 0;
    int idxImagen = model.textures[idxTextura].source;
    if (idxImagen < 0) return 0;
    if (cacheTexturas.size() < model.images.size())
        cacheTexturas.resize(model.images.size(), 0);
    if (cacheTexturas[idxImagen] == 0)
    {
        const tinygltf::Image& img = model.images[idxImagen];
        cacheTexturas[idxImagen] = subirTextura(img.image.data(), img.width, img.height, img.component);
        texturasCargadas.push_back(cacheTexturas[idxImagen]);
    }
    return cacheTexturas[idxImagen];
}

// Cada nodo puede traer una matriz completa o sus transformaciones por separado
// En este último caso se construye la matriz combinando traslación, rotación y escala
glm::mat4 matrizDelNodo(const tinygltf::Node& nodo)
{
    if (nodo.matrix.size() == 16)
    {
        glm::mat4 m(1.0f);
        for (int i = 0; i < 16; i++) m[i / 4][i % 4] = (float)nodo.matrix[i];
        return m;
    }
    glm::mat4 m(1.0f);
    if (nodo.translation.size() == 3)
        m = glm::translate(m, glm::vec3((float)nodo.translation[0], (float)nodo.translation[1], (float)nodo.translation[2]));
    if (nodo.rotation.size() == 4)
        m = m * glm::mat4_cast(glm::quat((float)nodo.rotation[3], (float)nodo.rotation[0], (float)nodo.rotation[1], (float)nodo.rotation[2]));
    if (nodo.scale.size() == 3)
        m = glm::scale(m, glm::vec3((float)nodo.scale[0], (float)nodo.scale[1], (float)nodo.scale[2]));
    return m;
}

// Los accessors indican dónde están guardados los atributos de la malla
// Esta función obtiene el inicio de esos datos y la separación entre elementos
const unsigned char* datosAccessor(const tinygltf::Model& model, int idxAccessor, int& stride)
{
    const tinygltf::Accessor& acc = model.accessors[idxAccessor];
    const tinygltf::BufferView& bv = model.bufferViews[acc.bufferView];
    const tinygltf::Buffer& buf = model.buffers[bv.buffer];
    stride = acc.ByteStride(bv);   // cuántos bytes hay de un vértice al siguiente
    return &buf.data[bv.byteOffset + acc.byteOffset];
}

// Recorre la estructura del modelo y recupera la informacion de cada uno
void procesarNodo(const tinygltf::Model& model, int idxNodo, const glm::mat4& matrizPadre,
    std::vector<Pieza>& piezas, glm::vec3& bbMin, glm::vec3& bbMax)
{
    const tinygltf::Node& nodo = model.nodes[idxNodo];
    glm::mat4 matrizMundo = matrizPadre * matrizDelNodo(nodo);   // la del padre * la propia

    if (nodo.mesh >= 0)
    {
        const tinygltf::Mesh& malla = model.meshes[nodo.mesh];
        for (const tinygltf::Primitive& prim : malla.primitives)
        {
            if (prim.mode != TINYGLTF_MODE_TRIANGLES) continue;
            if (prim.attributes.find("POSITION") == prim.attributes.end()) continue;

            // Primero se recuperan los atributos de cada vértice: posición, normal,
            // coordenadas UV y, cuando existen, los colores definidos en COLOR_0.
            int stridePos, strideNor = 0, strideUv = 0;
            int accPos = prim.attributes.at("POSITION");
            const unsigned char* pPos = datosAccessor(model, accPos, stridePos);
            size_t numVertices = model.accessors[accPos].count;

            const unsigned char* pNor = nullptr;
            if (prim.attributes.count("NORMAL"))
                pNor = datosAccessor(model, prim.attributes.at("NORMAL"), strideNor);
            const unsigned char* pUv = nullptr;
            if (prim.attributes.count("TEXCOORD_0"))
                pUv = datosAccessor(model, prim.attributes.at("TEXCOORD_0"), strideUv);
            // Leer los colores de los vértices (COLOR_0)
            int strideColor = 0;
            const unsigned char* pColor = nullptr;

            if (prim.attributes.count("COLOR_0"))
            {
                pColor = datosAccessor(
                    model,
                    prim.attributes.at("COLOR_0"),
                    strideColor
                );
            }

            // Cada vértice tiene 12 floats:
            // posición(3) + normal(3) + uv(2) + color(4)
            std::vector<float> vertices;
            vertices.reserve(numVertices * 12);
            for (size_t i = 0; i < numVertices; i++)
            {
                const float* p = (const float*)(pPos + i * stridePos);
                vertices.push_back(p[0]); vertices.push_back(p[1]); vertices.push_back(p[2]);

                if (pNor) {
                    const float* n = (const float*)(pNor + i * strideNor);
                    vertices.push_back(n[0]); vertices.push_back(n[1]); vertices.push_back(n[2]);
                }
                else { vertices.push_back(0.0f); vertices.push_back(1.0f); vertices.push_back(0.0f); }

                if (pUv) {
                    const float* t = (const float*)(pUv + i * strideUv);
                    vertices.push_back(t[0]); vertices.push_back(t[1]);
                }
                else { vertices.push_back(0.0f); vertices.push_back(0.0f); }
                // Conservar el color por vértice del modelo; si no existe, usa blanco
            // para que ese atributo no altere el color del material ni su textura
                if (pColor)
                {
                    const float* c = (const float*)(pColor + i * strideColor);

                    vertices.push_back(c[0]);
                    vertices.push_back(c[1]);
                    vertices.push_back(c[2]);
                    vertices.push_back(c[3]);
                }
                else
                {
                    // Blanco para los modelos que no tienen colores por vértice
                    vertices.push_back(1.0f);
                    vertices.push_back(1.0f);
                    vertices.push_back(1.0f);
                    vertices.push_back(1.0f);
                }

                // Para calcular el tamaño total del modelo (caja que lo envuelve)
                glm::vec3 enMundo = glm::vec3(matrizMundo * glm::vec4(p[0], p[1], p[2], 1.0f));
                bbMin = glm::min(bbMin, enMundo);
                bbMax = glm::max(bbMax, enMundo);
            }

            // Los índices determinan qué vértices forman cada triángulo. Si la
            // primitiva no tiene índices, utiliza los vértices en el orden recibido.
            std::vector<unsigned int> indices;
            if (prim.indices >= 0)
            {
                const tinygltf::Accessor& accIdx = model.accessors[prim.indices];
                int strideIdx;
                const unsigned char* pIdx = datosAccessor(model, prim.indices, strideIdx);
                for (size_t i = 0; i < accIdx.count; i++)
                {
                    if (accIdx.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
                        indices.push_back(*(const unsigned int*)(pIdx + i * strideIdx));
                    else if (accIdx.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
                        indices.push_back(*(const unsigned short*)(pIdx + i * strideIdx));
                    else
                        indices.push_back(*(pIdx + i * strideIdx));
                }
            }
            else
            {
                for (unsigned int i = 0; i < numVertices; i++) indices.push_back(i);
            }

            // Organiza los datos en VAO, VBO y EBO. Los atributos quedan asociados
            // a las posiciones, normales, coordenadas UV y colores de cada vértice.
            Pieza pieza;
            pieza.matriz = matrizMundo;
            pieza.numIndices = (GLsizei)indices.size();

            glGenVertexArrays(1, &pieza.VAO);
            glGenBuffers(1, &pieza.VBO);
            glGenBuffers(1, &pieza.EBO);

            glBindVertexArray(pieza.VAO);
            glBindBuffer(GL_ARRAY_BUFFER, pieza.VBO);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pieza.EBO);   // el EBO se queda guardado dentro del VAO
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

            // Posición
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 12 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);

            // Normal
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 12 * sizeof(float), (void*)(3 * sizeof(float)));
            glEnableVertexAttribArray(1);

            // Coordenadas de textura
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 12 * sizeof(float), (void*)(6 * sizeof(float)));
            glEnableVertexAttribArray(2);

            // Color
            glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 12 * sizeof(float), (void*)(8 * sizeof(float)));
            glEnableVertexAttribArray(3);
            glBindVertexArray(0);

            // Recupera el material de la primitiva como color base, textura principal,
            // textura emisiva y modo de transparencia.
            if (prim.material >= 0)
            {
                const tinygltf::Material& mat = model.materials[prim.material];
                const std::vector<double>& c = mat.pbrMetallicRoughness.baseColorFactor;
                if (c.size() == 4) pieza.colorBase = glm::vec4((float)c[0], (float)c[1], (float)c[2], (float)c[3]);
                pieza.texColor = texturaDelModelo(model, mat.pbrMetallicRoughness.baseColorTexture.index);
                pieza.texEmisiva = texturaDelModelo(model, mat.emissiveTexture.index);
                pieza.transparente = (mat.alphaMode == "BLEND");
            }

            piezas.push_back(pieza);
        }
    }

    // Continúa el recorrido con los nodos hijos, conservando la transformación
    // acumulada de sus nodos padres.
    for (int hijo : nodo.children)
        procesarNodo(model, hijo, matrizMundo, piezas, bbMin, bbMax);
}
// ==========================================================================
//                           CURVAS DE BÉZIER
// ==========================================================================
// Evalúa una curva cúbica a partir de cuatro puntos de control. El parámetro t
// recorre la curva entre 0 y 1 y permite obtener la posición del objeto en ella.
glm::vec3 bezierCubica(
    glm::vec3 P0,
    glm::vec3 P1,
    glm::vec3 P2,
    glm::vec3 P3,
    float t)
{
    float u = 1.0f - t;

    return u * u * u * P0
        + 3.0f * u * u * t * P1
        + 3.0f * u * t * t * P2
        + t * t * t * P3;
}

// ==========================================================================
//                         INICIO DE LA APLICACIÓN
// ==========================================================================
int main()
{
    // ================= 1) CONSTRUCT: contexto (GLFW + GLAD) =================
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1000, 700, "Arcade - adelanto", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);   // oculta y "atrapa" el mouse
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    glViewport(0, 0, fbWidth, fbHeight);

    glEnable(GL_DEPTH_TEST);                                  // buffer de profundidad
    glEnable(GL_BLEND);                                       // permite transparencias (vidrio)
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // ================= 2) GEOMETRY: declaración de vértices =================
    // Un solo cuadrado (1 x 1) sirve para el piso, el techo y las 4 paredes:
    // cada cara es este mismo cuadrado movido, girado y estirado con una matriz "model".
    // Cada vértice: posición (x,y,z) + normal (x,y,z) + coordenada de textura (u,v)
    float verticesCuadrado[] = {
        -0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,   // abajo izquierda
         0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   1.0f, 0.0f,   // abajo derecha
         0.5f,  0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   1.0f, 1.0f,   // arriba derecha
        -0.5f,  0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 1.0f    // arriba izquierda
    };
    // Con índices (EBO) el cuadrado necesita solo 4 vértices en vez de 6
    unsigned int indicesCuadrado[] = { 0, 1, 2,   2, 3, 0 };

    // ================= 3) SHADER COMPILATION =================
    Shader shader("Shaders/vertexShader.glsl", "Shaders/fragmentShader.glsl");

    if (shader.ID == 0)
    {
        std::cout << "Error al crear el programa de shaders." << std::endl;
        glfwTerminate();
        return -1;
    }

    GLuint ShaderProgram = shader.ID;

    // Obtiene las ubicaciones de las variables uniformes que conectan el código
    // de C++ con los shaders: matrices, materiales, cámara y fuentes de luz.
    GLint modelLoc = glGetUniformLocation(ShaderProgram, "model");
    GLint viewLoc = glGetUniformLocation(ShaderProgram, "view");
    GLint projLoc = glGetUniformLocation(ShaderProgram, "projection");
    GLint uvScaleLoc = glGetUniformLocation(ShaderProgram, "uvScale");
    GLint colorBaseLoc = glGetUniformLocation(ShaderProgram, "colorBase");
    GLint tieneEmisivaLoc = glGetUniformLocation(ShaderProgram, "tieneEmisiva");
    GLint viewPosLoc = glGetUniformLocation(ShaderProgram, "viewPos");
    GLint lightPosLoc[4] = {
        glGetUniformLocation(ShaderProgram, "lightPos[0]"),
        glGetUniformLocation(ShaderProgram, "lightPos[1]"),
        glGetUniformLocation(ShaderProgram, "lightPos[2]"),
        glGetUniformLocation(ShaderProgram, "lightPos[3]")
    };

    GLint lightColorLoc[4] = {
     glGetUniformLocation(ShaderProgram, "lightColor[0]"),
     glGetUniformLocation(ShaderProgram, "lightColor[1]"),
     glGetUniformLocation(ShaderProgram, "lightColor[2]"),
     glGetUniformLocation(ShaderProgram, "lightColor[3]")
    };

    shader.usar();
    glUniform1i(glGetUniformLocation(ShaderProgram, "texColor"), 0);     // texColor usa la unidad 0
    glUniform1i(glGetUniformLocation(ShaderProgram, "texEmisiva"), 1);   // texEmisiva usa la unidad 1

    // ================= 4) BUFFERS: VAO, VBO, EBO =================
    // ---- del cuadrado (habitación) ----
    GLuint VAOcuadrado, VBOcuadrado, EBOcuadrado;
    glGenVertexArrays(1, &VAOcuadrado);
    glGenBuffers(1, &VBOcuadrado);
    glGenBuffers(1, &EBOcuadrado);

    glBindVertexArray(VAOcuadrado);
    glBindBuffer(GL_ARRAY_BUFFER, VBOcuadrado);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verticesCuadrado), verticesCuadrado, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBOcuadrado);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indicesCuadrado), indicesCuadrado, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    // CARGO Y NORMALIZO LOS MODELOS GLTF
    // Cada archivo se recorre para recuperar sus piezas. Después ajusta su escala
    // a la altura indicada y lo centro horizontalmente para ubicarlo en la escena.
    auto cargarModelo = [&](const char* ruta, float alturaModelo)
        {
            ModeloCargado resultado;
            cacheTexturas.clear(); // cada archivo tiene sus propias imágenes internas

            tinygltf::Model modeloGltf;
            tinygltf::TinyGLTF cargador;
            std::string errores, avisos;
            bool cargado = cargador.LoadBinaryFromFile(&modeloGltf, &errores, &avisos, ruta);

            if (!avisos.empty()) std::cout << "Aviso en " << ruta << ": " << avisos << std::endl;
            if (!cargado)
            {
                std::cout << "No se pudo cargar el modelo " << ruta << ": " << errores << std::endl;
                return resultado;
            }

            glm::vec3 bbMin(1e9f), bbMax(-1e9f);
            int escena = modeloGltf.defaultScene >= 0 ? modeloGltf.defaultScene : 0;

            for (int nodoRaiz : modeloGltf.scenes[escena].nodes)
                procesarNodo(modeloGltf, nodoRaiz, glm::mat4(1.0f), resultado.piezas, bbMin, bbMax);

            if (!resultado.piezas.empty())
            {
                float escala = alturaModelo / (bbMax.y - bbMin.y);
                glm::vec3 centro = (bbMin + bbMax) * 0.5f;
                resultado.normalizacion = glm::scale(glm::mat4(1.0f), glm::vec3(escala));
                resultado.normalizacion = glm::translate(resultado.normalizacion,
                    glm::vec3(-centro.x, -bbMin.y, -centro.z));
            }

            std::cout << "Modelo cargado: " << ruta << " (" << resultado.piezas.size() << " piezas)." << std::endl;
            return resultado;
        };

    std::vector<ModeloCargado> modelos;
    modelos.push_back(cargarModelo("arcade_machine.glb", 2.45f));
    modelos.push_back(cargarModelo("abandondedvandalised_arcade_machine.glb", 2.45f));
    modelos.push_back(cargarModelo("dance_dance_revolution.glb", 2.15f));
    modelos.push_back(cargarModelo("pacman_arcade__animation.glb", 2.45f));
    modelos.push_back(cargarModelo("girl_speedsculpt.glb", 2.00f));
    modelos.push_back(cargarModelo("pacman_ghost.glb", 0.60f));

    // ---- Texturas de la habitación ----
    // stb_image voltea la imagen al cargarla para que coincida con las coordenadas (u,v) de OpenGL.
    // Se activa DESPUÉS de cargar el .glb porque las imágenes de glTF ya vienen acomodadas.
    stbi_set_flip_vertically_on_load(true);
    GLuint texParedA = cargarTextura("pared1.jpeg");
    GLuint texParedB = cargarTextura("pared2.jpeg");
    GLuint texPiso = cargarTextura("piso.jpeg");

    // Textura blanca de 1 píxel: se usa cuando una pieza no tiene textura propia
    unsigned char pixelBlanco[3] = { 255, 255, 255 };
    GLuint texBlanca = subirTextura(pixelBlanco, 1, 1, 3);

    // ======================================================================
    //                    ROTACIÓN Y UBICACIÓN DE OBJETOS
    // ======================================================================
    // Defino la posición y el ángulo inicial de cada máquina arcade. Estas
    // transformaciones se aplican mediante matrices antes de dibujar cada modelo.
    struct ColocacionMaquina { int modelo; glm::vec3 posicion; float anguloY; };
    std::vector<ColocacionMaquina> maquinas = {
        { 0, glm::vec3(-2.7f, 0.0f, -largoCuarto / 2.0f + 1.0f), 35.0f },
        { 1, glm::vec3(-anchoCuarto / 2.0f + 1.0f, 0.0f, 0.0f), 90.0f },
        { 2, glm::vec3(2.7f, 0.0f, -largoCuarto / 2.0f + 1.5f), 0.0f }
    };
    // Máquina de Pac-Man junto a la pared derecha
    maquinas.push_back({
        3,
        glm::vec3(anchoCuarto / 2.0f - 1.0f, 0.0f, 0.0f),
        -90.0f
        });

    // Dibuja todas las piezas de un modelo aplicando la transformación externa
    // junto con la normalización y la matriz propia de cada pieza.
    auto dibujarModelo = [&](const ModeloCargado& modeloActual,
        const glm::mat4& transformacion)
        {
            glUniform2f(uvScaleLoc, 1.0f, 1.0f);

            // Dibuja primero las piezas opacas y después las transparentes para
            // que los elementos como los vidrios se representen al final.
            for (int vuelta = 0; vuelta < 2; vuelta++)
            {
                bool dibujarTransparentes = (vuelta == 1);

                for (const Pieza& pieza : modeloActual.piezas)
                {
                    if (pieza.transparente != dibujarTransparentes)
                        continue;

                    glm::mat4 modelPieza =
                        transformacion * modeloActual.normalizacion * pieza.matriz;

                    glUniformMatrix4fv(
                        modelLoc, 1, GL_FALSE, glm::value_ptr(modelPieza));

                    glUniform4fv(
                        colorBaseLoc, 1, glm::value_ptr(pieza.colorBase));

                    glUniform1i(
                        tieneEmisivaLoc, pieza.texEmisiva != 0 ? 1 : 0);

                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(
                        GL_TEXTURE_2D,
                        pieza.texColor ? pieza.texColor : texBlanca);

                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(
                        GL_TEXTURE_2D,
                        pieza.texEmisiva ? pieza.texEmisiva : texBlanca);

                    glActiveTexture(GL_TEXTURE0);

                    glBindVertexArray(pieza.VAO);
                    glDrawElements(
                        GL_TRIANGLES,
                        pieza.numIndices,
                        GL_UNSIGNED_INT,
                        0);
                }
            }
        };

    float tiempoAnterior = (float)glfwGetTime();

    // ================= 5) CONTINUUM: render loop =================
    while (!glfwWindowShouldClose(window))
    {
        float tiempoActual = (float)glfwGetTime();
        float deltaTime = tiempoActual - tiempoAnterior;     // tiempo entre frames
        tiempoAnterior = tiempoActual;

        // ==================================================================
        // MATRICES DE TRASLACIÓN DE LA CÁMARA: MOVIMIENTO CON EL TECLADO
        // ==================================================================
        // W y S desplazan la cámara hacia adelante y atrás; A y D la desplazan
        // lateralmente. El movimiento se calcula con deltaTime para que la
        // velocidad no dependa de la cantidad de frames por segundo.
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);          // ESC cierra (el mouse está atrapado)

        float velocidad = 2.5f * deltaTime;
        // W/S avanzan solo en el plano horizontal
        glm::vec3 adelante = glm::normalize(glm::vec3(cameraFront.x, 0.0f, cameraFront.z));
        glm::vec3 derecha = glm::normalize(glm::cross(cameraFront, cameraUp));
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) cameraPos += adelante * velocidad;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) cameraPos -= adelante * velocidad;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) cameraPos -= derecha * velocidad;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) cameraPos += derecha * velocidad;

        // Limita la posición para que la cámara permanezca dentro de la habitación
        // y mantiene fija la altura de los ojos durante el recorrido.
        float margen = 0.5f;
        float limiteX = anchoCuarto / 2.0f - margen;
        float limiteZ = largoCuarto / 2.0f - margen;
        if (cameraPos.x > limiteX) cameraPos.x = limiteX;
        if (cameraPos.x < -limiteX) cameraPos.x = -limiteX;
        if (cameraPos.z > limiteZ) cameraPos.z = limiteZ;
        if (cameraPos.z < -limiteZ) cameraPos.z = -limiteZ;
        cameraPos.y = alturaOjos;

        glClearColor(0.05f, 0.07f, 0.14f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(ShaderProgram);

        // La matriz de vista se construye con la posición, la dirección de mirada
        // y el vector superior de la cámara. La matriz de perspectiva define
        // el campo de visión y los planos cercano y lejano de la escena
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        float aspect = (fbHeight != 0) ? (float)fbWidth / (float)fbHeight : 1.0f;
        glm::mat4 projection = glm::perspective(glm::radians(60.0f), aspect, 0.1f, 100.0f);
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3fv(viewPosLoc, 1, glm::value_ptr(cameraPos));

        // ==================================================================
        //                         ILUMINACIÓN PHONG
        // ==================================================================
        // Define cuatro fuentes de luz: tres luces de colores para el efecto
        // neón y una luz blanca ubicada en el centro del techo. Sus posiciones
        // y colores se envían al fragment shader para calcular la iluminación.

        glm::vec3 posicionesLuces[4] = {
            glm::vec3(-3.5f, 3.0f, -2.5f),
            glm::vec3(3.5f, 3.0f, -2.5f),
            glm::vec3(0.0f, 3.0f,  2.5f),
            glm::vec3(0.0f, 3.4f,  0.0f)
        };

        glm::vec3 coloresLuces[4] = {
            glm::vec3(0.15f, 0.25f, 1.0f),
            glm::vec3(1.0f, 0.08f, 0.65f),
            glm::vec3(0.05f, 0.85f, 1.0f),
            glm::vec3(0.20f, 0.20f, 0.20f)
        };

        // Envía las posiciones y los colores de las cuatro luces al shader.
        for (int i = 0; i < 4; i++)
        {
            glUniform3fv(lightPosLoc[i], 1, glm::value_ptr(posicionesLuces[i]));
            glUniform3fv(lightColorLoc[i], 1, glm::value_ptr(coloresLuces[i]));
        }

        // ============ CONSTRUCCIÓN DE LA HABITACIÓN ============
        // Reutiliza un mismo cuadrado para formar piso, techo y paredes. En cada
        // cara cambia su matriz de modelo y asigna la textura correspondiente.
        glUniform4f(colorBaseLoc, 1.0f, 1.0f, 1.0f, 1.0f);
        glUniform1i(tieneEmisivaLoc, 0);
        glActiveTexture(GL_TEXTURE0);
        glBindVertexArray(VAOcuadrado);
        glVertexAttrib4f(3, 1.0f, 1.0f, 1.0f, 1.0f);

        float tamanoBaldosa = 2.0f;   // una repetición de la imagen del piso mide 2 x 2 unidades

        // ---- Piso ----
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::scale(model, glm::vec3(anchoCuarto, largoCuarto, 1.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniform2f(uvScaleLoc, 1.0f, 1.0f); // una imagen completa cubre todo el piso
        glBindTexture(GL_TEXTURE_2D, texPiso ? texPiso : texBlanca);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // ---- Techo ----
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, altoCuarto, 0.0f));
        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::scale(model, glm::vec3(anchoCuarto, largoCuarto, 1.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniform2f(uvScaleLoc, 1.0f, 1.0f);
        glBindTexture(GL_TEXTURE_2D, texParedA ? texParedA : texBlanca);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // ---- Pared del fondo (z = -largo/2) ----
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, altoCuarto / 2.0f, -largoCuarto / 2.0f));
        model = glm::scale(model, glm::vec3(anchoCuarto, altoCuarto, 1.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glBindTexture(GL_TEXTURE_2D, texParedA ? texParedA : texBlanca);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // ---- Pared del frente (z = +largo/2), girada 180° ----
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, altoCuarto / 2.0f, largoCuarto / 2.0f));
        model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(anchoCuarto, altoCuarto, 1.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glBindTexture(GL_TEXTURE_2D, texParedA ? texParedA : texBlanca);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // ---- Pared izquierda (x = -ancho/2), girada 90° ----
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-anchoCuarto / 2.0f, altoCuarto / 2.0f, 0.0f));
        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(largoCuarto, altoCuarto, 1.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glBindTexture(GL_TEXTURE_2D, texParedB ? texParedB : texBlanca);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // ---- Pared derecha (x = +ancho/2), girada -90° ----
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(anchoCuarto / 2.0f, altoCuarto / 2.0f, 0.0f));
        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(largoCuarto, altoCuarto, 1.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glBindTexture(GL_TEXTURE_2D, texParedB ? texParedB : texBlanca);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // ============ DIBUJO DE LAS MÁQUINAS ARCADE ============
        // Para cada máquina combina su traslación y rotación con la normalización
        // del modelo y la matriz de cada pieza antes de enviarla a OpenGL.
        // Primero se dibujan las piezas opacas y luego las transparentes.
        glUniform2f(uvScaleLoc, 1.0f, 1.0f);
        for (int vuelta = 0; vuelta < 2; vuelta++)
        {
            bool dibujarTransparentes = (vuelta == 1);
            for (const ColocacionMaquina& maq : maquinas)
            {
                glm::mat4 colocacion = glm::translate(glm::mat4(1.0f), maq.posicion);
                colocacion = glm::rotate(colocacion, glm::radians(maq.anguloY), glm::vec3(0.0f, 1.0f, 0.0f));

                const ModeloCargado& modeloActual = modelos[maq.modelo];
                for (const Pieza& pieza : modeloActual.piezas)
                {
                    if (pieza.transparente != dibujarTransparentes) continue;

                    glm::mat4 modelPieza = colocacion * modeloActual.normalizacion * pieza.matriz;
                    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelPieza));
                    glUniform4fv(colorBaseLoc, 1, glm::value_ptr(pieza.colorBase));
                    glUniform1i(tieneEmisivaLoc, pieza.texEmisiva != 0 ? 1 : 0);

                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, pieza.texColor ? pieza.texColor : texBlanca);
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, pieza.texEmisiva ? pieza.texEmisiva : texBlanca);
                    glActiveTexture(GL_TEXTURE0);

                    glBindVertexArray(pieza.VAO);
                    glDrawElements(GL_TRIANGLES, pieza.numIndices, GL_UNSIGNED_INT, 0);
                }
            }
        }

        // ==================================================================
        //                         ROTACIÓN DE OBJETOS
        // ==================================================================
        // Ubica la figura en el centro de la habitación y crea una rotación
        // continua alrededor del eje Y. El ángulo depende del tiempo transcurrido,
        // por eso el movimiento se mantiene constante durante la ejecución.
        glm::mat4 modeloChica = glm::translate(
            glm::mat4(1.0f),
            glm::vec3(0.0f, 0.0f, 0.0f)
        );

        float anguloChica = tiempoActual * 45.0f;

        modeloChica = glm::rotate(
            modeloChica,
            glm::radians(anguloChica),
            glm::vec3(0.0f, 1.0f, 0.0f)
        );

        dibujarModelo(modelos[4], modeloChica);


        // ==================================================================
        //                         CURVAS DE BÉZIER
        // ==================================================================
        // Los fantasmas recorren una trayectoria cerrada formada por cuatro
        // curvas cúbicas conectadas. A, B, C y D son los puntos de unión; los
        // otros dos puntos de cada tramo controlan la curvatura del recorrido.

        glm::vec3 A(3.0f, 0.3f, 3.0f);    // Delantera derecha
        glm::vec3 B(3.0f, 0.3f, -3.0f);   // Trasera derecha
        glm::vec3 C(-3.0f, 0.3f, -3.0f);  // Trasera izquierda
        glm::vec3 D(-3.0f, 0.3f, 3.0f);   // Delantera izquierda

        // El tiempo determina el avance del recorrido. fmod mantiene el valor
        // dentro de cuatro tramos; de ahí obtengo el tramo activo y su parámetro t.
        float recorrido = fmod(tiempoActual * 0.12f, 4.0f);

        int tramo = (int)recorrido;
        float t = recorrido - tramo;

        glm::vec3 P0, P1, P2, P3;

        if (tramo == 0)
        {
            // De la esquina delantera derecha a la trasera derecha
            P0 = A;
            P1 = glm::vec3(4.0f, 0.8f, 1.0f);
            P2 = glm::vec3(4.0f, 0.8f, -1.0f);
            P3 = B;
        }
        else if (tramo == 1)
        {
            // De la esquina trasera derecha a la trasera izquierda
            P0 = B;
            P1 = glm::vec3(1.0f, 1.0f, -4.0f);
            P2 = glm::vec3(-1.0f, 1.0f, -4.0f);
            P3 = C;
        }
        else if (tramo == 2)
        {
            // De la esquina trasera izquierda a la delantera izquierda
            P0 = C;
            P1 = glm::vec3(-4.0f, 0.8f, -1.0f);
            P2 = glm::vec3(-4.0f, 0.8f, 1.0f);
            P3 = D;
        }
        else
        {
            // De la esquina delantera izquierda a la delantera derecha
            P0 = D;
            P1 = glm::vec3(-1.0f, 0.8f, 4.0f);
            P2 = glm::vec3(1.0f, 0.8f, 4.0f);
            P3 = A;
        }

        // Evalúa la curva activa para obtener la posición que tendrá el grupo
        // de fantasmas en este instante de la animación.
        glm::vec3 posicionFantasmas =
            bezierCubica(P0, P1, P2, P3, t);


        // Traslada el modelo 
        // completo a la posición calculada sobre la curva.
        glm::mat4 modeloFantasmas = glm::translate(
            glm::mat4(1.0f),
            posicionFantasmas
        );

        dibujarModelo(modelos[5], modeloFantasmas);

        glBindVertexArray(0);


        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // ======================================================================
    //                         LIBERACIÓN DE RECURSOS
    // ======================================================================
    // Libera los buffers, las texturas, el programa de shaders y finalmente
    // la ventana y el contexto de GLFW que se utilizaron durante la escena.
    glDeleteVertexArrays(1, &VAOcuadrado);
    glDeleteBuffers(1, &VBOcuadrado);
    glDeleteBuffers(1, &EBOcuadrado);
    for (ModeloCargado& modelo : modelos)
    {
        for (Pieza& pieza : modelo.piezas)
        {
            glDeleteVertexArrays(1, &pieza.VAO);
            glDeleteBuffers(1, &pieza.VBO);
            glDeleteBuffers(1, &pieza.EBO);
        }
    }
    for (GLuint tex : texturasCargadas)
        if (tex != 0) glDeleteTextures(1, &tex);
    glDeleteTextures(1, &texParedA);
    glDeleteTextures(1, &texParedB);
    glDeleteTextures(1, &texPiso);
    glDeleteTextures(1, &texBlanca);
    glDeleteProgram(ShaderProgram);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}