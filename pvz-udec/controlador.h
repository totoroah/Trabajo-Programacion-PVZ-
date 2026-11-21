#ifndef CONTROLADOR_H
#define CONTROLADOR_H

#include <stdbool.h>
#include "raylib.h"
#include "lectura_de_entradas.h"
#include "logica.h"
#include "visualizacion.h"
#include "oleadas.h"

// Estructura principal que guarda todo el estado de la ventana y la partida en curso
typedef struct {
    ConfiguracionJuego config;
    int filas;
    int columnas;
    int ancho_pantalla;
    int alto_jardin;
    int alto_pantalla;

    RenderTexture2D lienzo;
    TexturasJuego texturas;

    PantallaActual pantalla;
    SubMenuActual submenu;
    bool pantalla_completa;
    bool salir_programa;

    EstadoJuego *juego;
    GestorOleadas oleadas;
    EfectosVisuales efectos;

    Elemento planta_seleccionada;
    bool pala_activa;
    bool pausado;
    float cooldowns[3];
    float tiempo_transcurrido;
} Aplicacion;

// Funciones principales del ciclo de vida del programa llamadas desde main.c
bool inicializar_aplicacion(Aplicacion *app);
void ejecutar_bucle_principal(Aplicacion *app);
void liberar_aplicacion(Aplicacion *app);

#endif