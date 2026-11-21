========================================================================
PLANTAS VS ZOMBIS - PROYECTO SEMESTRAL (C / RAYLIB)
========================================================================

Este archivo contiene las instrucciones necesarias para instalar la
libreria grafica raylib, compilar el codigo fuente y ejecutar el juego.

------------------------------------------------------------------------
1. ESTRUCTURA DEL PROYECTO
------------------------------------------------------------------------
Antes de compilar, asegurese de que la carpeta raiz del proyecto contenga
los siguientes archivos y subcarpetas:

  Carpetas requeridas:
  - assets/                  (Imagenes y sprites del juego)
  - config/                  (Archivos configuracion.txt y jardin.txt)
  - save/                    (Carpeta para las partidas guardadas)

  Codigo fuente (.c y .h):
  - main.c
  - controlador.c / controlador.h
  - guardado.c / guardado.h
  - lectura_de_entradas.c / lectura_de_entradas.h
  - logica.c / logica.h
  - oleadas.c / oleadas.h
  - visualizacion.c / visualizacion.h

------------------------------------------------------------------------
2. INSTALACION DE RAYLIB Y COMPILADOR (GCC)
------------------------------------------------------------------------

A) En Linux (Ubuntu / Debian / WSL):
   Abra una terminal y ejecute los siguientes comandos para instalar el
   compilador gcc, las dependencias de ventana y la libreria raylib:

   sudo apt update
   sudo apt install build-essential git cmake libasound2-dev libx11-dev libxrandr-dev libxi-dev libgl1-mesa-dev libglu1-mesa-dev libxcursor-dev libxinerama-dev libwayland-dev libxkbcommon-dev
   sudo apt install libraylib-dev

   (Nota: Si su distribucion no encuentra 'libraylib-dev' en apt, puede
   compilar raylib directamente desde su repositorio oficial con:
     git clone --depth 1 https://github.com/raysan5/raylib.git
     cd raylib/src && make PLATFORM=PLATFORM_DESKTOP
     sudo make install
   )

B) En Windows (usando MSYS2 / MinGW64):
   1. Descargue e instale MSYS2 desde https://www.msys2.org/
   2. Abra la terminal "MSYS2 MINGW64" e instale gcc y raylib con:
      pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-raylib

------------------------------------------------------------------------
3. COMPILACION DEL PROYECTO
------------------------------------------------------------------------
Abra la terminal, ubiquese dentro de la carpeta raiz del proyecto (donde
se encuentran todos los archivos .c) y ejecute el comando segun su sistema:

A) En Linux / WSL:
   gcc main.c controlador.c guardado.c lectura_de_entradas.c logica.c oleadas.c visualizacion.c -o pvz -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

B) En Windows (MSYS2 MinGW64):
   gcc main.c controlador.c guardado.c lectura_de_entradas.c logica.c oleadas.c visualizacion.c -o pvz.exe -lraylib -lopengl32 -lgdi32 -lwinmm

------------------------------------------------------------------------
4. EJECUCION DEL JUEGO
------------------------------------------------------------------------
IMPORTANTE: Ejecute el programa siempre desde la carpeta raiz del
proyecto para que pueda encontrar las carpetas "assets/", "config/" y
"save/".

- En Linux / WSL:
  ./pvz

- En Windows:
  ./pvz.exe   (o haciendo doble clic sobre pvz.exe en la carpeta)

------------------------------------------------------------------------
5. CONTROLES RAPIDOS DENTRO DEL JUEGO
------------------------------------------------------------------------
- Teclas [1], [2], [3] : Seleccionar carta (Girasol, Lanzaguisantes, Nuez)
- Tecla [X]            : Activar/desactivar la pala para quitar plantas
- Clic Izquierdo       : Plantar en una casilla vacia o quitar planta
- Tecla [F11]          : Alternar entre modo ventana y pantalla completa
- Tecla [ESC]          : Abrir menu de pausa (Continuar / Guardar / Salir)
========================================================================

Miembros del equipo:
- Lucas Carrasco
- Giuliana Andrea
- Joaquin Opazo