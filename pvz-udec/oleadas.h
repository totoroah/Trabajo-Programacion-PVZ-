#ifndef OLEADAS_H
#define OLEADAS_H

#include "logica.h"
#include "visualizacion.h"

#define MAX_COLA_HORDA 30

// Estructura que controla la generacion progresiva de zombis y hordas por oleada
typedef struct {
    int filas_jardin;
    int total_oleadas;
    int oleada_actual;
    int normales_por_oleada;
    int normales_restantes;
    int tamano_horda;
    int estado_oleada; // 0: normales, 1: esperando limpiar jardin, 2: alerta horda, 3: desplegando horda, 4: fin de ola

    float timer_espera_horda;

    Elemento cola_tipo[MAX_COLA_HORDA];
    int cola_fila[MAX_COLA_HORDA];
    int pendientes_horda;
    float timer_cola_horda;

    int bolsa_filas[MAX_FILAS];
    int idx_bolsa;

    float intervalo_normal;
    float timer_spawn_normal;
} GestorOleadas;

// Funciones de inicializacion, calculo de avance y actualizacion de oleadas
void inicializar_oleadas(GestorOleadas *gestor, ConfiguracionJuego *config, int filas);
int calcular_zombies_restantes(const GestorOleadas *gestor);
float calcular_progreso_nivel(const GestorOleadas *gestor, EstadoJuego *juego);
void actualizar_oleadas(GestorOleadas *gestor, EstadoJuego *juego, EfectosVisuales *efectos, float dt);

#endif