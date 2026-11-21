#ifndef VISUALIZACION_H
#define VISUALIZACION_H

#include "raylib.h"
#include "logica.h"

// Dimensiones base de la interfaz y el tablero virtual
#define TAMANO_CELDA 80
#define ALTO_HUD 96
#define ALTO_BARRA_INFERIOR 46
#define MAX_FILAS 20

// Estados de navegacion entre el menu principal y la partida
typedef enum {
    PANTALLA_MENU = 0,
    PANTALLA_JUGANDO = 1
} PantallaActual;

// Submenus modales dentro de la pantalla de inicio
typedef enum {
    SUBMENU_NINGUNO = 0,
    SUBMENU_OPCIONES = 1,
    SUBMENU_MANUAL = 2
} SubMenuActual;

// Agrupa todas las texturas cargadas desde la carpeta assets/
typedef struct {
    Texture2D pasto;
    Texture2D girasol;
    Texture2D lanzaguisantes;
    Texture2D nuez;
    Texture2D zombie;
    Texture2D zombieBandera;
    Texture2D sol;
    Texture2D pala;
} TexturasJuego;

// Temporizadores visuales para rayos de disparo y alertas de horda
typedef struct {
    float timer_disparo[MAX_FILAS];
    int col_objetivo[MAX_FILAS];
    float timer_alerta_horda;
    bool es_horda_final;
} EfectosVisuales;

// Carga y descarga de sprites
TexturasJuego cargar_texturas(void);
void liberar_texturas(TexturasJuego *tex);

// Gestion de pantalla completa y conversion de coordenadas del raton
void alternar_pantalla_completa(int ancho_virtual, int alto_virtual, bool *pantalla_completa);
Vector2 obtener_raton_virtual(int ancho_virtual, int alto_virtual);

// Actualizacion y renderizado del menu principal
int actualizar_menu_principal(int ancho_pantalla, int alto_pantalla, SubMenuActual *submenu, bool hay_guardado, bool *pantalla_completa);
void dibujar_menu_principal(TexturasJuego tex, int ancho_pantalla, int alto_pantalla, SubMenuActual submenu, bool hay_guardado, bool pantalla_completa);

// Actualizacion del mini-menu de pausa
int actualizar_menu_pausa(int ancho_pantalla, int alto_pantalla);

// Deteccion de clics en las cartas del HUD y en el boton Menu inferior
Elemento verificar_clic_cartas(Vector2 mouse, Elemento actual, bool *pala_activa);
bool verificar_clic_boton_menu(Vector2 mouse, int ancho_pantalla, int alto_jardin);

// Renderizado completo de la partida (HUD, jardin, entidades, rayos, progreso y modales)
void dibujar_juego(EstadoJuego *juego,
                   TexturasJuego tex,
                   EfectosVisuales efectos,
                   Elemento planta_seleccionada,
                   bool pala_activa,
                   bool pausado,
                   const float cooldowns[3],
                   const float cooldowns_max[3],
                   float tiempo_transcurrido,
                   float tiempo_prep_restante,
                   float progreso_nivel,
                   int total_oleadas,
                   int oleada_actual);

#endif