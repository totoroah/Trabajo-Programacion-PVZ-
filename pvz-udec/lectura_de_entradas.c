#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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


typedef enum {
    NINGUNA,
    GAMEPLAY,
    ZOMBIE,
    PLANTA
} Seccion;

void liberar_configuracion(ConfiguracionJuego *config) {
    free(config->zombies);
    free(config->plantas);

    config->zombies = NULL;
    config->plantas = NULL;

    config->cantidad_zombies = 0;
    config->cantidad_plantas = 0;
}

int agregar_zombie(ConfiguracionJuego *config, const char *nombre) {

    // Reservar espacio para un zombie adicional
    Zombie *temp = realloc(
        config->zombies,
        (config->cantidad_zombies + 1) * sizeof(Zombie)
    );

    // Comprobar si realloc tuvo éxito
    if (temp == NULL) {
        return 0;
    }

    // Actualizar el puntero
    config->zombies = temp;

    // Obtener la posición del nuevo zombie
    int i = config->cantidad_zombies;

    // Inicializar sus atributos en cero
    config->zombies[i] = (Zombie){0};

    // Guardar su nombre
    snprintf(config->zombies[i].nombre, 50, "%s", nombre);

    // Actualizar la cantidad
    config->cantidad_zombies++;

    return 1;
}

int agregar_planta(ConfiguracionJuego *config, const char *nombre) {

    // Reservar espacio para una planta adicional
    Planta *temp = realloc(
        config->plantas,
        (config->cantidad_plantas + 1) * sizeof(Planta)
    );

    // Comprobar si realloc tuvo éxito
    if (temp == NULL) {
        return 0;
    }

    // Actualizar el puntero
    config->plantas = temp;

    // Obtener la posición de la nueva planta
    int i = config->cantidad_plantas;

    // Inicializar sus atributos en cero
    config->plantas[i] = (Planta){0};

    // Guardar su nombre
    snprintf(config->plantas[i].nombre, 50, "%s", nombre);

    // Actualizar la cantidad
    config->cantidad_plantas++;

    return 1;
}

int leer_configuracion(const char *nombre_archivo,
                       ConfiguracionJuego *config) {

    FILE *archivo = fopen(nombre_archivo, "r");

    if (archivo == NULL) {
        return 0;
    }

    char linea[200];
    char nombre_seccion[50];
    char clave[50];
    float valor;

    Seccion seccion = NINGUNA;

    int indice_zombie = -1;
    int indice_planta = -1;

    while (fgets(linea, sizeof(linea), archivo) != NULL) {

        // Detectar una sección
        if (sscanf(linea, " [%49[^]]", nombre_seccion) == 1) {

            if (strcmp(nombre_seccion, "gameplay") == 0) {
                seccion = GAMEPLAY;
            }

            else if (strncmp(nombre_seccion, "zombie_", 7) == 0) {
                seccion = ZOMBIE;

                if (!agregar_zombie(config, nombre_seccion)) {
                    fclose(archivo);
                    return 0;
                }

                indice_zombie = config->cantidad_zombies - 1;
            }

            else if (strncmp(nombre_seccion, "planta_", 7) == 0) {
                seccion = PLANTA;

                if (!agregar_planta(config, nombre_seccion)) {
                    fclose(archivo);
                    return 0;
                }

                indice_planta = config->cantidad_plantas - 1;
            }

            else {
                seccion = NINGUNA;
            }

            continue;
        }

        // Detectar una variable y su valor
        if (sscanf(linea, " %49[^=] = %f", clave, &valor) != 2) {
            continue;
        }

        // Eliminar espacios sobrantes del nombre de la variable
        clave[strcspn(clave, " \t")] = '\0';

        // Guardar variables de gameplay
        if (seccion == GAMEPLAY) {

            if (strcmp(clave, "sol_inicial") == 0)
                config->gameplay.sol_inicial = (int)valor;

            else if (strcmp(clave, "valor_sol") == 0)
                config->gameplay.valor_sol = (int)valor;

            else if (strcmp(clave, "sol_automatico_frecuencia") == 0)
                config->gameplay.sol_automatico_frecuencia = valor;

            else if (strcmp(clave, "tiempo_preparacion") == 0)
                config->gameplay.tiempo_preparacion = (int)valor;

            else if (strcmp(clave, "tiempo_total") == 0)
                config->gameplay.tiempo_total = (int)valor;

            else if (strcmp(clave, "oleadas") == 0)
                config->gameplay.oleadas = (int)valor;

            else if (strcmp(clave, "resistencia_casa") == 0)
                config->gameplay.resistencia_casa = (int)valor;
        }

        // Guardar estadísticas de zombies
        else if (seccion == ZOMBIE && indice_zombie >= 0) {

            StatsZombie *stats =
                &config->zombies[indice_zombie].stats;

            if (strcmp(clave, "vida") == 0)
                stats->vida = (int)valor;

            else if (strcmp(clave, "danio") == 0)
                stats->danio = (int)valor;

            else if (strcmp(clave, "velocidad") == 0)
                stats->velocidad = valor;

            else if (strcmp(clave, "frecuencia_ataque") == 0)
                stats->frecuencia_ataque = valor;
        }

        // Guardar estadísticas de plantas
        else if (seccion == PLANTA && indice_planta >= 0) {

            StatsPlanta *stats =
                &config->plantas[indice_planta].stats;

            if (strcmp(clave, "coste") == 0)
                stats->coste = (int)valor;

            else if (strcmp(clave, "vida") == 0)
                stats->vida = (int)valor;

            else if (strcmp(clave, "danio") == 0)
                stats->danio = (int)valor;

            else if (strcmp(clave, "frecuencia_ataque") == 0)
                stats->frecuencia_ataque = valor;

            else if (strcmp(clave, "frecuencia_sol") == 0)
                stats->frecuencia_sol = valor;
            
            else if (strcmp(clave, "rango") == 0)
                stats->rango = valor;
        }
    }

    fclose(archivo);
    return 1;
}

int **leer_jardin(const char *nombre_archivo,int *f, int *c) {
    //abrir el archivo
	FILE *archivo = fopen(nombre_archivo, "r");
	
	//mensaje de error
    if (archivo == NULL) {
        printf("Error al abrir el archivo.\n");
        return NULL;
    }
	
	//lectura de dimensiones
    if (fscanf(archivo, "%d %d", f, c) != 2) {
        fclose(archivo);
        return NULL;
    }

    // validar dimensiones
    if (*f <= 0 || *c <= 0 || *f > 100 || *c > 100) {
        fclose(archivo);
        return NULL;
    }

    // reservar memoria para las filas
    int **jardin = malloc(*f * sizeof(int *));

    if (jardin == NULL) {
        fclose(archivo);
        return NULL;
    }

    // reservar cada fila
    
    for (int i = 0; i < *f; i++) {
        jardin[i] = malloc(*c * sizeof(int));

        if (jardin[i] == NULL) {
            // liberar filas que ya se reservaron
            for (int k = 0; k < i; k++) {
                free(jardin[k]);
            }

            free(jardin);
            fclose(archivo);
            return NULL;
        }
    }

    // leer elementos
    for (int i = 0; i < *f; i++) {
        for (int j = 0; j < *c; j++) {
            
			if (fscanf(archivo, "%d", &jardin[i][j]) != 1) {
                for (int k = 0; k < *f; k++) {
                    free(jardin[k]);
                }

                free(jardin);
                fclose(archivo);
                return NULL;
            }

            // elementos dentro de la matriz
            if (jardin[i][j] < 0 || jardin[i][j] > 5) {
                for (int k = 0; k < *f; k++) {
                    free(jardin[k]);
                }

                free(jardin);
                fclose(archivo);
                return NULL;
            }
        }
    }

    fclose(archivo);
    return jardin;
}


