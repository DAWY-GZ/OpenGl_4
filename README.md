# Paint en OpenGL (C++)

Aplicación de dibujo estilo **Paint** desarrollada en **C++** utilizando **OpenGL**, **GLFW** y **GLAD**. Cuenta con un lienzo en cuadrícula de **40x40 celdas** y una interfaz gráfica lateral construida íntegramente con primitivas de OpenGL, sin dependencias de librerías UI externas.

---

## Características y Herramientas

* **Pintar libre (`F`):** Pinta celdas de forma continua arrastrando el ratón sobre la cuadrícula.
* **Línea (`L`):** Traza líneas rectas utilizando el algoritmo de Bresenham.
* **Círculo (`O`):** Dibuja circunferencias mediante el algoritmo de punto medio de Bresenham.
* **Cuadrado (`Q`):** Genera cuadrados y rectángulos trazando sus lados con Bresenham.
* **Triángulo (`T`):** Dibuja triángulos conectando sus 3 vértices con Bresenham.
* **Rellenar (`R`):** Aplica relleno por inundación (*Flood Fill*) sobre una zona cerrada o del mismo color al hacer clic.
* **Borrar (`E`):** Elimina el color de celdas individuales.
* **Limpiar (`C`):** Restablece todo el lienzo en blanco.
* **Vista previa en tiempo real:** Muestra el contorno de la figura mientras mantienes presionado el clic antes de confirmar el trazo.

> **Nota técnica:** Todo el panel izquierdo (botones, paleta de colores y tipografía) está renderizado directamente mediante primitivas geométricas de OpenGL.

---

## Controles Rápidos

| Acción | Entrada |
| :--- | :--- |
| **Seleccionar herramienta / Pintar** | `Clic izquierdo` |
| **Cambiar entre los 10 colores** | Teclas `1` a `9` y `0` |
| **Seleccionar herramienta** | Teclas `F`, `L`, `O`, `Q`, `T`, `R`, `E` |
| **Limpiar todo el lienzo** | Tecla `C` |
| **Cerrar aplicación** | Tecla `ESC` |

---

##  Requisitos Previos

Para compilar y ejecutar este proyecto necesitas:

* Un compilador de C++ compatible (MSVC / Visual Studio o `g++`).
* **GLFW 3**
* **GLAD** (`glad.c` y sus archivos de cabecera `.h`).
* Drivers gráficos con soporte para **OpenGL**.

---

## Cómo Compilar

### En Visual Studio

1. Crea un **Proyecto vacío de C++**.
2. Añade al proyecto el archivo fuente principal (`.cpp`) y `glad.c`.
3. Abre las **Propiedades del proyecto** (asegúrate de estar en configuración **x64**):
   * En *C/C++ → General → Directorios de inclusión adicionales*, agrega las rutas de los headers de **GLFW** y **GLAD**.
   * En *Vinculador → General → Directorios de bibliotecas adicionales*, agrega la ruta de las librerías de **GLFW**.
   * En *Vinculador → Entrada → Dependencias adicionales*, añade `glfw3.lib` y `opengl32.lib`.
4. Compila y ejecuta en arquitectura **x64**.
