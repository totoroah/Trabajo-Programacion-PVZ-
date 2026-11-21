
#ifndef LOGICA_H	// Si logica.h no está definido
#define LOGICA_H	// definelo			  // Es decir, evita que se lea mas de una vez y haya un error de redefinicion

#include "lectura_de_entradas.h"

typedef enum {			// Enum (Elemento) para poder asignar un nombre a cada numero que representa un elemento
    VACIO = 0,
    PLANTA_GIRASOL = 1,
    PLANTA_LANZAGUISANTES = 2,
    PLANTA_NUEZ = 3,
    ZOMBIE_BASE = 4,
    ZOMBIE_BANDERA = 5
} Elemento;

typedef struct {		// Struct (Celda) que guarda los tres datos que se necesitan de una celda
    Elemento tipo;
    int vida;			// Que elemento es, cuanta vida tiene y cuanto daño hace
    int danio;
} Celda;

typedef struct {		// Struct (Estadojuego) que guarda el estado del juego leido desde configuracion.txt
    Celda **jardin;
    int filas;			// Matriz dinamica que representa el jardin
    int columnas;
    int soles;			// Cantidad de soles disponibles
    int resistencia_casa;	// Vidas de la casa leidas de configuracion.txt (si es 1 pierdes al primer zombie que llega)
    int juego_terminado;	// Estado de la partida; jugando=0 ; perdida=1 ; ganada=2
    ConfiguracionJuego *config; // Puntero a configuracion.txt
} EstadoJuego;

// Declaracion de funciones

EstadoJuego* inicializar_juego(ConfiguracionJuego *config, int **mapa_inicial, int filas, int columnas);
	// Reserva la MD para el jardin y carga los valores iniciales de soles y resistencia de la casa

void liberar_juego(EstadoJuego *juego);
	// Libera la MD prestada

int plantar(EstadoJuego *juego, int fila, int columna, Elemento planta);
	// Intenta plantar en una coordena, devuelve 1 si se pudo y 0 si no hay suficientes soles o la casilla esta ocupada

void actualizar_plantas(EstadoJuego *juego, float delta_tiempo);
	// Ejecuta las acciones de las plantas (generar soles y disparar) en la frecuencia correspondiente

void mover_y_atacar_zombies(EstadoJuego *juego, float delta_tiempo);
	// Hace que los zombies avancen a la iz. a la velocidad correspondiente y que resten vidas a las plantas o la casa

void generar_zombie(EstadoJuego *juego, int fila, Elemento tipo_zombie);
	// Hace aparcer un zombie nuevo en la columna final

int quedan_zombies(EstadoJuego *juego);
	// Revisa toda la matriz y devuelve 1 se quedan zombies o 0 en el caso contrario

void verificar_estado_partida(EstadoJuego *juego, int zombies_por_salir);
	// Revisa si aparte de no haber zombies en el tablero, no quedan otros pendientes por salir

#endif
