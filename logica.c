
#include <stdlib.h>
#include <string.h>
#include "logica.h" // Incluimos logica.h (donde estan los structs, enum y declaraciones de funciones)

	// Esta funcion busca en configuracion.txt que estadisticas le corresponden a una planta al momento de plantarla
static StatsPlanta* obtener_stats_planta(ConfiguracionJuego *config, const char *nombre) {
    for (int i = 0; i < config->cantidad_plantas; i++) {
        if (strcmp(config->plantas[i].nombre, nombre) == 0) { // Recorre la lista de plantas hasta que encuentra la que buscaba
            return &config->plantas[i].stats;	// Retorna la direccion de sus estadisticas (costo,vida,daño)
        }
    }
    return NULL;
}

	// Esta funcion busca en configuracion.txt que estadisticas le corresponden al zombie al momento de ser generado
static StatsZombie* obtener_stats_zombie(ConfiguracionJuego *config, const char *nombre) {
    for (int i = 0; i < config->cantidad_zombies; i++) {
        if (strcmp(config->zombies[i].nombre, nombre) == 0) { // Recorre la lista de zombies hasta que encuentra al que buscaba
            return &config->zombies[i].stats;	// Retorna la direccion de sus estadisticas (vida,daño,velocidad)
        }
    }
    return NULL;
}

	// La funcion inicializar_juego construye la partida en la MD antes de que empiece el juego
EstadoJuego* inicializar_juego(ConfiguracionJuego *config, int **mapa_inicial, int filas, int columnas) {
    EstadoJuego *juego = (EstadoJuego*)malloc(sizeof(EstadoJuego));	// Pide MD del tamaño del struct EstadoJuego y guarda la direccion
    if (juego == NULL) return NULL;					// en el puntero *juego

	// Carga los datos iniciales de la partida sacados de configuracion.txt
    juego->config = config;
    juego->filas = filas;
    juego->columnas = columnas;						// Tamaño de la matriz, con cuantos soles se empieza, las vidas
    juego->soles = config->gameplay.sol_inicial;			// de la casa, y en que estado se encuentra la partida (0=jugando)
    juego->resistencia_casa = config->gameplay.resistencia_casa;
    juego->juego_terminado = 0;

    juego->jardin = (Celda**)malloc(filas * sizeof(Celda*));	// Filas de la matriz dinamica
    for (int i = 0; i < filas; i++) {
        juego->jardin[i] = (Celda*)malloc(columnas * sizeof(Celda));	// Columnas de la matriz dinamica
        for (int j = 0; j < columnas; j++) {
            juego->jardin[i][j].tipo = (Elemento)mapa_inicial[i][j];	// Copia los elementos de jardin.txt
            juego->jardin[i][j].vida = 0;
            juego->jardin[i][j].danio = 0;				// Como al principio todo esta vacio, las vidas y el daño
        }								// parten en 0
    }

    return juego;
}

void liberar_juego(EstadoJuego *juego) {	// Libera la MD, pedida anteriormente, desde adentro hacia afuera al terminar la partida
    if (juego == NULL) return;
    for (int i = 0; i < juego->filas; i++) {		// Libera las celdas de cada fila
        free(juego->jardin[i]);
    }
    free(juego->jardin);				// Libera las filas
    free(juego);					// Libera el struct EstadoJuego
}

	// Esta funcion es para plantar
int plantar(EstadoJuego *juego, int fila, int columna, Elemento planta) {
    if (fila < 0 || fila >= juego->filas || columna < 0 || columna >= juego->columnas) return 0; // Revisa que la casilla este dentro de
    if (juego->jardin[fila][columna].tipo != VACIO) return 0; // Revisa que la casilla		 // los limites de la matriz
							      // este vacia

    const char *nombre_buscado = NULL;
    if (planta == PLANTA_GIRASOL) nombre_buscado = "planta_girasol";			 // Transforma el enum a una cadena de texto
    else if (planta == PLANTA_LANZAGUISANTES) nombre_buscado = "planta_lanzaguisantes";	 // para poder encontrarlo en configuracion.txt
    else if (planta == PLANTA_NUEZ) nombre_buscado = "planta_nuez";
    else return 0;

    StatsPlanta *stats = obtener_stats_planta(juego->config, nombre_buscado);	// Como está traducido lo podemos buscar y obtener
    if (stats == NULL) return 0;						// sus estadisticas

    if (juego->soles < stats->coste) return 0;	// Si no suficientes soles no se planta

    juego->soles -= stats->coste;			// Se cobran los soles que cuestan lo que se planto
    juego->jardin[fila][columna].tipo = planta;		// Se planta en la casilla correspondiente
    juego->jardin[fila][columna].vida = stats->vida;	// Se actualiza la vida de esa casilla a la vida de la planta
    juego->jardin[fila][columna].danio = stats->danio;	// Lo mismo con el daño

    return 1;
}

   // Esta funcion es para que las plantas ejecuten sus funciones (generar soles y disparar a zombies) en la frecuencia correspondiente
void actualizar_plantas(EstadoJuego *juego, float delta_tiempo) {

	static float tiempo_sol_auto = 0.0f;	// Contador para los soles que se generan de forma automatica
	static float tiempo_girasol = 0.0f;	// Contador para los soles que producen los girasoles
	static float tiempo_disparo = 0.0f;	// Contador para los disparos que producen los lanzaguisantes

	tiempo_sol_auto += delta_tiempo;	// Se le suma a cada contador el tiempo que paso en el cronometro
	tiempo_girasol += delta_tiempo;
	tiempo_disparo += delta_tiempo;

	float frecuencia_auto = juego->config->gameplay.sol_automatico_frecuencia; 	// Obtiene la frecuencia que le corresponde
											// a los soles automaticos

	if (frecuencia_auto > 0.0f && tiempo_sol_auto >= frecuencia_auto) {   // Cuando el contador llega a la frecuencia correspondiente
		juego->soles += juego->config->gameplay.valor_sol;	      // se suman los soles a la "billetera"
		tiempo_sol_auto -= frecuencia_auto;			      // se resta al contador la frecuencia para que vuelva a 0
	}

  // Obtenemos las frecuencias que corresponden a los soles generados por girasol y a los disparos generados por lanzaguisantes
	StatsPlanta *stats_girasol = obtener_stats_planta(juego->config, "planta_girasol");
    	StatsPlanta *stats_lanza = obtener_stats_planta(juego->config, "planta_lanzaguisantes");

	float frecuencia_girasol = stats_girasol->frecuencia_sol;	// Guardamos en estas variables
	float frecuencia_disparo = stats_lanza->frecuencia_ataque;

	int generar_sol_este_turno = 0;			// Variable auxiliar que sirve para saber cuando generar sol
	if (tiempo_girasol >= frecuencia_girasol) {	// Cuando el contador llega a la frecuencia correspondiente la variable auxiliar
		generar_sol_este_turno = 1;		// cambia a 1, asi mas adelante para cada girasol si el auxiliar es 1 genera soles
		tiempo_girasol -= frecuencia_girasol;	// Despues se le resta al contador la frecuencia para que empiece de nuevo
	}

	int disparar_este_turno = 0;			// Es analogo a la logica de arriba
	if (tiempo_disparo >= frecuencia_disparo) {
		disparar_este_turno = 1;
        	tiempo_disparo -= frecuencia_disparo;
    	}

    for (int i = 0; i < juego->filas; i++) {
        for (int j = 0; j < juego->columnas; j++) {		// Recorremos toda la matriz y vemos que hay en cada casilla
            Elemento actual = juego->jardin[i][j].tipo;

            if (actual == PLANTA_GIRASOL) {				// Si es un girasol y es turno de generar soles
                if (generar_sol_este_turno) {				// (la variable auxiliar es 1) se generan los soles
                    juego->soles += juego->config->gameplay.valor_sol;	// correspondientes segun la configuracion
                }
            }
            else if (actual == PLANTA_LANZAGUISANTES && disparar_este_turno) {	// Si es un lanzaguisantes y es turno de disparar
                for (int k = j + 1; k < juego->columnas; k++) {			// busca zombies a la der. en su fila
                    Elemento objetivo = juego->jardin[i][k].tipo;
                    if (objetivo == ZOMBIE_BASE || objetivo == ZOMBIE_BANDERA) { // Si los encuentra entonces le resta a la vida del
                        juego->jardin[i][k].vida -= juego->jardin[i][j].danio;	 // del zombie el poder de ataque del lanzaguisantes
                        if (juego->jardin[i][k].vida <= 0) {
                            juego->jardin[i][k].tipo = VACIO;		// Si el zombie se queda sin vida se vacia la casilla y se
                            juego->jardin[i][k].vida = 0;		// devuelven sus estadisticas a 0
                            juego->jardin[i][k].danio = 0;
                        }
                        break;
                    }
                }
            }
        }
    }
}

     // Hace que los zombies avancen a la iz. a la velocidad correspondiente y que resten vidas a las plantas o la casa
void mover_y_atacar_zombies(EstadoJuego *juego, float delta_tiempo) {

	static float tiempo_base = 0.0f;	// Contador para zombie base
	static float tiempo_bandera = 0.0f;	// Contador para zombie bandera

	tiempo_base += delta_tiempo;		// Sumamos el tiempo del cronometro a los contadores
	tiempo_bandera += delta_tiempo;

	// Obtenemos las velocidades que corresponden a cada zombie
	StatsZombie *stats_base = obtener_stats_zombie(juego->config, "zombie_base");
	StatsZombie *stats_bandera = obtener_stats_zombie(juego->config, "zombie_bandera");

	float frecuencia_base = 1.0f / stats_base->velocidad;		// Guardamos en las variables la division de distancia(1 casilla)
	float frecuencia_bandera = 1.0f / stats_bandera->velocidad;	// dividida en las velocidades de cada zombie, resultando el
									// tiempo que debe pasar para que el zombie se mueva
	int actuar_base = 0;
	if (tiempo_base >= frecuencia_base) {
		actuar_base = 1;		// Misma logica de antes, cuando el contador llega a la frecuencia que corresponde
		tiempo_base -= frecuencia_base;	// el auxiliar marca 1 y luego el contador vuelve a 0
	}

	int actuar_bandera = 0;
	if (tiempo_bandera >= frecuencia_bandera) {
		actuar_bandera = 1;			// Analogo a la logica anterior
		tiempo_bandera -= frecuencia_bandera;
    	}

	if (!actuar_base && !actuar_bandera) return;	// Si a ningun zombie le toca actuar la funcion se detiene aqui

    for (int i = 0; i < juego->filas; i++) {			// Recorremos la matriz de iz. a der. ya que los zombies avanzan
        for (int j = 0; j < juego->columnas; j++) {		// hacia la iz. y no queremos contarlos mas de una vez
            Elemento actual = juego->jardin[i][j].tipo;

            if (actual == ZOMBIE_BASE || actual == ZOMBIE_BANDERA) {	// Se detecta si es un zombie

		if (actual == ZOMBIE_BASE && !actuar_base) continue;		// Si no es el turno de actuar (moverse o atacar) del
                if (actual == ZOMBIE_BANDERA && !actuar_bandera) continue;	// zombie actual saltamos a la siguiente casilla

                if (j == 0) {			// Si esta en la columna 0
                    juego->resistencia_casa--;	// Se le resta vida a la casa (si la vida era 1 entonces al primer zombie se pierde)
                    juego->jardin[i][j].tipo = VACIO;	// Se borra el zombie de la casilla en que esta porque entro a la casa
                    juego->jardin[i][j].vida = 0;	// Se eliminan las estadisticas de la casilla
                    juego->jardin[i][j].danio = 0;

                    if (juego->resistencia_casa <= 0) {		// Si las vidas de la casa se acabaron el juego termino
                        juego->juego_terminado = 1;
                        return;
                    }
                }
                else {		// Si el zombie no ha llegado a la primera columna
                    Elemento izquierda = juego->jardin[i][j - 1].tipo;	// Definimos una variable para lo que esta a la iz.

                    if (izquierda == VACIO) {				// Si a la iz. no hay nada, se copia la info. de la casilla
                        juego->jardin[i][j - 1] = juego->jardin[i][j];	// actual a la casilla de la iz.
                        juego->jardin[i][j].tipo = VACIO;		// Luego se borra el zombie y sus estadisticas de la casilla
                        juego->jardin[i][j].vida = 0;			// anterior
                        juego->jardin[i][j].danio = 0;
                    }
                    else if (izquierda == PLANTA_GIRASOL ||		// Si en cambio a la iz. hay una planta
                             izquierda == PLANTA_LANZAGUISANTES ||
                             izquierda == PLANTA_NUEZ) {
                        juego->jardin[i][j - 1].vida -= juego->jardin[i][j].danio; // Se le resta a la vida de la planta el daño del zombie
                        if (juego->jardin[i][j - 1].vida <= 0) {
                            juego->jardin[i][j - 1].tipo = VACIO; // Si la planta se queda sin vida se elimina esta y sus estadisticas
                            juego->jardin[i][j - 1].vida = 0;
                            juego->jardin[i][j - 1].danio = 0;
                        }
                    }
                }
            }
        }
    }
}

	// Esta funcion se encarga de generar los zombies
void generar_zombie(EstadoJuego *juego, int fila, Elemento tipo_zombie) {
    if (fila < 0 || fila >= juego->filas) return;				// Comprueba estar dentro de los limites de la matriz

    const char *nombre_buscado = NULL;
    if (tipo_zombie == ZOMBIE_BASE) nombre_buscado = "zombie_base";		// Traduce de enum a cadena de texto para poder
    else if (tipo_zombie == ZOMBIE_BANDERA) nombre_buscado = "zombie_bandera";	// encontrarlo en configuracion.txt
    else return;

    StatsZombie *stats = obtener_stats_zombie(juego->config, nombre_buscado);	// Obtiene las estadisticas desde configuracion.txt
    if (stats == NULL) return;

    int ultima_col = juego->columnas - 1;
    if (juego->jardin[fila][ultima_col].tipo == VACIO) {
        juego->jardin[fila][ultima_col].tipo = tipo_zombie;		// Genera un zombie en una casilla de la ultima columna
        juego->jardin[fila][ultima_col].vida = stats->vida;		// e importa sus estadisticas a la casilla
        juego->jardin[fila][ultima_col].danio = stats->danio;
    }
}

	// Esta funcion verifica si quedan zombies en el jardin
int quedan_zombies(EstadoJuego *juego) {
    for (int i = 0; i < juego->filas; i++) {
        for (int j = 0; j < juego->columnas; j++) {
            Elemento t = juego->jardin[i][j].tipo;
            if (t == ZOMBIE_BASE || t == ZOMBIE_BANDERA) return 1;	// Si quedan retorna 1 y si no 0
        }
    }
    return 0;
}

	// Esta funcion termina la partida si es que aun no se ha perdido
void verificar_estado_partida(EstadoJuego *juego, int zombies_por_salir) {
    if (juego->juego_terminado == 1) return;
    if (zombies_por_salir == 0 && !quedan_zombies(juego)) {	// Si no quedan mas oleadas de zombies y no hay zombies en el jardin
        juego->juego_terminado = 2;				// la partida se termina y se da por ganada
    }
}
