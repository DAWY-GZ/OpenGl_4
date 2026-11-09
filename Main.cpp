#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <cmath>

using namespace std;

const int WIDTH = 800;
const int HEIGHT = 800;
const int GRID_SIZE = 40; // Aumentado a 40x40 para mayor resolución al dibujar

struct Celda
{
    int x;
    int y;
    float r, g, b; // Color individual por casilla
};

vector<Celda> celdasPintadas;

// Estado del Paint
bool dibujando = false;
int ultimoX = -1;
int ultimoY = -1;

// Color actual seleccionado (Por defecto Cian/Azul)
float colorActualR = 0.1f;
float colorActualG = 0.7f;
float colorActualB = 1.0f;
bool modoGoma = false;

const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec3 aColor;

out vec3 CellColor;

void main()
{
    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
    CellColor = aColor;
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
in vec3 CellColor;
out vec4 FragColor;

uniform bool usarColorUniform;
uniform vec3 colorUniform;

void main()
{
    if (usarColorUniform)
        FragColor = vec4(colorUniform, 1.0);
    else
        FragColor = vec4(CellColor, 1.0);
}
)";

unsigned int crearShader(unsigned int tipo, const char* codigo)
{
    unsigned int shader = glCreateShader(tipo);
    glShaderSource(shader, 1, &codigo, NULL);
    glCompileShader(shader);
    return shader;
}

unsigned int crearPrograma()
{
    unsigned int vertexShader = crearShader(GL_VERTEX_SHADER, vertexShaderSource);
    unsigned int fragmentShader = crearShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    unsigned int programa = glCreateProgram();

    glAttachShader(programa, vertexShader);
    glAttachShader(programa, fragmentShader);
    glLinkProgram(programa);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return programa;
}

void mouseACelda(double mouseX, double mouseY, int& celdaX, int& celdaY)
{
    float pasoPixel = (float)WIDTH / GRID_SIZE;

    celdaX = (int)(mouseX / pasoPixel) + 1;
    celdaY = GRID_SIZE - (int)(mouseY / pasoPixel);

    if (celdaX < 1) celdaX = 1;
    if (celdaX > GRID_SIZE) celdaX = GRID_SIZE;
    if (celdaY < 1) celdaY = 1;
    if (celdaY > GRID_SIZE) celdaY = GRID_SIZE;
}

void borrarCasilla(int x, int y)
{
    for (size_t i = 0; i < celdasPintadas.size(); ++i)
    {
        if (celdasPintadas[i].x == x && celdasPintadas[i].y == y)
        {
            celdasPintadas.erase(celdasPintadas.begin() + i);
            break;
        }
    }
}

void pintarCasilla(int x, int y)
{
    if (x < 1 || x > GRID_SIZE || y < 1 || y > GRID_SIZE) return;

    if (modoGoma)
    {
        borrarCasilla(x, y);
        return;
    }

    // Si ya existe la casilla, actualizamos su color
    for (Celda& celda : celdasPintadas)
    {
        if (celda.x == x && celda.y == y)
        {
            celda.r = colorActualR;
            celda.g = colorActualG;
            celda.b = colorActualB;
            return;
        }
    }

    celdasPintadas.push_back({ x, y, colorActualR, colorActualG, colorActualB });
}

void bresenham(int x0, int y0, int x1, int y1)
{
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    while (true)
    {
        pintarCasilla(x0, y0);
        if (x0 == x1 && y0 == y1) break;

        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx) { err += dx; y0 += sy; }
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (action == GLFW_PRESS)
        {
            dibujando = true;
            double mouseX, mouseY;
            glfwGetCursorPos(window, &mouseX, &mouseY);
            mouseACelda(mouseX, mouseY, ultimoX, ultimoY);
            pintarCasilla(ultimoX, ultimoY);
        }
        else if (action == GLFW_RELEASE)
        {
            dibujando = false;
            ultimoX = -1;
            ultimoY = -1;
        }
    }
}

void cursor_pos_callback(GLFWwindow* window, double mouseX, double mouseY)
{
    if (dibujando)
    {
        int actualX, actualY;
        mouseACelda(mouseX, mouseY, actualX, actualY);

        if (actualX != ultimoX || actualY != ultimoY)
        {
            // Traza la línea continua entre el punto anterior y el actual
            bresenham(ultimoX, ultimoY, actualX, actualY);
            ultimoX = actualX;
            ultimoY = actualY;
        }
    }
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS)
    {
        // Cambiar Colores
        if (key == GLFW_KEY_1) { colorActualR = 1.0f; colorActualG = 0.0f; colorActualB = 0.0f; modoGoma = false; cout << "Color: Rojo" << endl; }
        if (key == GLFW_KEY_2) { colorActualR = 0.0f; colorActualG = 1.0f; colorActualB = 0.0f; modoGoma = false; cout << "Color: Verde" << endl; }
        if (key == GLFW_KEY_3) { colorActualR = 0.0f; colorActualG = 0.0f; colorActualB = 1.0f; modoGoma = false; cout << "Color: Azul" << endl; }
        if (key == GLFW_KEY_4) { colorActualR = 1.0f; colorActualG = 1.0f; colorActualB = 0.0f; modoGoma = false; cout << "Color: Amarillo" << endl; }
        if (key == GLFW_KEY_5) { colorActualR = 0.1f; colorActualG = 0.7f; colorActualB = 1.0f; modoGoma = false; cout << "Color: Cian" << endl; }

        // Modo Goma
        if (key == GLFW_KEY_E || key == GLFW_KEY_0) { modoGoma = true; cout << "Modo: Goma de Borrar" << endl; }

        // Limpiar todo el lienzo
        if (key == GLFW_KEY_C)
        {
            celdasPintadas.clear();
            cout << "Lienzo borrado." << endl;
        }
    }
}

vector<float> crearGrilla()
{
    vector<float> vertices;
    float paso = 2.0f / GRID_SIZE;

    for (int i = 0; i <= GRID_SIZE; i++)
    {
        float pos = -1.0f + i * paso;
        // Verticales
        vertices.push_back(pos); vertices.push_back(-1.0f);
        vertices.push_back(pos); vertices.push_back(1.0f);
        // Horizontales
        vertices.push_back(-1.0f); vertices.push_back(pos);
        vertices.push_back(1.0f);  vertices.push_back(pos);
    }
    return vertices;
}

vector<float> crearCuadrosPintados()
{
    vector<float> vertices;
    float paso = 2.0f / GRID_SIZE;

    for (const Celda& celda : celdasPintadas)
    {
        float x1 = -1.0f + (celda.x - 1) * paso;
        float y1 = -1.0f + (celda.y - 1) * paso;
        float x2 = x1 + paso;
        float y2 = y1 + paso;

        // Triángulo 1
        vertices.push_back(x1); vertices.push_back(y1); vertices.push_back(celda.r); vertices.push_back(celda.g); vertices.push_back(celda.b);
        vertices.push_back(x2); vertices.push_back(y1); vertices.push_back(celda.r); vertices.push_back(celda.g); vertices.push_back(celda.b);
        vertices.push_back(x2); vertices.push_back(y2); vertices.push_back(celda.r); vertices.push_back(celda.g); vertices.push_back(celda.b);

        // Triángulo 2
        vertices.push_back(x1); vertices.push_back(y1); vertices.push_back(celda.r); vertices.push_back(celda.g); vertices.push_back(celda.b);
        vertices.push_back(x2); vertices.push_back(y2); vertices.push_back(celda.r); vertices.push_back(celda.g); vertices.push_back(celda.b);
        vertices.push_back(x1); vertices.push_back(y2); vertices.push_back(celda.r); vertices.push_back(celda.g); vertices.push_back(celda.b);
    }
    return vertices;
}

int main()
{
    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Paint en OpenGL con Bresenham", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }

    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;

    // Callbacks de interactividad
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_pos_callback);
    glfwSetKeyCallback(window, key_callback);

    unsigned int shaderProgram = crearPrograma();
    vector<float> verticesGrilla = crearGrilla();

    unsigned int VAOGrilla, VBOGrilla;
    glGenVertexArrays(1, &VAOGrilla);
    glGenBuffers(1, &VBOGrilla);

    glBindVertexArray(VAOGrilla);
    glBindBuffer(GL_ARRAY_BUFFER, VBOGrilla);
    glBufferData(GL_ARRAY_BUFFER, verticesGrilla.size() * sizeof(float), verticesGrilla.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glViewport(0, 0, WIDTH, HEIGHT);

    cout << "=== CONTROLES DEL PAINT ===" << endl;
    cout << "Click Izquierdo + Arrastrar : Dibujar" << endl;
    cout << "Teclas 1 a 5                : Cambiar Color" << endl;
    cout << "Tecla E                     : Goma de borrar" << endl;
    cout << "Tecla C                     : Limpiar todo el lienzo" << endl;
    cout << "ESC                         : Salir" << endl;

    while (!glfwWindowShouldClose(window))
    {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        glClearColor(0.05f, 0.08f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);

        int usarColorUniformLoc = glGetUniformLocation(shaderProgram, "usarColorUniform");
        int colorUniformLoc = glGetUniformLocation(shaderProgram, "colorUniform");

        // 1. Renderizar Píxeles Pintados
        vector<float> verticesCuadros = crearCuadrosPintados();
        if (!verticesCuadros.empty())
        {
            unsigned int VAOCuadros, VBOCuadros;
            glGenVertexArrays(1, &VAOCuadros);
            glGenBuffers(1, &VBOCuadros);

            glBindVertexArray(VAOCuadros);
            glBindBuffer(GL_ARRAY_BUFFER, VBOCuadros);
            glBufferData(GL_ARRAY_BUFFER, verticesCuadros.size() * sizeof(float), verticesCuadros.data(), GL_DYNAMIC_DRAW);

            // Atributo Posición
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);
            // Atributo Color
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
            glEnableVertexAttribArray(1);

            glUniform1i(usarColorUniformLoc, false); // Usar colores definidos por vértice

            glDrawArrays(GL_TRIANGLES, 0, verticesCuadros.size() / 5);

            glDeleteVertexArrays(1, &VAOCuadros);
            glDeleteBuffers(1, &VBOCuadros);
        }

        // 2. Renderizar Cuadrícula (Líneas blancas)
        glUniform1i(usarColorUniformLoc, true);
        glUniform3f(colorUniformLoc, 0.2f, 0.2f, 0.25f);

        glBindVertexArray(VAOGrilla);
        glDrawArrays(GL_LINES, 0, verticesGrilla.size() / 2);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAOGrilla);
    glDeleteBuffers(1, &VBOGrilla);
    glDeleteProgram(shaderProgram);
    glfwTerminate();

    return 0;
}