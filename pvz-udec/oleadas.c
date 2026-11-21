#include <stdlib.h>
#include "oleadas.h"

// Mezcla aleatoriamente un arreglo de indices de fila para evitar repeticiones seguidas
static void mezclar_filas(int *arr, int n) {
    for (int i = n - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int tmp = arr[i];
        arr[i] = arr[j];
        arr[j] = tmp;
    }
}

// Cuenta cuantos zombis siguen vivos actualmente dentro de la matriz del jardin
static int contar_zombies_vivos(EstadoJuego *juego) {
    int vivos = 0;
    for (int i = 0; i < juego->filas; i++) {
        for (int j = 0; j < juego->columnas; j++) {
            Elemento t = juego->jardin[i][j].tipo;
            if (t == ZOMBIE_BASE || t == ZOMBIE_BANDERA) {
                vivos++;
            }
        }
    }
    return vivos;
}

// Configura la cantidad de zombis normales, el tamano de la horda y la frecuencia segun la oleada
static void configurar_parametros_oleada(GestorOleadas *gestor, int idx_ola) {
    int base_horda = (gestor->filas_jardin < 5) ? gestor->filas_jardin : 5;

    if (idx_ola == 0) {
        gestor->normales_por_oleada = 4;
        gestor->tamano_horda = base_horda;
        gestor->intervalo_normal = 14.0f;
    } else {
        gestor->normales_por_oleada = 10 * idx_ola;
        gestor->tamano_horda = base_horda * (2 + idx_ola);
        if (gestor->tamano_horda > MAX_COLA_HORDA) gestor->tamano_horda = MAX_COLA_HORDA;

        gestor->intervalo_normal = 7.5f;
    }

    gestor->normales_restantes = gestor->normales_por_oleada;
}

// Calcula el total de zombis (normales + horda) que pertenecen a una oleada especifica
static int total_zombies_de_oleada(int idx_ola, int filas) {
    int base_horda = (filas < 5) ? filas : 5;
    if (idx_ola == 0) {
        return 4 + base_horda;
    }
    int normales = 10 * idx_ola;
    int horda = base_horda * (2 + idx_ola);
    if (horda > MAX_COLA_HORDA) horda = MAX_COLA_HORDA;
    return normales + horda;
}

// Inicializa el gestor de oleadas con los parametros de configuracion.txt
void inicializar_oleadas(GestorOleadas *gestor, ConfiguracionJuego *config, int filas) {
    *gestor = (GestorOleadas){ 0 };

    gestor->filas_jardin = filas;
    gestor->total_oleadas = (config->gameplay.oleadas > 0) ? config->gameplay.oleadas : 1;
    gestor->oleada_actual = 0;
    gestor->estado_oleada = 0;
    gestor->idx_bolsa = filas;

    configurar_parametros_oleada(gestor, 0);

    // Deja el timer listo para que el primer zombi aparezca al terminar la preparacion
    gestor->timer_spawn_normal = gestor->intervalo_normal;
}

// Suma todos los zombis que aun faltan por salir en la oleada actual y las siguientes
int calcular_zombies_restantes(const GestorOleadas *gestor) {
    if (gestor->oleada_actual >= gestor->total_oleadas) return 0;

    int zombies_ola_actual = 0;
    if (gestor->estado_oleada == 0) {
        zombies_ola_actual = gestor->normales_restantes + gestor->tamano_horda;
    } else if (gestor->estado_oleada == 1 || gestor->estado_oleada == 2) {
        zombies_ola_actual = gestor->tamano_horda;
    } else if (gestor->estado_oleada == 3) {
        zombies_ola_actual = gestor->pendientes_horda;
    }

    int zombies_futuros = 0;
    for (int w = gestor->oleada_actual + 1; w < gestor->total_oleadas; w++) {
        zombies_futuros += total_zombies_de_oleada(w, gestor->filas_jardin);
    }

    return zombies_ola_actual + zombies_futuros;
}

// Calcula un porcentaje entre 0.0 y 1.0 para dibujar la barra inferior de progreso y banderas
float calcular_progreso_nivel(const GestorOleadas *gestor, EstadoJuego *juego) {
    if (gestor->total_oleadas <= 0) return 0.0f;
    if (gestor->oleada_actual >= gestor->total_oleadas) return 1.0f;

    float progreso_ola = 0.0f;
    int vivos = contar_zombies_vivos(juego);

    // Fase de zombis normales (ocupa del 0% al 88% de cada tramo hasta llegar a la bandera)
    if (gestor->estado_oleada == 0 || gestor->estado_oleada == 1) {
        int normales_salidos = gestor->normales_por_oleada - gestor->normales_restantes;
        int normales_derrotados = normales_salidos - vivos;
        if (normales_derrotados < 0) normales_derrotados = 0;

        float avance_normal = ((float)normales_derrotados + (float)vivos * 0.30f) / (float)gestor->normales_por_oleada;
        if (avance_normal > 1.0f) avance_normal = 1.0f;
        progreso_ola = avance_normal * 0.88f;
    }
    // Pausa sobre la bandera mientras se muestra la alerta de horda
    else if (gestor->estado_oleada == 2) {
        progreso_ola = 0.88f;
    }
    // Fase de horda (completa el ultimo 12% del tramo al eliminar la horda)
    else {
        int horda_salidos = gestor->tamano_horda - gestor->pendientes_horda;
        int horda_derrotados = horda_salidos - vivos;
        if (horda_derrotados < 0) horda_derrotados = 0;

        float avance_horda = ((float)horda_derrotados + (float)vivos * 0.25f) / (float)gestor->tamano_horda;
        if (avance_horda > 1.0f) avance_horda = 1.0f;
        progreso_ola = 0.88f + avance_horda * 0.12f;
    }

    float progreso = ((float)gestor->oleada_actual + progreso_ola) / (float)gestor->total_oleadas;
    if (progreso < 0.0f) progreso = 0.0f;
    if (progreso > 1.0f) progreso = 1.0f;
    return progreso;
}

// Maquina de estados que controla la salida de zombis normales, alertas y hordas con abanderado
void actualizar_oleadas(GestorOleadas *gestor, EstadoJuego *juego, EfectosVisuales *efectos, float dt) {
    if (gestor->oleada_actual >= gestor->total_oleadas) return;

    int filas = juego->filas;
    int ultima_col = juego->columnas - 1;

    // Estado 0: genera zombis normales usando una bolsa de filas mezcladas
    if (gestor->estado_oleada == 0) {
        if (gestor->normales_restantes > 0) {
            gestor->timer_spawn_normal += dt;
            if (gestor->timer_spawn_normal >= gestor->intervalo_normal) {
                int cantidad_lote = (gestor->oleada_actual >= 1) ? 2 : 1;
                if (cantidad_lote > gestor->normales_restantes) {
                    cantidad_lote = gestor->normales_restantes;
                }

                int generados = 0;
                for (int c = 0; c < cantidad_lote; c++) {
                    if (gestor->idx_bolsa >= filas) {
                        for (int f = 0; f < filas; f++) gestor->bolsa_filas[f] = f;
                        mezclar_filas(gestor->bolsa_filas, filas);
                        gestor->idx_bolsa = 0;
                    }
                    int fila_elegida = gestor->bolsa_filas[gestor->idx_bolsa];
                    if (juego->jardin[fila_elegida][ultima_col].tipo == VACIO) {
                        generar_zombie(juego, fila_elegida, ZOMBIE_BASE);
                        gestor->normales_restantes--;
                        gestor->idx_bolsa++;
                        generados++;
                    }
                }

                if (generados > 0) {
                    gestor->timer_spawn_normal = 0.0f;
                }
            }
        } else {
            gestor->estado_oleada = 1;
        }
    }
    // Estado 1: espera a que no queden zombis vivos antes de lanzar el aviso de horda
    else if (gestor->estado_oleada == 1) {
        if (!quedan_zombies(juego)) {
            gestor->estado_oleada = 2;
            gestor->timer_espera_horda = 2.2f;
            efectos->timer_alerta_horda = 3.5f;
            efectos->es_horda_final = (gestor->total_oleadas > 1 &&
                                       gestor->oleada_actual == gestor->total_oleadas - 1);
        }
    }
    // Estado 2: muestra la alerta en pantalla y prepara la cola de la horda con el zombi bandera
    else if (gestor->estado_oleada == 2) {
        gestor->timer_espera_horda -= dt;
        if (gestor->timer_espera_horda <= 0.0f) {
            int filas_horda[MAX_FILAS];
            for (int f = 0; f < filas; f++) filas_horda[f] = f;
            mezclar_filas(filas_horda, filas);

            for (int k = 0; k < gestor->tamano_horda && gestor->pendientes_horda < MAX_COLA_HORDA; k++) {
                if (k > 0 && (k % filas) == 0) {
                    mezclar_filas(filas_horda, filas);
                }
                gestor->cola_tipo[gestor->pendientes_horda] = (k == 0) ? ZOMBIE_BANDERA : ZOMBIE_BASE;
                gestor->cola_fila[gestor->pendientes_horda] = filas_horda[k % filas];
                gestor->pendientes_horda++;
            }
            gestor->timer_cola_horda = 1.0f;
            gestor->estado_oleada = 3;
        }
    }
    // Estado 3: va sacando los zombis de la cola a medida que se desocupa la columna derecha
    else if (gestor->estado_oleada == 3) {
        if (gestor->pendientes_horda > 0) {
            gestor->timer_cola_horda += dt;
            if (gestor->timer_cola_horda >= 0.30f) {
                gestor->timer_cola_horda = 0.0f;
                int i = 0;
                while (i < gestor->pendientes_horda) {
                    int f_obj = gestor->cola_fila[i];
                    if (juego->jardin[f_obj][ultima_col].tipo == VACIO) {
                        generar_zombie(juego, f_obj, gestor->cola_tipo[i]);
                        for (int m = i; m < gestor->pendientes_horda - 1; m++) {
                            gestor->cola_tipo[m] = gestor->cola_tipo[m + 1];
                            gestor->cola_fila[m] = gestor->cola_fila[m + 1];
                        }
                        gestor->pendientes_horda--;
                    } else {
                        i++;
                    }
                }
            }
        } else {
            gestor->estado_oleada = 4;
        }
    }
    // Estado 4: espera a que el jugador elimine toda la horda para avanzar a la siguiente oleada
    else if (gestor->estado_oleada == 4) {
        if (!quedan_zombies(juego)) {
            gestor->oleada_actual++;
            if (gestor->oleada_actual < gestor->total_oleadas) {
                configurar_parametros_oleada(gestor, gestor->oleada_actual);
                gestor->estado_oleada = 0;
                gestor->timer_spawn_normal = gestor->intervalo_normal * 0.55f;
            }
        }
    }
}