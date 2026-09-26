#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>

const unsigned int WIDTH = 800;
const unsigned int HEIGHT = 800;

const int GRID_SIZE = 12;

// --------------------------------------------------
// SHADERS
// --------------------------------------------------

const char* vertexShaderSource = R"(
#version 330 core

layout (location = 0) in vec2 aPos;

void main()
{
    gl_Position = vec4(aPos, 0.0, 1.0);
}
)";

const char* fragmentShaderSource = R"(
#version 330 core

out vec4 FragColor;

uniform vec3 color;

void main()
{
    FragColor = vec4(color, 1.0);
}
)";

// --------------------------------------------------
// CREAR SHADER
// --------------------------------------------------

unsigned int crearShader(unsigned int tipo, const char* codigo)
{
    unsigned int shader = glCreateShader(tipo);

    glShaderSource(shader, 1, &codigo, nullptr);
    glCompileShader(shader);

    int success;
    char infoLog[512];

    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (!success)
    {
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);

        std::cout << "ERROR SHADER:\n"
            << infoLog << std::endl;
    }

    return shader;
}

// --------------------------------------------------
// CREAR PROGRAMA
// --------------------------------------------------

unsigned int crearPrograma()
{
    unsigned int vertexShader =
        crearShader(GL_VERTEX_SHADER, vertexShaderSource);

    unsigned int fragmentShader =
        crearShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    unsigned int program = glCreateProgram();

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);

    glLinkProgram(program);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}

// --------------------------------------------------
// MAIN
// --------------------------------------------------

int main()
{
    // GLFW
    if (!glfwInit())
    {
        std::cout << "Error iniciando GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,
        GLFW_OPENGL_CORE_PROFILE);

    // VENTANA
    GLFWwindow* window =
        glfwCreateWindow(
            WIDTH,
            HEIGHT,
            "Grilla 12x12",
            nullptr,
            nullptr
        );

    if (window == nullptr)
    {
        std::cout << "Error creando ventana" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    // GLAD
    if (!gladLoadGLLoader(
        (GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Error iniciando GLAD" << std::endl;
        return -1;
    }

    glViewport(0, 0, WIDTH, HEIGHT);

    // --------------------------------------------------
    // CREAR GRILLA
    // --------------------------------------------------

    std::vector<float> vertices;

    float paso = 2.0f / GRID_SIZE;

    // Líneas verticales
    for (int i = 0; i <= GRID_SIZE; i++)
    {
        float x = -1.0f + i * paso;

        vertices.push_back(x);
        vertices.push_back(-1.0f);

        vertices.push_back(x);
        vertices.push_back(1.0f);
    }

    // Líneas horizontales
    for (int i = 0; i <= GRID_SIZE; i++)
    {
        float y = -1.0f + i * paso;

        vertices.push_back(-1.0f);
        vertices.push_back(y);

        vertices.push_back(1.0f);
        vertices.push_back(y);
    }

    // --------------------------------------------------
    // VAO Y VBO DE LA GRILLA
    // --------------------------------------------------

    unsigned int VAO;
    unsigned int VBO;

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(
        GL_ARRAY_BUFFER,
        vertices.size() * sizeof(float),
        vertices.data(),
        GL_STATIC_DRAW
    );

    glVertexAttribPointer(
        0,
        2,
        GL_FLOAT,
        GL_FALSE,
        2 * sizeof(float),
        (void*)0
    );

    glEnableVertexAttribArray(0);

    // --------------------------------------------------
    // CREAR LOS CUADRADOS DE LA DIAGONAL
    // --------------------------------------------------

    std::vector<float> cuadrados;

    // Desde 2,2 hasta 11,11
    for (int i = 2; i <= 11; i++)
    {
        int fila = i;
        int columna = i;

        float x1 = -1.0f + (columna - 1) * paso;
        float y1 = -1.0f + (GRID_SIZE - fila) * paso;

        float x2 = x1 + paso;
        float y2 = y1 + paso;

        // Primer triángulo
        cuadrados.push_back(x1);
        cuadrados.push_back(y1);

        cuadrados.push_back(x2);
        cuadrados.push_back(y1);

        cuadrados.push_back(x2);
        cuadrados.push_back(y2);

        // Segundo triángulo
        cuadrados.push_back(x2);
        cuadrados.push_back(y2);

        cuadrados.push_back(x1);
        cuadrados.push_back(y2);

        cuadrados.push_back(x1);
        cuadrados.push_back(y1);
    }

    // --------------------------------------------------
    // VAO Y VBO DE LOS CUADRADOS
    // --------------------------------------------------

    unsigned int VAOcuadrados;
    unsigned int VBOcuadrados;

    glGenVertexArrays(1, &VAOcuadrados);
    glGenBuffers(1, &VBOcuadrados);

    glBindVertexArray(VAOcuadrados);

    glBindBuffer(GL_ARRAY_BUFFER, VBOcuadrados);

    glBufferData(
        GL_ARRAY_BUFFER,
        cuadrados.size() * sizeof(float),
        cuadrados.data(),
        GL_STATIC_DRAW
    );

    glVertexAttribPointer(
        0,
        2,
        GL_FLOAT,
        GL_FALSE,
        2 * sizeof(float),
        (void*)0
    );

    glEnableVertexAttribArray(0);

    // --------------------------------------------------
    // SHADER
    // --------------------------------------------------

    unsigned int shaderProgram = crearPrograma();

    int colorLocation =
        glGetUniformLocation(shaderProgram, "color");

    // --------------------------------------------------
    // LOOP
    // --------------------------------------------------

    while (!glfwWindowShouldClose(window))
    {
        // Fondo
        glClearColor(
            0.05f,
            0.10f,
            0.20f,
            1.0f
        );

        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);

        // --------------------------------------------------
        // PINTAR DIAGONAL
        // --------------------------------------------------

        glUniform3f(
            colorLocation,
            0.2f,
            0.7f,
            1.0f
        );

        glBindVertexArray(VAOcuadrados);

        glDrawArrays(
            GL_TRIANGLES,
            0,
            cuadrados.size() / 2
        );

        // --------------------------------------------------
        // DIBUJAR GRILLA ENCIMA
        // --------------------------------------------------

        glUniform3f(
            colorLocation,
            1.0f,
            1.0f,
            1.0f
        );

        glBindVertexArray(VAO);

        glDrawArrays(
            GL_LINES,
            0,
            vertices.size() / 2
        );

        // --------------------------------------------------
        // MOSTRAR
        // --------------------------------------------------

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // --------------------------------------------------
    // LIMPIAR
    // --------------------------------------------------

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);

    glDeleteVertexArrays(1, &VAOcuadrados);
    glDeleteBuffers(1, &VBOcuadrados);

    glDeleteProgram(shaderProgram);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}