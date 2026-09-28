#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <cmath>


using namespace std;


using namespace std;

const int WIDTH = 800;
const int HEIGHT = 800;
const int GRID_SIZE = 20;

struct Celda
{
    int x;
    int y;
};

vector<Celda> celdasPintadas;

bool esperandoCentro = true;
bool esperandoRadio = false;

int centroX = 0;
int centroY = 0;
int radio = 0;

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

void pintarCasilla(int x, int y)
{
    if (x < 1 || x > GRID_SIZE ||
        y < 1 || y > GRID_SIZE)
    {
        return;
    }

    for (const Celda& celda : celdasPintadas)
    {
        if (celda.x == x &&
            celda.y == y)
        {
            return;
        }
    }

    Celda nueva;

    nueva.x = x;
    nueva.y = y;

    celdasPintadas.push_back(nueva);
}

void bresenhamCirculo(int xc, int yc, int radio)
{
    int x = 0;
    int y = radio;

    int d = 3 - 2 * radio;

    cout << endl;
    cout << "BRESENHAM CIRCULO" << endl;

    cout << "Centro: ("
        << xc
        << ", "
        << yc
        << ")"
        << endl;

    cout << "Radio: "
        << radio
        << endl;

    cout << endl;

    while (x <= y)
    {
        pintarCasilla(xc + x, yc + y);
        pintarCasilla(xc - x, yc + y);
        pintarCasilla(xc + x, yc - y);
        pintarCasilla(xc - x, yc - y);

        pintarCasilla(xc + y, yc + x);
        pintarCasilla(xc - y, yc + x);
        pintarCasilla(xc + y, yc - x);
        pintarCasilla(xc - y, yc - x);

        cout << "x = "
            << x
            << " | y = "
            << y
            << endl;

        if (d < 0)
        {
            d = d + 4 * x + 6;
        }
        else
        {
            d = d + 4 * (x - y) + 10;
            y--;
        }

        x++;
    }
}

vector<float> crearGrilla()
{
    vector<float> vertices;

    float paso = 2.0f / GRID_SIZE;

    for (int i = 0; i <= GRID_SIZE; i++)
    {
        float x = -1.0f + i * paso;

        vertices.push_back(x);
        vertices.push_back(-1.0f);

        vertices.push_back(x);
        vertices.push_back(1.0f);
    }

    for (int i = 0; i <= GRID_SIZE; i++)
    {
        float y = -1.0f + i * paso;

        vertices.push_back(-1.0f);
        vertices.push_back(y);

        vertices.push_back(1.0f);
        vertices.push_back(y);
    }

    return vertices;
}

vector<float> crearCuadrosPintados()
{
    vector<float> vertices;

    float paso = 2.0f / GRID_SIZE;

    for (const Celda& celda : celdasPintadas)
    {
        float x1 =
            -1.0f +
            (celda.x - 1) * paso;

        float y1 =
            -1.0f +
            (celda.y - 1) * paso;

        float x2 = x1 + paso;
        float y2 = y1 + paso;

        vertices.push_back(x1);
        vertices.push_back(y1);

        vertices.push_back(x2);
        vertices.push_back(y1);

        vertices.push_back(x2);
        vertices.push_back(y2);

        vertices.push_back(x1);
        vertices.push_back(y1);

        vertices.push_back(x2);
        vertices.push_back(y2);

        vertices.push_back(x1);
        vertices.push_back(y2);
    }

    return vertices;
}

void mouseACelda(
    double mouseX,
    double mouseY,
    int& celdaX,
    int& celdaY
)
{
    float pasoPixel =
        (float)WIDTH / GRID_SIZE;

    celdaX =
        (int)(mouseX / pasoPixel) + 1;

    celdaY =
        GRID_SIZE -
        (int)(mouseY / pasoPixel);

    if (celdaX < 1)
        celdaX = 1;

    if (celdaX > GRID_SIZE)
        celdaX = GRID_SIZE;

    if (celdaY < 1)
        celdaY = 1;

    if (celdaY > GRID_SIZE)
        celdaY = GRID_SIZE;
}

void mouse_callback(
    GLFWwindow* window,
    int button,
    int action,
    int mods
)
{
    if (button != GLFW_MOUSE_BUTTON_LEFT ||
        action != GLFW_PRESS)
    {
        return;
    }

    double mouseX;
    double mouseY;

    glfwGetCursorPos(
        window,
        &mouseX,
        &mouseY
    );

    int x;
    int y;

    mouseACelda(
        mouseX,
        mouseY,
        x,
        y
    );

    if (esperandoCentro)
    {
        centroX = x;
        centroY = y;

        esperandoCentro = false;
        esperandoRadio = true;

        cout << endl;

        cout << "Centro seleccionado: ("
            << centroX
            << ", "
            << centroY
            << ")"
            << endl;

        cout << "Selecciona otra casilla "
            << "para definir el radio."
            << endl;
    }
    else if (esperandoRadio)
    {
        double dx = x - centroX;
        double dy = y - centroY;

        double distancia =
            sqrt(
                dx * dx +
                dy * dy
            );

        radio =
            (int)round(distancia);

        if (radio < 1)
        {
            radio = 1;
        }

        cout << "Punto seleccionado: ("
            << x
            << ", "
            << y
            << ")"
            << endl;

        cout << "Radio calculado: "
            << radio
            << endl;

        celdasPintadas.clear();

        bresenhamCirculo(
            centroX,
            centroY,
            radio
        );

        esperandoCentro = true;
        esperandoRadio = false;

        cout << endl;
        cout << "Circulo terminado." << endl;
        cout << "Haz clic para crear otro." << endl;
    }
}

int main()
{
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

    GLFWwindow* window =
        glfwCreateWindow(
            WIDTH,
            HEIGHT,
            "Bresenham - Circulo 20x20",
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

    if (!gladLoadGLLoader(
        (GLADloadproc)glfwGetProcAddress))
    {
        cout << "Error iniciando GLAD"
            << endl;

        return -1;
    }

    glfwSetMouseButtonCallback(
        window,
        mouse_callback
    );

    unsigned int shaderProgram =
        crearPrograma();

    vector<float> verticesGrilla =
        crearGrilla();

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
        verticesGrilla.size() *
        sizeof(float),
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

    glViewport(
        0,
        0,
        WIDTH,
        HEIGHT
    );

    glLineWidth(2.0f);

    while (!glfwWindowShouldClose(window))
    {
        if (glfwGetKey(
            window,
            GLFW_KEY_ESCAPE
        ) == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(
                window,
                true
            );
        }

        if (glfwGetKey(
            window,
            GLFW_KEY_R
        ) == GLFW_PRESS)
        {
            celdasPintadas.clear();

            esperandoCentro = true;
            esperandoRadio = false;

            cout << "Circulo reiniciado."
                << endl;

            glfwWaitEventsTimeout(0.15);
        }

        glClearColor(
            0.05f,
            0.08f,
            0.15f,
            1.0f
        );

        glClear(
            GL_COLOR_BUFFER_BIT
        );

        vector<float> verticesCuadros =
            crearCuadrosPintados();

        glUseProgram(
            shaderProgram
        );

        int colorLocation =
            glGetUniformLocation(
                shaderProgram,
                "color"
            );

        if (!verticesCuadros.empty())
        {
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

            glBufferData(
                GL_ARRAY_BUFFER,
                verticesCuadros.size() *
                sizeof(float),
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

            glUniform3f(
                colorLocation,
                0.1f,
                0.7f,
                1.0f
            );

            glDrawArrays(
                GL_TRIANGLES,
                0,
                verticesCuadros.size() / 2
            );

            glDeleteVertexArrays(
                1,
                &VAOCuadros
            );

            glDeleteBuffers(
                1,
                &VBOCuadros
            );
        }

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

        glfwSwapBuffers(window);

        glfwPollEvents();
    }

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