#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <string>

using namespace std;

const int WIDTH = 800;
const int HEIGHT = 800;
const int PANEL_W = 260;
const int WINDOW_W = PANEL_W + WIDTH;
const int WINDOW_H = HEIGHT;
const int GRID_SIZE = 40;

enum Herramienta
{
    PINCEL_LIBRE = 0,
    PINCEL_LINEA = 1,
    PINCEL_CIRCULO = 2,
    PINCEL_CUADRADO = 3,
    PINCEL_TRIANGULO = 4,
    PINCEL_RELLENO = 5,
    PINCEL_GOMA = 6
};

struct Celda
{
    int x;
    int y;
    float r, g, b;
};

struct ColorRGB
{
    float r, g, b;
    const char* nombre;
};

vector<Celda> celdasPintadas;
vector<Celda> vistaPrevia;

bool dibujando = false;
bool trazandoForma = false;
int ultimoX = -1;
int ultimoY = -1;
int anclaX = -1;
int anclaY = -1;

float colorActualR = 0.10f;
float colorActualG = 0.70f;
float colorActualB = 1.00f;
int colorSeleccionado = 4;
int herramienta = PINCEL_LIBRE;

ColorRGB paleta[] = {
    { 1.00f, 0.15f, 0.15f, "Rojo" },
    { 0.15f, 0.85f, 0.25f, "Verde" },
    { 0.20f, 0.40f, 1.00f, "Azul" },
    { 1.00f, 0.90f, 0.15f, "Amarillo" },
    { 0.10f, 0.70f, 1.00f, "Cian" },
    { 1.00f, 1.00f, 1.00f, "Blanco" },
    { 1.00f, 0.50f, 0.10f, "Naranja" },
    { 0.90f, 0.20f, 0.75f, "Magenta" },
    { 0.08f, 0.08f, 0.08f, "Negro" },
    { 0.55f, 0.55f, 0.60f, "Gris" }
};

void reconstruirPrevia();

// --- Shaders ---
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

// --- Conversiones de coordenadas ---
float pxToNdcX(float px)
{
    return (px / (float)WINDOW_W) * 2.0f - 1.0f;
}

float pxToNdcY(float py)
{
    return 1.0f - (py / (float)WINDOW_H) * 2.0f;
}

float origenLienzoX()
{
    return (float)PANEL_W / WINDOW_W * 2.0f - 1.0f;
}

float pasoCeldaX()
{
    return (1.0f - origenLienzoX()) / GRID_SIZE;
}

float pasoCeldaY()
{
    return 2.0f / GRID_SIZE;
}

// --- Estado de color y herramienta ---
void aplicarColor(int indice)
{
    colorSeleccionado = indice;
    colorActualR = paleta[indice].r;
    colorActualG = paleta[indice].g;
    colorActualB = paleta[indice].b;
    if (herramienta == PINCEL_GOMA)
        herramienta = PINCEL_LIBRE;
    if (trazandoForma)
        reconstruirPrevia();
    cout << "Color: " << paleta[indice].nombre << endl;
}

void aplicarHerramienta(int id)
{
    herramienta = id;
    vistaPrevia.clear();
    trazandoForma = false;
    const char* nombres[] = {
        "Libre",
        "Linea Bresenham",
        "Circulo Bresenham",
        "Cuadrado Bresenham",
        "Triangulo Bresenham",
        "Rellenar",
        "Borrar"
    };
    cout << "Pincel: " << nombres[id] << endl;
}

bool mouseACeldaLienzo(double mouseX, double mouseY, int& celdaX, int& celdaY)
{
    double localX = mouseX - PANEL_W;
    if (localX < 0.0 || localX >= WIDTH || mouseY < 0.0 || mouseY >= HEIGHT)
        return false;

    float pasoPixel = (float)WIDTH / GRID_SIZE;
    celdaX = (int)(localX / pasoPixel) + 1;
    celdaY = GRID_SIZE - (int)(mouseY / pasoPixel);

    if (celdaX < 1) celdaX = 1;
    if (celdaX > GRID_SIZE) celdaX = GRID_SIZE;
    if (celdaY < 1) celdaY = 1;
    if (celdaY > GRID_SIZE) celdaY = GRID_SIZE;
    return true;
}

// --- Edicion de celdas ---
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

    if (herramienta == PINCEL_GOMA)
    {
        borrarCasilla(x, y);
        return;
    }

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

void marcarPrevia(int x, int y)
{
    if (x < 1 || x > GRID_SIZE || y < 1 || y > GRID_SIZE) return;
    vistaPrevia.push_back({ x, y, colorActualR, colorActualG, colorActualB });
}

// --- Algoritmos Bresenham ---
void trazarLinea(int x0, int y0, int x1, int y1, bool previa)
{
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    while (true)
    {
        if (previa) marcarPrevia(x0, y0);
        else pintarCasilla(x0, y0);

        if (x0 == x1 && y0 == y1) break;

        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx) { err += dx; y0 += sy; }
    }
}

void trazarCirculo(int xc, int yc, int radio, bool previa)
{
    if (radio < 0) return;
    if (radio == 0)
    {
        if (previa) marcarPrevia(xc, yc);
        else pintarCasilla(xc, yc);
        return;
    }

    int x = 0;
    int y = radio;
    int d = 3 - 2 * radio;

    while (y >= x)
    {
        int px[8] = { xc + x, xc - x, xc + x, xc - x, xc + y, xc - y, xc + y, xc - y };
        int py[8] = { yc + y, yc + y, yc - y, yc - y, yc + x, yc + x, yc - x, yc - x };
        for (int i = 0; i < 8; i++)
        {
            if (previa) marcarPrevia(px[i], py[i]);
            else pintarCasilla(px[i], py[i]);
        }

        x++;
        if (d > 0)
        {
            y--;
            d = d + 4 * (x - y) + 10;
        }
        else
        {
            d = d + 4 * x + 6;
        }
    }
}

void trazarCuadrado(int x0, int y0, int x1, int y1, bool previa)
{
    trazarLinea(x0, y0, x1, y0, previa);
    trazarLinea(x1, y0, x1, y1, previa);
    trazarLinea(x1, y1, x0, y1, previa);
    trazarLinea(x0, y1, x0, y0, previa);
}

void trazarTriangulo(int x0, int y0, int x1, int y1, bool previa)
{
    int vx = x0;
    int vy = y1;
    trazarLinea(x0, y0, x1, y1, previa);
    trazarLinea(x1, y1, vx, vy, previa);
    trazarLinea(vx, vy, x0, y0, previa);
}

// --- Relleno (Flood Fill) ---
bool coloresIguales(float r1, float g1, float b1, float r2, float g2, float b2)
{
    return fabs(r1 - r2) < 0.001f &&
        fabs(g1 - g2) < 0.001f &&
        fabs(b1 - b2) < 0.001f;
}

void rellenarRegion(int inicioX, int inicioY)
{
    if (inicioX < 1 || inicioX > GRID_SIZE || inicioY < 1 || inicioY > GRID_SIZE) return;

    bool tieneColor[GRID_SIZE + 1][GRID_SIZE + 1] = { false };
    float mapaR[GRID_SIZE + 1][GRID_SIZE + 1] = { { 0.0f } };
    float mapaG[GRID_SIZE + 1][GRID_SIZE + 1] = { { 0.0f } };
    float mapaB[GRID_SIZE + 1][GRID_SIZE + 1] = { { 0.0f } };

    for (const Celda& c : celdasPintadas)
    {
        if (c.x >= 1 && c.x <= GRID_SIZE && c.y >= 1 && c.y <= GRID_SIZE)
        {
            tieneColor[c.x][c.y] = true;
            mapaR[c.x][c.y] = c.r;
            mapaG[c.x][c.y] = c.g;
            mapaB[c.x][c.y] = c.b;
        }
    }

    bool objetivoPintado = tieneColor[inicioX][inicioY];
    float objR = mapaR[inicioX][inicioY];
    float objG = mapaG[inicioX][inicioY];
    float objB = mapaB[inicioX][inicioY];

    if (objetivoPintado && coloresIguales(objR, objG, objB, colorActualR, colorActualG, colorActualB))
        return;

    auto coincideObjetivo = [&](int x, int y) -> bool {
        if (!objetivoPintado)
            return !tieneColor[x][y];
        return tieneColor[x][y] && coloresIguales(mapaR[x][y], mapaG[x][y], mapaB[x][y], objR, objG, objB);
        };

    bool visitado[GRID_SIZE + 1][GRID_SIZE + 1] = { false };
    vector<pair<int, int>> pila;
    pila.push_back({ inicioX, inicioY });
    visitado[inicioX][inicioY] = true;

    while (!pila.empty())
    {
        pair<int, int> actual = pila.back();
        pila.pop_back();

        int cx = actual.first;
        int cy = actual.second;
        pintarCasilla(cx, cy);

        const int dirs[4][2] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };
        for (int i = 0; i < 4; i++)
        {
            int nx = cx + dirs[i][0];
            int ny = cy + dirs[i][1];
            if (nx >= 1 && nx <= GRID_SIZE && ny >= 1 && ny <= GRID_SIZE)
            {
                if (!visitado[nx][ny] && coincideObjetivo(nx, ny))
                {
                    visitado[nx][ny] = true;
                    pila.push_back({ nx, ny });
                }
            }
        }
    }
}

void reconstruirPrevia()
{
    vistaPrevia.clear();
    if (!trazandoForma) return;

    if (herramienta == PINCEL_LINEA)
        trazarLinea(anclaX, anclaY, ultimoX, ultimoY, true);
    else if (herramienta == PINCEL_CIRCULO)
    {
        int dx = ultimoX - anclaX;
        int dy = ultimoY - anclaY;
        int radio = (int)round(sqrt((double)dx * dx + (double)dy * dy));
        trazarCirculo(anclaX, anclaY, radio, true);
    }
    else if (herramienta == PINCEL_CUADRADO)
        trazarCuadrado(anclaX, anclaY, ultimoX, ultimoY, true);
    else if (herramienta == PINCEL_TRIANGULO)
        trazarTriangulo(anclaX, anclaY, ultimoX, ultimoY, true);
}

void confirmarForma()
{
    if (!trazandoForma) return;

    if (herramienta == PINCEL_LINEA)
        trazarLinea(anclaX, anclaY, ultimoX, ultimoY, false);
    else if (herramienta == PINCEL_CIRCULO)
    {
        int dx = ultimoX - anclaX;
        int dy = ultimoY - anclaY;
        int radio = (int)round(sqrt((double)dx * dx + (double)dy * dy));
        trazarCirculo(anclaX, anclaY, radio, false);
    }
    else if (herramienta == PINCEL_CUADRADO)
        trazarCuadrado(anclaX, anclaY, ultimoX, ultimoY, false);
    else if (herramienta == PINCEL_TRIANGULO)
        trazarTriangulo(anclaX, anclaY, ultimoX, ultimoY, false);

    vistaPrevia.clear();
    trazandoForma = false;
}

// --- Geometria del panel izquierdo (estilo visual de la captura) ---
struct RectPx { float x, y, w, h; };

RectPx rectColorSwatch(int i)
{
    float inicioX = 16.0f;
    float inicioY = 66.0f;
    float tam = 36.0f;
    float sep = 8.0f;
    int col = i % 5;
    int fila = i / 5;
    return { inicioX + col * (tam + sep), inicioY + fila * (tam + sep), tam, tam };
}

int numBotonesHerramienta() { return 8; } // Libre, Linea, Circulo, Cuadrado, Triangulo, Rellenar, Borrar, Limpiar

RectPx rectBotonHerramienta(int idx)
{
    float x = 16.0f;
    float y0 = 196.0f;
    float w = PANEL_W - 32.0f;
    float h = 42.0f;
    float gap = 8.0f;
    float extraGap = (idx == 7) ? 10.0f : 0.0f; // Separacion extra antes de LIMPIAR
    return { x, y0 + idx * (h + gap) + extraGap, w, h };
}

RectPx rectMuestraColorActual()
{
    return { 16.0f, 676.0f, 44.0f, 44.0f };
}

bool dentroRect(double mx, double my, const RectPx& r)
{
    return mx >= r.x && mx <= r.x + r.w && my >= r.y && my <= r.y + r.h;
}

// --- Callbacks ---
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button != GLFW_MOUSE_BUTTON_LEFT) return;

    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);

    if (action == GLFW_PRESS)
    {
        if (mouseX < PANEL_W)
        {
            for (int i = 0; i < 10; i++)
            {
                if (dentroRect(mouseX, mouseY, rectColorSwatch(i)))
                {
                    aplicarColor(i);
                    return;
                }
            }

            for (int idx = 0; idx < numBotonesHerramienta(); idx++)
            {
                if (dentroRect(mouseX, mouseY, rectBotonHerramienta(idx)))
                {
                    if (idx == 0) aplicarHerramienta(PINCEL_LIBRE);
                    else if (idx == 1) aplicarHerramienta(PINCEL_LINEA);
                    else if (idx == 2) aplicarHerramienta(PINCEL_CIRCULO);
                    else if (idx == 3) aplicarHerramienta(PINCEL_CUADRADO);
                    else if (idx == 4) aplicarHerramienta(PINCEL_TRIANGULO);
                    else if (idx == 5) aplicarHerramienta(PINCEL_RELLENO);
                    else if (idx == 6) aplicarHerramienta(PINCEL_GOMA);
                    else if (idx == 7)
                    {
                        celdasPintadas.clear();
                        vistaPrevia.clear();
                        cout << "Lienzo borrado." << endl;
                    }
                    return;
                }
            }
            return;
        }

        int celdaX, celdaY;
        if (!mouseACeldaLienzo(mouseX, mouseY, celdaX, celdaY))
            return;

        if (herramienta == PINCEL_RELLENO)
        {
            rellenarRegion(celdaX, celdaY);
            return;
        }

        dibujando = true;
        ultimoX = celdaX;
        ultimoY = celdaY;

        if (herramienta == PINCEL_LIBRE || herramienta == PINCEL_GOMA)
            pintarCasilla(celdaX, celdaY);
        else
        {
            anclaX = celdaX;
            anclaY = celdaY;
            trazandoForma = true;
            reconstruirPrevia();
        }
    }
    else if (action == GLFW_RELEASE)
    {
        if (trazandoForma)
            confirmarForma();

        dibujando = false;
        ultimoX = -1;
        ultimoY = -1;
    }
}

void cursor_pos_callback(GLFWwindow* window, double mouseX, double mouseY)
{
    if (!dibujando) return;

    int actualX, actualY;
    if (!mouseACeldaLienzo(mouseX, mouseY, actualX, actualY))
        return;

    if (actualX == ultimoX && actualY == ultimoY)
        return;

    if (herramienta == PINCEL_LIBRE || herramienta == PINCEL_GOMA)
    {
        trazarLinea(ultimoX, ultimoY, actualX, actualY, false);
        ultimoX = actualX;
        ultimoY = actualY;
    }
    else if (trazandoForma)
    {
        ultimoX = actualX;
        ultimoY = actualY;
        reconstruirPrevia();
    }
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action != GLFW_PRESS) return;

    if (key == GLFW_KEY_1) aplicarColor(0);
    if (key == GLFW_KEY_2) aplicarColor(1);
    if (key == GLFW_KEY_3) aplicarColor(2);
    if (key == GLFW_KEY_4) aplicarColor(3);
    if (key == GLFW_KEY_5) aplicarColor(4);
    if (key == GLFW_KEY_6) aplicarColor(5);
    if (key == GLFW_KEY_7) aplicarColor(6);
    if (key == GLFW_KEY_8) aplicarColor(7);
    if (key == GLFW_KEY_9) aplicarColor(8);
    if (key == GLFW_KEY_0) aplicarColor(9);

    if (key == GLFW_KEY_F) aplicarHerramienta(PINCEL_LIBRE);
    if (key == GLFW_KEY_L) aplicarHerramienta(PINCEL_LINEA);
    if (key == GLFW_KEY_O) aplicarHerramienta(PINCEL_CIRCULO);
    if (key == GLFW_KEY_Q) aplicarHerramienta(PINCEL_CUADRADO);
    if (key == GLFW_KEY_T) aplicarHerramienta(PINCEL_TRIANGULO);
    if (key == GLFW_KEY_R) aplicarHerramienta(PINCEL_RELLENO);
    if (key == GLFW_KEY_E) aplicarHerramienta(PINCEL_GOMA);

    if (key == GLFW_KEY_C)
    {
        celdasPintadas.clear();
        vistaPrevia.clear();
        cout << "Lienzo borrado." << endl;
    }
}

// --- Dibujado de geometria y texto bitmap en OpenGL ---
void pushQuadPx(vector<float>& v, float x, float y, float w, float h, float r, float g, float b)
{
    float x1 = pxToNdcX(x);
    float y1 = pxToNdcY(y + h);
    float x2 = pxToNdcX(x + w);
    float y2 = pxToNdcY(y);

    float tri[6][5] = {
        { x1, y1, r, g, b },
        { x2, y1, r, g, b },
        { x2, y2, r, g, b },
        { x1, y1, r, g, b },
        { x2, y2, r, g, b },
        { x1, y2, r, g, b }
    };
    for (int i = 0; i < 6; i++)
        for (int j = 0; j < 5; j++)
            v.push_back(tri[i][j]);
}

void pushBordePx(vector<float>& lines, float x, float y, float w, float h)
{
    float x1 = pxToNdcX(x);
    float y1 = pxToNdcY(y);
    float x2 = pxToNdcX(x + w);
    float y2 = pxToNdcY(y + h);

    float segs[] = {
        x1, y1, x2, y1,
        x2, y1, x2, y2,
        x2, y2, x1, y2,
        x1, y2, x1, y1
    };
    lines.insert(lines.end(), begin(segs), end(segs));
}

// Fuente bitmap 5x7 compacta en mayusculas para etiquetas de botones y titulos
const uint8_t* glifo5x7(char c)
{
    static const uint8_t A[7] = { 0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11 };
    static const uint8_t B[7] = { 0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E };
    static const uint8_t C[7] = { 0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E };
    static const uint8_t D[7] = { 0x1C, 0x12, 0x11, 0x11, 0x11, 0x12, 0x1C };
    static const uint8_t E[7] = { 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F };
    static const uint8_t G[7] = { 0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F };
    static const uint8_t I[7] = { 0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x1F };
    static const uint8_t L[7] = { 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F };
    static const uint8_t M[7] = { 0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11 };
    static const uint8_t N[7] = { 0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11 };
    static const uint8_t O[7] = { 0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E };
    static const uint8_t P[7] = { 0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10 };
    static const uint8_t Q[7] = { 0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D };
    static const uint8_t R[7] = { 0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11 };
    static const uint8_t S[7] = { 0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E };
    static const uint8_t T[7] = { 0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04 };
    static const uint8_t U[7] = { 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E };
    static const uint8_t ESP[7] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

    switch (toupper((unsigned char)c))
    {
    case 'A': return A; case 'B': return B; case 'C': return C; case 'D': return D;
    case 'E': return E; case 'G': return G; case 'I': return I; case 'L': return L;
    case 'M': return M; case 'N': return N; case 'O': return O; case 'P': return P;
    case 'Q': return Q; case 'R': return R; case 'S': return S; case 'T': return T;
    case 'U': return U; default: return ESP;
    }
}

void pushTextoPx(vector<float>& quads, const string& texto, float x, float y, float escala, float r, float g, float b, bool centrado, float anchoCaja = 0.0f)
{
    float anchoChar = 5.0f * escala;
    float sepChar = 1.0f * escala;
    float anchoTotal = texto.size() * anchoChar + (texto.size() > 0 ? (texto.size() - 1) * sepChar : 0.0f);
    float cursorX = centrado ? (x + (anchoCaja - anchoTotal) * 0.5f) : x;

    for (char c : texto)
    {
        const uint8_t* gBits = glifo5x7(c);
        for (int fila = 0; fila < 7; fila++)
        {
            for (int col = 0; col < 5; col++)
            {
                if (gBits[fila] & (1 << (4 - col)))
                {
                    pushQuadPx(quads, cursorX + col * escala, y + fila * escala, escala, escala, r, g, b);
                }
            }
        }
        cursorX += anchoChar + sepChar;
    }
}

void agregarCelda(vector<float>& vertices, const Celda& celda)
{
    float pasoX = pasoCeldaX();
    float pasoY = pasoCeldaY();
    float x1 = origenLienzoX() + (celda.x - 1) * pasoX;
    float y1 = -1.0f + (celda.y - 1) * pasoY;
    float x2 = x1 + pasoX;
    float y2 = y1 + pasoY;

    vertices.push_back(x1); vertices.push_back(y1); vertices.push_back(celda.r); vertices.push_back(celda.g); vertices.push_back(celda.b);
    vertices.push_back(x2); vertices.push_back(y1); vertices.push_back(celda.r); vertices.push_back(celda.g); vertices.push_back(celda.b);
    vertices.push_back(x2); vertices.push_back(y2); vertices.push_back(celda.r); vertices.push_back(celda.g); vertices.push_back(celda.b);

    vertices.push_back(x1); vertices.push_back(y1); vertices.push_back(celda.r); vertices.push_back(celda.g); vertices.push_back(celda.b);
    vertices.push_back(x2); vertices.push_back(y2); vertices.push_back(celda.r); vertices.push_back(celda.g); vertices.push_back(celda.b);
    vertices.push_back(x1); vertices.push_back(y2); vertices.push_back(celda.r); vertices.push_back(celda.g); vertices.push_back(celda.b);
}

vector<float> crearGrilla()
{
    vector<float> vertices;
    float pasoX = pasoCeldaX();
    float pasoY = pasoCeldaY();
    float x0 = origenLienzoX();

    for (int i = 0; i <= GRID_SIZE; i++)
    {
        float x = x0 + i * pasoX;
        float y = -1.0f + i * pasoY;
        vertices.push_back(x); vertices.push_back(-1.0f);
        vertices.push_back(x); vertices.push_back(1.0f);
        vertices.push_back(x0); vertices.push_back(y);
        vertices.push_back(1.0f); vertices.push_back(y);
    }
    return vertices;
}

void dibujarMalla(const vector<float>& vertices, int componentesPorVertice, bool usarUniforme, float ur, float ug, float ub, unsigned int usarLoc, unsigned int colorLoc)
{
    if (vertices.empty()) return;

    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, componentesPorVertice * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    if (!usarUniforme)
    {
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, componentesPorVertice * sizeof(float), (void*)(2 * sizeof(float)));
        glEnableVertexAttribArray(1);
    }

    glUniform1i(usarLoc, usarUniforme);
    if (usarUniforme)
        glUniform3f(colorLoc, ur, ug, ub);

    glDrawArrays(usarUniforme ? GL_LINES : GL_TRIANGLES, 0, (GLsizei)(vertices.size() / componentesPorVertice));

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}

void construirPanel(vector<float>& quadsPanel, vector<float>& bordesDorados, vector<float>& divisores)
{
    // Fondo del panel izquierdo
    pushQuadPx(quadsPanel, 0.0f, 0.0f, (float)PANEL_W, (float)WINDOW_H, 0.08f, 0.11f, 0.16f);

    // Linea divisoria vertical entre panel y lienzo
    float divX = pxToNdcX((float)PANEL_W);
    divisores.push_back(divX); divisores.push_back(-1.0f);
    divisores.push_back(divX); divisores.push_back(1.0f);

    // Titulo COLORES
    pushTextoPx(quadsPanel, "COLORES", 16.0f, 34.0f, 2.5f, 0.90f, 0.92f, 0.96f, false);

    // 10 muestras de color (2 filas x 5 columnas)
    for (int i = 0; i < 10; i++)
    {
        RectPx r = rectColorSwatch(i);
        pushQuadPx(quadsPanel, r.x, r.y, r.w, r.h, paleta[i].r, paleta[i].g, paleta[i].b);

        if (i == colorSeleccionado)
        {
            for (int b = 0; b < 3; b++)
                pushBordePx(bordesDorados, r.x - b, r.y - b, r.w + 2 * b, r.h + 2 * b);
        }
    }

    // Titulo PINCEL
    pushTextoPx(quadsPanel, "PINCEL", 16.0f, 166.0f, 2.5f, 0.90f, 0.92f, 0.96f, false);

    struct BotonInfo { const char* etiqueta; float r, g, b; int toolId; bool esLimpiar; };
    BotonInfo botones[] = {
        { "LIBRE",     0.22f, 0.36f, 0.56f, PINCEL_LIBRE,     false },
        { "LINEA",     0.18f, 0.44f, 0.44f, PINCEL_LINEA,     false },
        { "CIRCULO",   0.36f, 0.24f, 0.48f, PINCEL_CIRCULO,   false },
        { "CUADRADO",  0.20f, 0.34f, 0.54f, PINCEL_CUADRADO,  false },
        { "TRIANGULO", 0.16f, 0.42f, 0.42f, PINCEL_TRIANGULO, false },
        { "RELLENAR",  0.34f, 0.22f, 0.46f, PINCEL_RELLENO,  false },
        { "BORRAR",    0.32f, 0.33f, 0.36f, PINCEL_GOMA,      false },
        { "LIMPIAR",   0.50f, 0.16f, 0.14f, -1,               true  }
    };

    for (int idx = 0; idx < numBotonesHerramienta(); idx++)
    {
        RectPx bRect = rectBotonHerramienta(idx);
        const BotonInfo& info = botones[idx];
        pushQuadPx(quadsPanel, bRect.x, bRect.y, bRect.w, bRect.h, info.r, info.g, info.b);

        float textoY = bRect.y + (bRect.h - 7.0f * 2.2f) * 0.5f;
        pushTextoPx(quadsPanel, info.etiqueta, bRect.x, textoY, 2.2f, 0.96f, 0.97f, 0.99f, true, bRect.w);

        if (!info.esLimpiar && herramienta == info.toolId)
        {
            for (int b = 0; b < 3; b++)
                pushBordePx(bordesDorados, bRect.x - b, bRect.y - b, bRect.w + 2 * b, bRect.h + 2 * b);
        }
    }

    // Seccion COLOR / PINTA
    pushTextoPx(quadsPanel, "COLOR", 16.0f, 646.0f, 2.5f, 0.90f, 0.92f, 0.96f, false);

    RectPx muestra = rectMuestraColorActual();
    pushQuadPx(quadsPanel, muestra.x, muestra.y, muestra.w, muestra.h, colorActualR, colorActualG, colorActualB);
    for (int b = 0; b < 3; b++)
        pushBordePx(bordesDorados, muestra.x - b, muestra.y - b, muestra.w + 2 * b, muestra.h + 2 * b);

    pushTextoPx(quadsPanel, "PINTA", muestra.x + muestra.w + 18.0f, muestra.y + 13.0f, 2.5f, 0.92f, 0.94f, 0.97f, false);
}

int main()
{
    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WINDOW_W, WINDOW_H, "Paint OpenGL", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;

    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_pos_callback);
    glfwSetKeyCallback(window, key_callback);

    unsigned int shaderProgram = crearPrograma();
    vector<float> verticesGrilla = crearGrilla();
    glViewport(0, 0, WINDOW_W, WINDOW_H);

    cout << "=== PAINT OPENGL ===" << endl;
    cout << "Teclas 1-9 y 0 : colores" << endl;
    cout << "F: Libre | L: Linea | O: Circulo | Q: Cuadrado | T: Triangulo | R: Rellenar | E: Borrar" << endl;
    cout << "C: Limpiar | ESC: Salir" << endl;

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        glViewport(0, 0, WINDOW_W, WINDOW_H);
        glClearColor(0.05f, 0.08f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(shaderProgram);

        int usarColorUniformLoc = glGetUniformLocation(shaderProgram, "usarColorUniform");
        int colorUniformLoc = glGetUniformLocation(shaderProgram, "colorUniform");

        vector<float> celdas;
        for (const Celda& celda : celdasPintadas) agregarCelda(celdas, celda);
        for (const Celda& celda : vistaPrevia) agregarCelda(celdas, celda);
        dibujarMalla(celdas, 5, false, 0, 0, 0, usarColorUniformLoc, colorUniformLoc);
        dibujarMalla(verticesGrilla, 2, true, 0.20f, 0.20f, 0.25f, usarColorUniformLoc, colorUniformLoc);

        vector<float> quadsPanel;
        vector<float> bordesDorados;
        vector<float> divisores;
        construirPanel(quadsPanel, bordesDorados, divisores);

        dibujarMalla(quadsPanel, 5, false, 0, 0, 0, usarColorUniformLoc, colorUniformLoc);
        dibujarMalla(divisores, 2, true, 0.22f, 0.26f, 0.34f, usarColorUniformLoc, colorUniformLoc);
        dibujarMalla(bordesDorados, 2, true, 1.00f, 0.86f, 0.25f, usarColorUniformLoc, colorUniformLoc);

        glfwSwapBuffers(window);
    }

    glDeleteProgram(shaderProgram);
    glfwTerminate();
    return 0;
}