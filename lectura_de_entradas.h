
#ifndef LECTURA_DE_ENTRADAS_H
#define LECTURA_DE_ENTRADAS_H

typedef struct {
    int sol_inicial;
    int valor_sol;
    float sol_automatico_frecuencia;
    int tiempo_preparacion;
    int tiempo_total;
    int oleadas;
    int resistencia_casa;
} ConfigGameplay;

typedef struct {
    int vida;
    int danio;
    float velocidad;
    float frecuencia_ataque;
} StatsZombie;

typedef struct {
    int coste;
    int vida;
    int danio;
    float frecuencia_ataque;
    float frecuencia_sol;
    int rango;
} StatsPlanta;

typedef struct {
    char nombre[50];
    StatsZombie stats;
} Zombie;

typedef struct {
    char nombre[50];
    StatsPlanta stats;
} Planta;

typedef struct {
    ConfigGameplay gameplay;
    Zombie *zombies;
    int cantidad_zombies;
    Planta *plantas;
    int cantidad_plantas;
} ConfiguracionJuego;

void liberar_configuracion(ConfiguracionJuego *config);
int leer_configuracion(const char *nombre_archivo, ConfiguracionJuego *config);
int **leer_jardin(const char *nombre_archivo, int *f, int *c);

#endif
