#include <stdio.h>
#include "guardado.h"

// Intenta abrir el archivo en modo lectura para comprobar si existe una partida guardada
bool existe_partida_guardada(const char *ruta) {
    FILE *f = fopen(ruta, "r");
    if (f == NULL) return false;
    fclose(f);
    return true;
}

// Escribe en un archivo de texto los recursos, oleadas, cola de horda y cada celda del tablero
bool guardar_partida(const char *ruta,
                     const EstadoJuego *juego,
                     const GestorOleadas *oleadas,
                     float tiempo_transcurrido,
                     const float cooldowns[3]) {
    if (juego == NULL || oleadas == NULL) return false;

    FILE *f = fopen(ruta, "w");
    if (f == NULL) return false;

    // Cabecera, dimensiones del jardin, soles y tiempos de recarga
    fprintf(f, "PVZ_SAVE_V1\n");
    fprintf(f, "%d %d\n", juego->filas, juego->columnas);
    fprintf(f, "%d %d %d %.4f\n",
            juego->soles,
            juego->resistencia_casa,
            juego->juego_terminado,
            tiempo_transcurrido);
    fprintf(f, "%.4f %.4f %.4f\n", cooldowns[0], cooldowns[1], cooldowns[2]);

    // Contadores y temporizadores del gestor de oleadas
    fprintf(f, "%d %d %d %d %d %d %.4f %.4f %d %.4f %.4f %d\n",
            oleadas->total_oleadas,
            oleadas->oleada_actual,
            oleadas->estado_oleada,
            oleadas->normales_por_oleada,
            oleadas->normales_restantes,
            oleadas->tamano_horda,
            oleadas->timer_spawn_normal,
            oleadas->intervalo_normal,
            oleadas->pendientes_horda,
            oleadas->timer_cola_horda,
            oleadas->timer_espera_horda,
            oleadas->idx_bolsa);

    // Orden aleatorio de filas y zombies pendientes en la cola de horda
    for (int i = 0; i < MAX_FILAS; i++) {
        fprintf(f, "%d ", oleadas->bolsa_filas[i]);
    }
    fprintf(f, "\n");

    for (int i = 0; i < oleadas->pendientes_horda && i < MAX_COLA_HORDA; i++) {
        fprintf(f, "%d %d\n", (int)oleadas->cola_tipo[i], oleadas->cola_fila[i]);
    }

    // Estado celda por celda de la matriz del jardin (tipo, vida y danio)
    for (int fila = 0; fila < juego->filas; fila++) {
        for (int col = 0; col < juego->columnas; col++) {
            fprintf(f, "%d %d %d\n",
                    (int)juego->jardin[fila][col].tipo,
                    juego->jardin[fila][col].vida,
                    juego->jardin[fila][col].danio);
        }
    }

    fclose(f);
    return true;
}

// Lee el archivo de guardado en el mismo orden en que fue escrito y restaura la partida
bool cargar_partida(const char *ruta,
                    EstadoJuego *juego,
                    GestorOleadas *oleadas,
                    float *tiempo_transcurrido,
                    float cooldowns[3]) {
    if (juego == NULL || oleadas == NULL) return false;

    FILE *f = fopen(ruta, "r");
    if (f == NULL) return false;

    // Valida la cabecera y que el tamano de matriz coincida con el jardin actual
    char cabecera[32] = { 0 };
    if (fscanf(f, "%31s", cabecera) != 1) {
        fclose(f);
        return false;
    }

    int filas_archivo = 0, cols_archivo = 0;
    if (fscanf(f, "%d %d", &filas_archivo, &cols_archivo) != 2 ||
        filas_archivo != juego->filas || cols_archivo != juego->columnas) {
        fclose(f);
        return false;
    }

    if (fscanf(f, "%d %d %d %f",
               &juego->soles,
               &juego->resistencia_casa,
               &juego->juego_terminado,
               tiempo_transcurrido) != 4) {
        fclose(f);
        return false;
    }

    if (fscanf(f, "%f %f %f", &cooldowns[0], &cooldowns[1], &cooldowns[2]) != 3) {
        fclose(f);
        return false;
    }

    // Restaura el estado del gestor de oleadas y la cola de horda
    if (fscanf(f, "%d %d %d %d %d %d %f %f %d %f %f %d",
               &oleadas->total_oleadas,
               &oleadas->oleada_actual,
               &oleadas->estado_oleada,
               &oleadas->normales_por_oleada,
               &oleadas->normales_restantes,
               &oleadas->tamano_horda,
               &oleadas->timer_spawn_normal,
               &oleadas->intervalo_normal,
               &oleadas->pendientes_horda,
               &oleadas->timer_cola_horda,
               &oleadas->timer_espera_horda,
               &oleadas->idx_bolsa) != 12) {
        fclose(f);
        return false;
    }

    for (int i = 0; i < MAX_FILAS; i++) {
        if (fscanf(f, "%d", &oleadas->bolsa_filas[i]) != 1) {
            fclose(f);
            return false;
        }
    }

    for (int i = 0; i < oleadas->pendientes_horda && i < MAX_COLA_HORDA; i++) {
        int tipo_int = 0;
        if (fscanf(f, "%d %d", &tipo_int, &oleadas->cola_fila[i]) != 2) {
            fclose(f);
            return false;
        }
        oleadas->cola_tipo[i] = (Elemento)tipo_int;
    }

    // Restaura cada planta y zombi en su casilla correspondiente
    for (int fila = 0; fila < juego->filas; fila++) {
        for (int col = 0; col < juego->columnas; col++) {
            int tipo_int = 0;
            if (fscanf(f, "%d %d %d",
                       &tipo_int,
                       &juego->jardin[fila][col].vida,
                       &juego->jardin[fila][col].danio) != 3) {
                fclose(f);
                return false;
            }
            juego->jardin[fila][col].tipo = (Elemento)tipo_int;
        }
    }

    fclose(f);
    return true;
}