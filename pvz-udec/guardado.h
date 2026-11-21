#ifndef GUARDADO_H
#define GUARDADO_H

#include <stdbool.h>
#include "logica.h"
#include "oleadas.h"

// Ruta por defecto donde se almacena la partida guardada
#define ARCHIVO_GUARDADO "save/partida_guardada.txt"

// Verifica si existe un archivo de partida guardada en disco
bool existe_partida_guardada(const char *ruta);

// Guarda el estado completo de la matriz, soles, tiempo, cooldowns y oleadas
bool guardar_partida(const char *ruta,
                     const EstadoJuego *juego,
                     const GestorOleadas *oleadas,
                     float tiempo_transcurrido,
                     const float cooldowns[3]);

// Carga una partida guardada sobre la instancia de EstadoJuego y GestorOleadas
bool cargar_partida(const char *ruta,
                    EstadoJuego *juego,
                    GestorOleadas *oleadas,
                    float *tiempo_transcurrido,
                    float cooldowns[3]);

#endif