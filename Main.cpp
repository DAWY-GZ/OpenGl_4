#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <cmath>

using namespace std;

// configuracion 

const int WIDTH = 800;
const int HEIGHT = 800;

const int GRID_SIZE = 20;

// estructura para la celdaaa
struct Celda
{
    int x;
    int y;
};


// celdas que Bresenham va a pintar
vector<Celda> celdasPintadas;


// Estado
bool esperandoInicio = true;
bool esperandoFinal = false;


// P inicio
int inicioX = 0;
int inicioY = 0;

// P final
int finalX = 0;
int finalY = 0;

// shaders

const char* vertexShaderSource = R"(
#version 330 core

layout (location = 0) in vec2 aPos;

void main()
{
    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
}
)";


const char* fragmentShaderSource = R"(
#version 330 core

uniform vec3 color;

out vec4 FragColor;

void main()
{
    FragColor = vec4(color, 1.0);
}
)";

// crear el shader

unsigned int crearShader(unsigned int tipo, const char* codigo)
{
    unsigned int shader = glCreateShader(tipo);

    glShaderSource(shader, 1, &codigo, NULL);
    glCompileShader(shader);

    int success;
    char infoLog[512];

    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (!success)
    {
        glGetShaderInfoLog(
            shader,
            512,
            NULL,
            infoLog
        );

        cout << "ERROR SHADER:\n"
            << infoLog
            << endl;
    }

    return shader;
}


// crear el programa

unsigned int crearPrograma()
{
    unsigned int vertexShader =
        crearShader(
            GL_VERTEX_SHADER,
            vertexShaderSource
        );

    unsigned int fragmentShader =
        crearShader(
            GL_FRAGMENT_SHADER,
            fragmentShaderSource
        );

    unsigned int programa =
        glCreateProgram();

    glAttachShader(
        programa,
        vertexShader
    );

    glAttachShader(
        programa,
        fragmentShader
    );

    glLinkProgram(programa);

    int success;
    char infoLog[512];

    glGetProgramiv(
        programa,
        GL_LINK_STATUS,
        &success
    );

    if (!success)
    {
        glGetProgramInfoLog(
            programa,
            512,
            NULL,
            infoLog
        );

        cout << "ERROR PROGRAMA:\n"
            << infoLog
            << endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return programa;
}


// pintar una celda

void pintarCasilla(int x, int y)
{
    // verif límites

    if (x < 1 || x > GRID_SIZE ||
        y < 1 || y > GRID_SIZE)
    {
        return;
    }


	// no repetir celdas

    for (const Celda& celda : celdasPintadas)
    {
        if (celda.x == x &&
            celda.y == y)
        {
            return;
        }
    }


    // Agregar celda

    Celda nueva;

    nueva.x = x;
    nueva.y = y;

    celdasPintadas.push_back(nueva);
}

// Bresenham

void bresenham(
    int x0,
    int y0,
    int x1,
    int y1
)
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);

    int sx;

    if (x0 < x1)
        sx = 1;
    else
        sx = -1;

    int sy;

    if (y0 < y1)
        sy = 1;
    else
        sy = -1;

    int error = dx - dy;


    while (true)
    {
        // Pintar la celda actual

        pintarCasilla(x0, y0);


        // Mostrar
        cout << "("
            << x0
            << ", "
            << y0
            << ")"
            << endl;


        // punto final

        if (x0 == x1 &&
            y0 == y1)
        {
            break;
        }


        int e2 = 2 * error;


        if (e2 > -dy)
        {
            error -= dy;
            x0 += sx;
        }


        if (e2 < dx)
        {
            error += dx;
            y0 += sy;
        }
    }
}

// CREAR GRILLA

vector<float> crearGrilla()
{
    vector<float> vertices;


    float paso =
        2.0f / GRID_SIZE;

    // verticales

    for (int i = 0;
        i <= GRID_SIZE;
        i++)
    {
        float x =
            -1.0f + i * paso;


        vertices.push_back(x);
        vertices.push_back(-1.0f);


        vertices.push_back(x);
        vertices.push_back(1.0f);
    }


    // horizontales

    for (int i = 0;
        i <= GRID_SIZE;
        i++)
    {
        float y =
            -1.0f + i * paso;


        vertices.push_back(-1.0f);
        vertices.push_back(y);


        vertices.push_back(1.0f);
        vertices.push_back(y);
    }


    return vertices;
}

// crear cuadros pintados

vector<float> crearCuadrosPintados()
{
    vector<float> vertices;


    float paso =
        2.0f / GRID_SIZE;


    for (const Celda& celda :
        celdasPintadas)
    {
        // Esquina inferior izquierda

        float x1 =
            -1.0f +
            (celda.x - 1) * paso;

        float y1 =
            -1.0f +
            (celda.y - 1) * paso;


        // Esquina superior derecha

        float x2 =
            x1 + paso;

        float y2 =
            y1 + paso;

        // primer triangulo

        vertices.push_back(x1);
        vertices.push_back(y1);

        vertices.push_back(x2);
        vertices.push_back(y1);

        vertices.push_back(x2);
        vertices.push_back(y2);

		// segundo triangulo

        vertices.push_back(x1);
        vertices.push_back(y1);

        vertices.push_back(x2);
        vertices.push_back(y2);

        vertices.push_back(x1);
        vertices.push_back(y2);
    }


    return vertices;
}

// mouse a celda

void mouseACelda(
    double mouseX,
    double mouseY,
    int& celdaX,
    int& celdaY
)
{
    // Convertir posición del mouse
    // a coordenadas de la ventana

    float pasoPixel =
        (float)WIDTH / GRID_SIZE;


    // Columna

    celdaX =
        (int)(mouseX / pasoPixel) + 1;


    // Fila
    //
    // Se invierte Y porque en OpenGL
    // el origen está abajo,
    // mientras que el mouse empieza arriba.

    celdaY =
        GRID_SIZE -
        (int)(mouseY / pasoPixel);


    // Asegurar límites

    if (celdaX < 1)
        celdaX = 1;

    if (celdaX > GRID_SIZE)
        celdaX = GRID_SIZE;


    if (celdaY < 1)
        celdaY = 1;

    if (celdaY > GRID_SIZE)
        celdaY = GRID_SIZE;
}

// CALLBACK DEL MOUSE

void mouse_callback(
    GLFWwindow* window,
    int button,
    int action,
    int mods
)
{
    // Solo nos interesa
    // el clic izquierdo

    if (button != GLFW_MOUSE_BUTTON_LEFT ||
        action != GLFW_PRESS)
    {
        return;
    }


    // Obtener posición del mouse

    double mouseX;
    double mouseY;

    glfwGetCursorPos(
        window,
        &mouseX,
        &mouseY
    );


    // Convertir a coordenadas
    // de la grilla

    int x;
    int y;

    mouseACelda(
        mouseX,
        mouseY,
        x,
        y
    );

    // PRIMER CLIC

    if (esperandoInicio)
    {
        inicioX = x;
        inicioY = y;

        esperandoInicio = false;
        esperandoFinal = true;


        cout << endl;
        cout << "Punto inicial: ("
            << inicioX
            << ", "
            << inicioY
            << ")"
            << endl;

        cout << "Ahora selecciona el punto final..."
            << endl;
    }

    // SEGUNDO CLIC

    else if (esperandoFinal)
    {
        finalX = x;
        finalY = y;

        esperandoFinal = false;


        cout << "Punto final: ("
            << finalX
            << ", "
            << finalY
            << ")"
            << endl;


        cout << endl;
        cout << "BRESENHAM:" << endl;


        // Limpiar celdas anteriores

        celdasPintadas.clear();


        // Ejecutar Bresenham

        bresenham(
            inicioX,
            inicioY,
            finalX,
            finalY
        );


        cout << endl;
        cout << "Linea terminada."
            << endl;

        cout << "Haz clic nuevamente "
            << "para crear otra linea."
            << endl;


        // Preparar nuevo inicio

        esperandoInicio = true;
    }
}


// MAIN

int main()
{

    // INICIAR GLFW

    if (!glfwInit())
    {
        cout << "Error iniciando GLFW"
            << endl;

        return -1;
    }


    glfwWindowHint(
        GLFW_CONTEXT_VERSION_MAJOR,
        3
    );

    glfwWindowHint(
        GLFW_CONTEXT_VERSION_MINOR,
        3
    );

    glfwWindowHint(
        GLFW_OPENGL_PROFILE,
        GLFW_OPENGL_CORE_PROFILE
    );

    // CREAR VENTANA

    GLFWwindow* window =
        glfwCreateWindow(
            WIDTH,
            HEIGHT,
            "Bresenham - Grilla 20x20",
            NULL,
            NULL
        );


    if (window == NULL)
    {
        cout << "Error creando ventana"
            << endl;

        glfwTerminate();

        return -1;
    }


    glfwMakeContextCurrent(window);

    // INICIAR GLAD

    if (!gladLoadGLLoader(
        (GLADloadproc)glfwGetProcAddress))
    {
        cout << "Error iniciando GLAD"
            << endl;

        return -1;
    }

    // CALLBACK DEL MOUSE

    glfwSetMouseButtonCallback(
        window,
        mouse_callback
    );

    // CREAR SHADER

    unsigned int shaderProgram =
        crearPrograma();

    // CREAR GRILLA

    vector<float> verticesGrilla =
        crearGrilla();

    // VAO Y VBO DE LA GRILLA

    unsigned int VAOGrilla;
    unsigned int VBOGrilla;


    glGenVertexArrays(
        1,
        &VAOGrilla
    );

    glGenBuffers(
        1,
        &VBOGrilla
    );


    glBindVertexArray(
        VAOGrilla
    );


    glBindBuffer(
        GL_ARRAY_BUFFER,
        VBOGrilla
    );


    glBufferData(
        GL_ARRAY_BUFFER,
        verticesGrilla.size()
        * sizeof(float),

        verticesGrilla.data(),

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

    // CONFIGURACIÓN OPENGL

    glViewport(
        0,
        0,
        WIDTH,
        HEIGHT
    );


    glLineWidth(2.0f);

    // BUCLE PRINCIPAL

    while (!glfwWindowShouldClose(window))
    {

        // FONDO

        glClearColor(
            0.05f,
            0.08f,
            0.15f,
            1.0f
        );


        glClear(
            GL_COLOR_BUFFER_BIT
        );

        // CREAR CUADROS ACTUALES

        vector<float> verticesCuadros =
            crearCuadrosPintados();

        // CREAR VAO/VBO PARA LOS CUADROS

        unsigned int VAOCuadros;
        unsigned int VBOCuadros;


        glGenVertexArrays(
            1,
            &VAOCuadros
        );

        glGenBuffers(
            1,
            &VBOCuadros
        );


        glBindVertexArray(
            VAOCuadros
        );


        glBindBuffer(
            GL_ARRAY_BUFFER,
            VBOCuadros
        );


        if (!verticesCuadros.empty())
        {
            glBufferData(
                GL_ARRAY_BUFFER,
                verticesCuadros.size()
                * sizeof(float),

                verticesCuadros.data(),

                GL_DYNAMIC_DRAW
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
        }

        // USAR SHADER

        glUseProgram(
            shaderProgram
        );


        int colorLocation =
            glGetUniformLocation(
                shaderProgram,
                "color"
            );

        // PINTAR CUADROS

        if (!verticesCuadros.empty())
        {
            glUniform3f(
                colorLocation,
                0.1f,
                0.7f,
                1.0f
            );


            glBindVertexArray(
                VAOCuadros
            );


            glDrawArrays(
                GL_TRIANGLES,
                0,
                verticesCuadros.size() / 2
            );
        }

        // DIBUJAR GRILLA

        glUniform3f(
            colorLocation,
            1.0f,
            1.0f,
            1.0f
        );


        glBindVertexArray(
            VAOGrilla
        );


        glDrawArrays(
            GL_LINES,
            0,
            verticesGrilla.size() / 2
        );

        // ACTUALIZAR

        glfwSwapBuffers(window);

        glfwPollEvents();

        // BORRAR VAO/VBO TEMPORALES

        glDeleteVertexArrays(
            1,
            &VAOCuadros
        );

        glDeleteBuffers(
            1,
            &VBOCuadros
        );
    }

    // LIMPIAR

    glDeleteVertexArrays(
        1,
        &VAOGrilla
    );

    glDeleteBuffers(
        1,
        &VBOGrilla
    );

    glDeleteProgram(
        shaderProgram
    );


    glfwTerminate();

    return 0;
}