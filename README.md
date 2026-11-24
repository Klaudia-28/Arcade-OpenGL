ARCADE OPENGL

- Información general

Nombre del proyecto: Arcade OpenGL  
Autora: Klaudia Sophia Pérez Riaño  
Lenguaje de programación: C++  
Asignatura: Computación Gráfica

- Descripción del proyecto

Arcade OpenGL es un proyecto desarrollado en C++ y OpenGL que consiste en la creación de un entorno tridimensional inspirado en una sala de videojuegos arcade.

El escenario está compuesto por diferentes máquinas recreativas, modelos 3D, texturas e iluminación de colores. Además, incorpora una cámara interactiva y animaciones que permiten dar movimiento a los elementos de la escena.

-Tecnologías utilizadas:

- C++: lenguaje utilizado para desarrollar la aplicación.
- OpenGL: biblioteca gráfica utilizada para la representación del entorno 3D.
- GLFW: gestión de ventanas y entradas del teclado y mouse.
- GLAD: carga de las funciones de OpenGL.
- GLM: operaciones matemáticas y transformaciones mediante vectores y matrices.
- TinyGLTF: lectura y procesamiento de modelos 3D en formato GLTF y GLB.
- STB Image: carga de imágenes utilizadas como texturas.
- GLSL: lenguaje utilizado para programar los shaders.

- Código fuente

El proyecto se encuentra organizado en los siguientes archivos:

- Main.cpp: contiene la inicialización de la aplicación, configuración del escenario, carga de modelos, sistema de cámara, iluminación, animaciones y ciclo principal de renderizado.
- Shader.h: declara la clase encargada de gestionar los shaders.
- Shader.cpp: implementa los procesos de lectura, compilación y enlace de los shaders.
- vertexShader.glsl: procesa los vértices de los modelos y aplica las matrices de transformación.
- fragmentShader.glsl: calcula el color final de los fragmentos mediante las texturas y el sistema de iluminación.
- glad.c: contiene la implementación del cargador de funciones de OpenGL.

- Arquitectura del software

El software está organizado en diferentes componentes que trabajan conjuntamente para construir y representar el entorno tridimensional:

- Inicialización: configura la ventana, el contexto de OpenGL y los recursos necesarios para ejecutar la aplicación.
- Gestión de geometría: utiliza estructuras de datos para almacenar la información de los modelos, sus vértices, índices, texturas y transformaciones. También configura los VAO, VBO y EBO.
- Carga de modelos: utiliza TinyGLTF para leer los archivos GLB, procesar sus nodos, geometrías, materiales, normales y coordenadas de textura.
- Transformaciones geométricas: utiliza matrices de modelo, vista y proyección para posicionar, orientar y representar los objetos dentro del escenario.
- Sistema de cámara: permite recorrer el entorno mediante los controles del teclado y modificar la orientación de la vista utilizando el mouse.
- Sistema de iluminación: implementa iluminación ambiental, difusa y especular, además de atenuación según la distancia. También utiliza texturas emisivas para generar efectos de brillo.
- Animaciones: incorpora la rotación continua de un modelo sobre su propio eje y el desplazamiento de otros objetos mediante curvas de Bézier cúbicas.
- Renderizado: el ciclo principal actualiza el tiempo, procesa las entradas del usuario, calcula las transformaciones y dibuja los elementos de la escena continuamente.

- Controles

- W: avanzar.
- S: retroceder.
- A: desplazarse hacia la izquierda.
- D: desplazarse hacia la derecha.
- Mouse: cambiar la orientación de la cámara.

- Requisitos y ejecución

Para ejecutar el proyecto se requiere:

- Visual Studio con soporte para C++.
- Compatibilidad con OpenGL 3.3 o superior.
- Las bibliotecas y dependencias necesarias para la compilación.
- Los modelos 3D y las texturas utilizados en el escenario.

Para ejecutar la aplicación, se debe abrir el archivo de solución del proyecto (`.sln`) en Visual Studio, verificar la configuración de las dependencias y ejecutar el programa.

- Repositorio

[Arcade OpenGL - GitHub](https://github.com/Klaudia-28/Arcade-OpenGL)
