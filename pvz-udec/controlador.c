#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "controlador.h"
#include "guardado.h"

// Tiempos de recarga base (en segundos) para girasol, lanzaguisantes y nuez
static const float COOLDOWNS_MAX[3] = { 7.5f, 7.5f, 20.0f };

// Devuelve el indice del arreglo de cooldowns segun el tipo de planta (0, 1 o 2)
static int obtener_indice_carta(Elemento planta) {
    if (planta == PLANTA_GIRASOL) return 0;
    if (planta == PLANTA_LANZAGUISANTES) return 1;
    if (planta == PLANTA_NUEZ) return 2;
    return -1;
}

// Verifica si el elemento de una casilla corresponde a una planta aliada
static bool es_planta(Elemento tipo) {
    return (tipo == PLANTA_GIRASOL || tipo == PLANTA_LANZAGUISANTES || tipo == PLANTA_NUEZ);
}

// Libera la memoria de la partida actual si existe y regresa al menu principal
static void cerrar_partida_actual(Aplicacion *app) {
    if (app->juego != NULL) {
        liberar_juego(app->juego);
        app->juego = NULL;
    }
    app->pausado = false;
    app->submenu = SUBMENU_NINGUNO;
    app->pantalla = PANTALLA_MENU;
}

// Lee el mapa base del jardin e inicializa una partida nueva desde cero
static bool iniciar_nueva_partida(Aplicacion *app) {
    cerrar_partida_actual(app);

    int filas = 0, columnas = 0;
    int **mapa_inicial = leer_jardin("config/jardin.txt", &filas, &columnas);
    if (mapa_inicial == NULL) return false;

    app->juego = inicializar_juego(&app->config, mapa_inicial, filas, columnas);
    for (int i = 0; i < filas; i++) free(mapa_inicial[i]);
    free(mapa_inicial);

    if (app->juego == NULL) return false;

    inicializar_oleadas(&app->oleadas, &app->config, filas);
    app->efectos = (EfectosVisuales){ 0 };
    app->planta_seleccionada = PLANTA_GIRASOL;
    app->pala_activa = false;
    app->pausado = false;
    app->cooldowns[0] = 0.0f;
    app->cooldowns[1] = 0.0f;
    app->cooldowns[2] = 0.0f;
    app->tiempo_transcurrido = 0.0f;
    return true;
}

// Maneja los atajos de teclado ([1]-[3], [X]) y los clics del jugador sobre cartas o celdas
static void procesar_entradas_partida(Aplicacion *app, Vector2 mouse) {
    if (IsKeyPressed(KEY_ONE) || IsKeyPressed(KEY_KP_1)) {
        app->planta_seleccionada = PLANTA_GIRASOL;
        app->pala_activa = false;
    }
    if (IsKeyPressed(KEY_TWO) || IsKeyPressed(KEY_KP_2)) {
        app->planta_seleccionada = PLANTA_LANZAGUISANTES;
        app->pala_activa = false;
    }
    if (IsKeyPressed(KEY_THREE) || IsKeyPressed(KEY_KP_3)) {
        app->planta_seleccionada = PLANTA_NUEZ;
        app->pala_activa = false;
    }
    if (IsKeyPressed(KEY_X)) {
        app->pala_activa = !app->pala_activa;
    }

    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) return;

    // Clic en la barra superior (cartas o pala) vs clic dentro de la matriz del jardin
    if (mouse.y < ALTO_HUD) {
        app->planta_seleccionada = verificar_clic_cartas(mouse, app->planta_seleccionada, &app->pala_activa);
    } else if (mouse.y >= ALTO_HUD && mouse.y < ALTO_HUD + app->alto_jardin) {
        int col = (int)(mouse.x / TAMANO_CELDA);
        int fila = (int)((mouse.y - ALTO_HUD) / TAMANO_CELDA);

        if (fila >= 0 && fila < app->filas && col >= 0 && col < app->columnas) {
            if (app->pala_activa) {
                if (es_planta(app->juego->jardin[fila][col].tipo)) {
                    app->juego->jardin[fila][col].tipo = VACIO;
                    app->juego->jardin[fila][col].vida = 0;
                    app->juego->jardin[fila][col].danio = 0;
                    app->pala_activa = false;
                }
            } else {
                int idx = obtener_indice_carta(app->planta_seleccionada);
                if (idx != -1 && app->cooldowns[idx] <= 0.0f) {
                    if (plantar(app->juego, fila, col, app->planta_seleccionada)) {
                        app->cooldowns[idx] = COOLDOWNS_MAX[idx];
                    }
                }
            }
        }
    }
}

// Actualiza temporizadores, disparos de plantas, avance de oleadas y estado de victoria/derrota
static void actualizar_simulacion_partida(Aplicacion *app, float dt) {
    for (int i = 0; i < app->filas && i < MAX_FILAS; i++) {
        if (app->efectos.timer_disparo[i] > 0.0f) app->efectos.timer_disparo[i] -= dt;
    }
    if (app->efectos.timer_alerta_horda > 0.0f) app->efectos.timer_alerta_horda -= dt;

    app->tiempo_transcurrido += dt;
    for (int c = 0; c < 3; c++) {
        if (app->cooldowns[c] > 0.0f) app->cooldowns[c] -= dt;
    }

    // Guarda la vida total de los zombis por fila antes de disparar para detectar impactos visuales
    int suma_hp_antes[MAX_FILAS] = { 0 };
    int primer_zombie_col[MAX_FILAS];
    for (int i = 0; i < app->filas && i < MAX_FILAS; i++) {
        primer_zombie_col[i] = -1;
        for (int j = 0; j < app->columnas; j++) {
            Elemento t = app->juego->jardin[i][j].tipo;
            if (t == ZOMBIE_BASE || t == ZOMBIE_BANDERA) {
                suma_hp_antes[i] += app->juego->jardin[i][j].vida;
                if (primer_zombie_col[i] == -1) primer_zombie_col[i] = j;
            }
        }
    }

    actualizar_plantas(app->juego, dt);

    // Si bajo la vida de los zombis en esa fila, activa el efecto visual del rayo en pantalla
    for (int i = 0; i < app->filas && i < MAX_FILAS; i++) {
        int suma_hp_despues = 0;
        for (int j = 0; j < app->columnas; j++) {
            Elemento t = app->juego->jardin[i][j].tipo;
            if (t == ZOMBIE_BASE || t == ZOMBIE_BANDERA) {
                suma_hp_despues += app->juego->jardin[i][j].vida;
            }
        }
        if (suma_hp_despues < suma_hp_antes[i] && primer_zombie_col[i] != -1) {
            app->efectos.timer_disparo[i] = 0.20f;
            app->efectos.col_objetivo[i] = primer_zombie_col[i];
        }
    }

    // Cuando termina el tiempo de preparacion empiezan a salir y avanzar los zombis
    if (app->tiempo_transcurrido >= (float)app->config.gameplay.tiempo_preparacion) {
        actualizar_oleadas(&app->oleadas, app->juego, &app->efectos, dt);
        int zombies_restantes = calcular_zombies_restantes(&app->oleadas);
        mover_y_atacar_zombies(app->juego, dt);
        verificar_estado_partida(app->juego, zombies_restantes);
    }
}

// Actualiza la logica y dibuja el menu principal en el lienzo virtual
static void actualizar_y_dibujar_menu(Aplicacion *app) {
    bool hay_guardado = existe_partida_guardada(ARCHIVO_GUARDADO);
    int accion = actualizar_menu_principal(app->ancho_pantalla, app->alto_pantalla,
                                           &app->submenu, hay_guardado, &app->pantalla_completa);

    if (accion == 1) {
        if (iniciar_nueva_partida(app)) app->pantalla = PANTALLA_JUGANDO;
    } else if (accion == 2) {
        if (iniciar_nueva_partida(app) &&
            cargar_partida(ARCHIVO_GUARDADO, app->juego, &app->oleadas, &app->tiempo_transcurrido, app->cooldowns)) {
            app->pantalla = PANTALLA_JUGANDO;
        } else {
            cerrar_partida_actual(app);
        }
    } else if (accion == -1) {
        app->salir_programa = true;
    }

    BeginTextureMode(app->lienzo);
    dibujar_menu_principal(app->texturas, app->ancho_pantalla, app->alto_pantalla,
                           app->submenu, hay_guardado, app->pantalla_completa);
    EndTextureMode();
}

// Gestiona el estado de pausa, fin de partida o juego activo y dibuja el tablero
static void actualizar_y_dibujar_partida(Aplicacion *app, float dt) {
    Vector2 mouse = obtener_raton_virtual(app->ancho_pantalla, app->alto_pantalla);

    if (app->juego->juego_terminado != 0) {
        if (IsKeyPressed(KEY_ESCAPE) ||
            (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
             verificar_clic_boton_menu(mouse, app->ancho_pantalla, app->alto_jardin))) {
            cerrar_partida_actual(app);
            return;
        }
    } else if (app->pausado) {
        int opcion_pausa = actualizar_menu_pausa(app->ancho_pantalla, app->alto_pantalla);
        if (opcion_pausa == 1) {
            app->pausado = false;
        } else if (opcion_pausa == 2) {
            guardar_partida(ARCHIVO_GUARDADO, app->juego, &app->oleadas, app->tiempo_transcurrido, app->cooldowns);
            cerrar_partida_actual(app);
            return;
        } else if (opcion_pausa == 3) {
            cerrar_partida_actual(app);
            return;
        }
    } else {
        if (IsKeyPressed(KEY_ESCAPE) ||
            (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
             verificar_clic_boton_menu(mouse, app->ancho_pantalla, app->alto_jardin))) {
            app->pausado = true;
        } else {
            procesar_entradas_partida(app, mouse);
            actualizar_simulacion_partida(app, dt);
        }
    }

    float prep_restante = (float)app->config.gameplay.tiempo_preparacion - app->tiempo_transcurrido;
    if (prep_restante < 0.0f) prep_restante = 0.0f;
    float progreso = (app->juego->juego_terminado == 2) ? 1.0f : calcular_progreso_nivel(&app->oleadas, app->juego);

    BeginTextureMode(app->lienzo);
    dibujar_juego(app->juego, app->texturas, app->efectos, app->planta_seleccionada,
                  app->pala_activa, app->pausado,
                  app->cooldowns, COOLDOWNS_MAX,
                  app->tiempo_transcurrido, prep_restante, progreso,
                  app->oleadas.total_oleadas, app->oleadas.oleada_actual);
    EndTextureMode();
}

// Dibuja el lienzo virtual centrado en la ventana respetando su proporcion original
static void volcar_lienzo_en_pantalla(Aplicacion *app) {
    float win_w = (float)GetScreenWidth();
    float win_h = (float)GetScreenHeight();
    float scale_x = win_w / (float)app->ancho_pantalla;
    float scale_y = win_h / (float)app->alto_pantalla;
    float scale = (scale_x < scale_y) ? scale_x : scale_y;

    float dest_w = (float)app->ancho_pantalla * scale;
    float dest_h = (float)app->alto_pantalla * scale;
    float dest_x = (win_w - dest_w) * 0.5f;
    float dest_y = (win_h - dest_h) * 0.5f;

    BeginDrawing();
    ClearBackground(BLACK);
    Rectangle src_rec = { 0.0f, 0.0f, (float)app->lienzo.texture.width, -(float)app->lienzo.texture.height };
    Rectangle dst_rec = { dest_x, dest_y, dest_w, dest_h };
    DrawTexturePro(app->lienzo.texture, src_rec, dst_rec, (Vector2){ 0.0f, 0.0f }, 0.0f, WHITE);
    EndDrawing();
}

// Lee los archivos de configuracion, crea la ventana y carga todas las texturas iniciales
bool inicializar_aplicacion(Aplicacion *app) {
    *app = (Aplicacion){ 0 };
    srand((unsigned int)time(NULL));

    if (!leer_configuracion("config/configuracion.txt", &app->config)) {
        printf("Error al leer config/configuracion.txt\n");
        return false;
    }

    int **mapa_check = leer_jardin("config/jardin.txt", &app->filas, &app->columnas);
    if (mapa_check == NULL) {
        printf("Error al leer config/jardin.txt\n");
        liberar_configuracion(&app->config);
        return false;
    }
    for (int i = 0; i < app->filas; i++) free(mapa_check[i]);
    free(mapa_check);

    app->ancho_pantalla = app->columnas * TAMANO_CELDA;
    app->alto_jardin = app->filas * TAMANO_CELDA;
    app->alto_pantalla = ALTO_HUD + app->alto_jardin + ALTO_BARRA_INFERIOR;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(app->ancho_pantalla, app->alto_pantalla, "Plantas vs Zombies - Proyecto Semestral");
    SetWindowMinSize(app->ancho_pantalla / 2, app->alto_pantalla / 2);
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);

    app->lienzo = LoadRenderTexture(app->ancho_pantalla, app->alto_pantalla);
    SetTextureFilter(app->lienzo.texture, TEXTURE_FILTER_BILINEAR);
    app->texturas = cargar_texturas();
    app->pantalla = PANTALLA_MENU;
    app->submenu = SUBMENU_NINGUNO;
    app->planta_seleccionada = PLANTA_GIRASOL;

    return true;
}

// Bucle principal a 60 FPS que alterna entre menu y partida hasta cerrar el juego
void ejecutar_bucle_principal(Aplicacion *app) {
    while (!WindowShouldClose() && !app->salir_programa) {
        float dt = GetFrameTime();

        if (IsKeyPressed(KEY_F11) ||
            ((IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT)) && IsKeyPressed(KEY_ENTER))) {
            alternar_pantalla_completa(app->ancho_pantalla, app->alto_pantalla, &app->pantalla_completa);
        }

        if (app->pantalla == PANTALLA_MENU) {
            actualizar_y_dibujar_menu(app);
        } else if (app->pantalla == PANTALLA_JUGANDO && app->juego != NULL) {
            actualizar_y_dibujar_partida(app, dt);
        }

        volcar_lienzo_en_pantalla(app);
    }
}

// Libera toda la memoria dinamica, texturas de GPU y cierra la ventana de raylib
void liberar_aplicacion(Aplicacion *app) {
    cerrar_partida_actual(app);
    UnloadRenderTexture(app->lienzo);
    liberar_texturas(&app->texturas);
    CloseWindow();
    liberar_configuracion(&app->config);
}