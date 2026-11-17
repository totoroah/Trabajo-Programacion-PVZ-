
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "raylib.h"
#include "logica.h"

int main(void) {
    srand((unsigned int)time(NULL));

    // 1. Cargar configuracion y jardin usando las funciones de Lucas
    ConfiguracionJuego config = {0};
    if (!leer_configuracion("configuracion.txt", &config)) {
        printf("Error al leer configuracion.txt\n");
        return 1;
    }

    int filas = 0, columnas = 0;
    int **mapa_inicial = leer_jardin("jardin.txt", &filas, &columnas);
    if (mapa_inicial == NULL) {
        printf("Error al leer jardin.txt\n");
        liberar_configuracion(&config);
        return 1;
    }

    // 2. Inicializar tu motor de juego
    EstadoJuego *juego = inicializar_juego(&config, mapa_inicial, filas, columnas);
    for (int i = 0; i < filas; i++) free(mapa_inicial[i]);
    free(mapa_inicial);

    if (juego == NULL) {
        liberar_configuracion(&config);
        return 1;
    }

    // 3. Configurar ventana de Raylib segun las dimensiones del jardin
    const int TAM_CELDA = 85;
    const int MARGEN_X = 60;
    const int MARGEN_Y = 110;
    const int ANCHO = MARGEN_X * 2 + columnas * TAM_CELDA;
    const int ALTO = MARGEN_Y + filas * TAM_CELDA + 50;

    InitWindow(ANCHO, ALTO, "Plantas vs Zombies - Prueba Integrada");
    SetTargetFPS(60);

    // Variables de control de juego en tiempo real
    Elemento planta_seleccionada = PLANTA_GIRASOL;
    float tiempo_transcurrido = 0.0f;
    float timer_spawn = 0.0f;
    int zombies_por_salir = config.gameplay.oleadas * 6; // 6 zombies por oleada

    // 4. Bucle principal grafico (60 FPS)
    while (!WindowShouldClose()) {
        float dt = GetFrameTime(); // Segundos exactos desde el ultimo frame

        // Seleccionar tipo de planta con teclas 1, 2, 3
        if (IsKeyPressed(KEY_ONE))   planta_seleccionada = PLANTA_GIRASOL;
        if (IsKeyPressed(KEY_TWO))   planta_seleccionada = PLANTA_LANZAGUISANTES;
        if (IsKeyPressed(KEY_THREE)) planta_seleccionada = PLANTA_NUEZ;

        if (juego->juego_terminado == 0) {
            tiempo_transcurrido += dt;

            // Detectar clic izquierdo del mouse sobre el tablero para plantar
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                Vector2 mouse = GetMousePosition();
                int col = (int)((mouse.x - MARGEN_X) / TAM_CELDA);
                int fil = (int)((mouse.y - MARGEN_Y) / TAM_CELDA);

                if (mouse.x >= MARGEN_X && mouse.y >= MARGEN_Y &&
                    fil >= 0 && fil < juego->filas && col >= 0 && col < juego->columnas) {
                    plantar(juego, fil, col, planta_seleccionada);
                }
            }

            // Actualizar plantas en tiempo real (soles automaticos, girasoles y disparos)
            actualizar_plantas(juego, dt);

            // Generar oleada de zombies tras pasar el tiempo_preparacion de configuracion.txt
            if (tiempo_transcurrido >= config.gameplay.tiempo_preparacion && zombies_por_salir > 0) {
                timer_spawn += dt;
                if (timer_spawn >= 6.0f) { // Aparece un zombie cada 6 segundos
                    int fila_random = rand() % juego->filas;
                    Elemento tipo_z = (rand() % 3 == 0) ? ZOMBIE_BANDERA : ZOMBIE_BASE;
                    generar_zombie(juego, fila_random, tipo_z);
                    zombies_por_salir--;
                    timer_spawn = 0.0f;
                }
            }

            // Mover y atacar zombies segun su velocidad real
            mover_y_atacar_zombies(juego, dt);

            // Verificar victoria o derrota
            verificar_estado_partida(juego, zombies_por_salir);
        }

        // 5. Dibujar interfaz y tablero
        BeginDrawing();
        ClearBackground((Color){ 34, 110, 46, 255 }); // Fondo verde pasto

        // Barra superior de estado
        DrawRectangle(0, 0, ANCHO, 95, (Color){ 45, 30, 20, 255 });
        DrawText(TextFormat("SOLES: %d", juego->soles), 20, 15, 24, GOLD);
        DrawText(TextFormat("CASA: %d", juego->resistencia_casa), 200, 15, 24, SKYBLUE);
        DrawText(TextFormat("TIEMPO: %.1fs", tiempo_transcurrido), 360, 15, 24, RAYWHITE);
        DrawText(TextFormat("ZOMBIES RESTANTES: %d", zombies_por_salir), 560, 15, 22, LIGHTGRAY);

        // Indicador de fase (Preparacion vs Oleada) y controles
        if (tiempo_transcurrido < config.gameplay.tiempo_preparacion) {
            float restante = config.gameplay.tiempo_preparacion - tiempo_transcurrido;
            DrawText(TextFormat("FASE DE PREPARACION: %.1fs para la oleada", restante), 20, 50, 18, YELLOW);
        } else {
            DrawText("OLEADA EN CURSO - Haz clic en el pasto para plantar", 20, 50, 18, ORANGE);
        }

        const char *nombre_sel = (planta_seleccionada == PLANTA_GIRASOL) ? "[1] Girasol (50)" :
                                 (planta_seleccionada == PLANTA_LANZAGUISANTES) ? "[2] Lanzaguisantes (100)" :
                                 "[3] Nuez (50)";
        DrawText(TextFormat("Seleccion: %s  (Teclas 1, 2, 3)", nombre_sel), 470, 50, 18, GREEN);

        // Dibujar grilla del jardin y sus elementos
        for (int i = 0; i < juego->filas; i++) {
            for (int j = 0; j < juego->columnas; j++) {
                int x = MARGEN_X + j * TAM_CELDA;
                int y = MARGEN_Y + i * TAM_CELDA;

                Color cesped = ((i + j) % 2 == 0) ? (Color){ 60, 155, 70, 255 } : (Color){ 50, 138, 60, 255 };
                DrawRectangle(x, y, TAM_CELDA, TAM_CELDA, cesped);
                DrawRectangleLines(x, y, TAM_CELDA, TAM_CELDA, DARKGREEN);

                Celda c = juego->jardin[i][j];
                int cx = x + TAM_CELDA / 2;
                int cy = y + TAM_CELDA / 2;

                if (c.tipo == PLANTA_GIRASOL) {
                    DrawCircle(cx, cy, 28, GOLD);
                    DrawCircle(cx, cy, 14, BROWN);
                    DrawText(TextFormat("%d", c.vida), x + 6, y + 6, 16, WHITE);
                } else if (c.tipo == PLANTA_LANZAGUISANTES) {
                    DrawCircle(cx, cy, 28, LIME);
                    DrawRectangle(cx + 10, cy - 8, 22, 16, DARKGREEN);
                    DrawText(TextFormat("%d", c.vida), x + 6, y + 6, 16, WHITE);
                } else if (c.tipo == PLANTA_NUEZ) {
                    DrawEllipse(cx, cy, 24, 32, (Color){ 165, 110, 60, 255 });
                    DrawText(TextFormat("%d", c.vida), x + 6, y + 6, 16, WHITE);
                } else if (c.tipo == ZOMBIE_BASE) {
                    DrawRectangle(cx - 20, cy - 30, 40, 60, (Color){ 110, 130, 110, 255 });
                    DrawText("Z", cx - 6, cy - 10, 22, BLACK);
                    DrawText(TextFormat("HP:%d", c.vida), x + 6, y + 6, 15, RED);
                } else if (c.tipo == ZOMBIE_BANDERA) {
                    DrawRectangle(cx - 20, cy - 30, 40, 60, (Color){ 180, 60, 60, 255 });
                    DrawRectangle(cx + 15, cy - 38, 16, 12, YELLOW); // Bandera
                    DrawText("B", cx - 6, cy - 10, 22, WHITE);
                    DrawText(TextFormat("HP:%d", c.vida), x + 6, y + 6, 15, YELLOW);
                }
            }
        }

        // Pantalla superpuesta al terminar la partida
        if (juego->juego_terminado == 1) {
            DrawRectangle(0, 0, ANCHO, ALTO, Fade(BLACK, 0.7f));
            DrawText("DERROTA: LOS ZOMBIES ENTRARON A LA CASA", ANCHO / 2 - 280, ALTO / 2 - 20, 26, RED);
        } else if (juego->juego_terminado == 2) {
            DrawRectangle(0, 0, ANCHO, ALTO, Fade(BLACK, 0.7f));
            DrawText("VICTORIA: SOBREVIVISTE A LA OLEADA", ANCHO / 2 - 240, ALTO / 2 - 20, 26, GREEN);
        }

        EndDrawing();
    }

    // 6. Cerrar ventana y liberar memoria
    CloseWindow();
    liberar_juego(juego);
    liberar_configuracion(&config);

    return 0;
}
