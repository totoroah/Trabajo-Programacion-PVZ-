#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "visualizacion.h"

// Busca la vida maxima de una planta o zombi en la configuracion para calcular su barra de HP
static int obtener_vida_maxima(EstadoJuego *juego, Elemento tipo) {
    const char *nombre = NULL;
    if (tipo == PLANTA_GIRASOL) nombre = "planta_girasol";
    else if (tipo == PLANTA_LANZAGUISANTES) nombre = "planta_lanzaguisantes";
    else if (tipo == PLANTA_NUEZ) nombre = "planta_nuez";
    else if (tipo == ZOMBIE_BASE) nombre = "zombie_base";
    else if (tipo == ZOMBIE_BANDERA) nombre = "zombie_bandera";
    else return 1;

    for (int i = 0; i < juego->config->cantidad_plantas; i++) {
        if (strcmp(juego->config->plantas[i].nombre, nombre) == 0) {
            return juego->config->plantas[i].stats.vida;
        }
    }
    for (int i = 0; i < juego->config->cantidad_zombies; i++) {
        if (strcmp(juego->config->zombies[i].nombre, nombre) == 0) {
            return juego->config->zombies[i].stats.vida;
        }
    }
    return 1;
}

// Busca el coste en soles de una planta para dibujarlo en la esquina de su carta
static int obtener_coste_planta(EstadoJuego *juego, Elemento tipo) {
    const char *nombre = NULL;
    if (tipo == PLANTA_GIRASOL) nombre = "planta_girasol";
    else if (tipo == PLANTA_LANZAGUISANTES) nombre = "planta_lanzaguisantes";
    else if (tipo == PLANTA_NUEZ) nombre = "planta_nuez";
    else return 0;

    for (int i = 0; i < juego->config->cantidad_plantas; i++) {
        if (strcmp(juego->config->plantas[i].nombre, nombre) == 0) {
            return juego->config->plantas[i].stats.coste;
        }
    }
    return 0;
}

// Alterna entre modo ventana y pantalla completa sin bordes
void alternar_pantalla_completa(int ancho_virtual, int alto_virtual, bool *pantalla_completa) {
    ToggleBorderlessWindowed();
    *pantalla_completa = IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE);

    if (!(*pantalla_completa)) {
        SetWindowSize(ancho_virtual, alto_virtual);
        int monitor = GetCurrentMonitor();
        int mon_w = GetMonitorWidth(monitor);
        int mon_h = GetMonitorHeight(monitor);
        if (mon_w > ancho_virtual && mon_h > alto_virtual) {
            SetWindowPosition((mon_w - ancho_virtual) / 2, (mon_h - alto_virtual) / 2);
        }
    }
}

// Transforma las coordenadas reales del mouse en pantalla a las coordenadas del lienzo virtual
Vector2 obtener_raton_virtual(int ancho_virtual, int alto_virtual) {
    Vector2 mouse = GetMousePosition();
    float win_w = (float)GetScreenWidth();
    float win_h = (float)GetScreenHeight();

    if (win_w <= 0.0f || win_h <= 0.0f) return mouse;

    float scale_x = win_w / (float)ancho_virtual;
    float scale_y = win_h / (float)alto_virtual;
    float scale = (scale_x < scale_y) ? scale_x : scale_y;

    float offset_x = (win_w - (float)ancho_virtual * scale) * 0.5f;
    float offset_y = (win_h - (float)alto_virtual * scale) * 0.5f;

    Vector2 virtual_mouse = {
        (mouse.x - offset_x) / scale,
        (mouse.y - offset_y) / scale
    };

    if (virtual_mouse.x < 0.0f) virtual_mouse.x = 0.0f;
    if (virtual_mouse.y < 0.0f) virtual_mouse.y = 0.0f;
    if (virtual_mouse.x > (float)ancho_virtual) virtual_mouse.x = (float)ancho_virtual;
    if (virtual_mouse.y > (float)alto_virtual) virtual_mouse.y = (float)alto_virtual;

    return virtual_mouse;
}

// Recorta margenes transparentes, escala la imagen y genera mipmaps para suavizar bordes
static Texture2D cargar_sprite_suave(const char *ruta) {
    Image img = LoadImage(ruta);
    if (img.data == NULL) {
        return LoadTexture(ruta);
    }

    ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

    // Detecta el rectangulo minimo que encierra los pixeles visibles del sprite
    Color *px = (Color *)img.data;
    int min_x = img.width;
    int min_y = img.height;
    int max_x = -1;
    int max_y = -1;

    for (int y = 0; y < img.height; y++) {
        for (int x = 0; x < img.width; x++) {
            if (px[y * img.width + x].a > 10) {
                if (x < min_x) min_x = x;
                if (x > max_x) max_x = x;
                if (y < min_y) min_y = y;
                if (y > max_y) max_y = y;
            }
        }
    }

    if (max_x >= min_x && max_y >= min_y) {
        int x0 = (min_x >= 2) ? (min_x - 2) : 0;
        int y0 = (min_y >= 2) ? (min_y - 2) : 0;
        int x1 = (max_x + 2 < img.width) ? (max_x + 2) : (img.width - 1);
        int y1 = (max_y + 2 < img.height) ? (max_y + 2) : (img.height - 1);

        Rectangle recorte = {
            (float)x0,
            (float)y0,
            (float)(x1 - x0 + 1),
            (float)(y1 - y0 + 1)
        };
        ImageCrop(&img, recorte);
    }

    // Reduce imagenes muy grandes a un maximo de 256px manteniendo el detalle
    int max_dim = (img.width > img.height) ? img.width : img.height;
    if (max_dim > 256) {
        float factor = 256.0f / (float)max_dim;
        int nuevo_w = (int)(img.width * factor + 0.5f);
        int nuevo_h = (int)(img.height * factor + 0.5f);
        if (nuevo_w < 1) nuevo_w = 1;
        if (nuevo_h < 1) nuevo_h = 1;
        ImageResize(&img, nuevo_w, nuevo_h);
    }

    // Suaviza el canal alfa de los bordes para evitar dientes de sierra
    int w = img.width;
    int h = img.height;
    px = (Color *)img.data;
    unsigned char *alpha_orig = (unsigned char *)malloc((size_t)(w * h));
    if (alpha_orig != NULL) {
        for (int i = 0; i < w * h; i++) {
            alpha_orig[i] = px[i].a;
        }
        for (int y = 1; y < h - 1; y++) {
            for (int x = 1; x < w - 1; x++) {
                int idx = y * w + x;
                if (alpha_orig[idx] > 200) {
                    int trans_vecinos = 0;
                    if (alpha_orig[idx - 1] < 20) trans_vecinos++;
                    if (alpha_orig[idx + 1] < 20) trans_vecinos++;
                    if (alpha_orig[idx - w] < 20) trans_vecinos++;
                    if (alpha_orig[idx + w] < 20) trans_vecinos++;

                    if (trans_vecinos > 0) {
                        int nuevo_a = (int)alpha_orig[idx] - trans_vecinos * 28;
                        if (nuevo_a < 105) nuevo_a = 105;
                        px[idx].a = (unsigned char)nuevo_a;
                    }
                }
            }
        }
        free(alpha_orig);
    }

    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);

    if (tex.id > 0) {
        GenTextureMipmaps(&tex);
        SetTextureFilter(tex, TEXTURE_FILTER_TRILINEAR);
    }
    return tex;
}

// Dibuja una textura escalada proporcionalmente dentro de un rectangulo sin deformarla
static void dibujar_textura_proporcional(Texture2D tex, Rectangle area, bool alinear_abajo, float rotacion, Color tinte) {
    if (tex.id == 0 || tex.width <= 0 || tex.height <= 0) return;

    float escala_x = area.width / (float)tex.width;
    float escala_y = area.height / (float)tex.height;
    float escala = (escala_x < escala_y) ? escala_x : escala_y;

    float draw_w = (float)tex.width * escala;
    float draw_h = (float)tex.height * escala;

    float pos_x = area.x + area.width / 2.0f;
    float pos_y = alinear_abajo ? (area.y + area.height) : (area.y + area.height / 2.0f);

    Vector2 origin = alinear_abajo
                     ? (Vector2){ draw_w / 2.0f, draw_h }
                     : (Vector2){ draw_w / 2.0f, draw_h / 2.0f };

    Rectangle orig = { 0.0f, 0.0f, (float)tex.width, (float)tex.height };
    Rectangle dest = { pos_x, pos_y, draw_w, draw_h };

    DrawTexturePro(tex, orig, dest, origin, rotacion, tinte);
}

// Dibuja un boton estilo piedra con sombra y cambio de color al pasar el raton
static void dibujar_boton_lapida(Rectangle rect, const char *texto, int tam_fuente,
                                 Color base, Color hover_col, Color borde, Color texto_col, Vector2 mouse) {
    bool hover = CheckCollisionPointRec(mouse, rect);
    Color relleno = hover ? hover_col : base;

    DrawRectangle((int)rect.x + 2, (int)rect.y + 3, (int)rect.width, (int)rect.height, (Color){ 20, 22, 25, 195 });
    DrawRectangleRec(rect, relleno);
    DrawRectangleLinesEx(rect, 2.5f, hover ? GOLD : borde);

    int w_txt = MeasureText(texto, tam_fuente);
    int tx = (int)(rect.x + (rect.width - w_txt) / 2);
    int ty = (int)(rect.y + (rect.height - tam_fuente) / 2);
    DrawText(texto, tx + 2, ty + 2, tam_fuente, (Color){ 15, 15, 15, 220 });
    DrawText(texto, tx, ty, tam_fuente, hover ? WHITE : texto_col);
}

// Dibuja una carta de seleccion de planta en la barra superior con su coste y recarga
static void dibujar_carta_planta(Rectangle rect, Texture2D textura, const char *tecla,
                                 int coste, int soles_disponibles, bool seleccionada,
                                 float cd_restante, float cd_max) {
    bool en_recarga = (cd_restante > 0.0f);
    bool disponible = (!en_recarga && soles_disponibles >= coste);

    Color fondo = disponible ? (Color){ 216, 226, 186, 255 } : (Color){ 115, 120, 105, 255 };
    DrawRectangleRec(rect, fondo);

    if (seleccionada && !en_recarga) {
        DrawRectangleLinesEx(rect, 3.5f, (Color){ 255, 215, 0, 255 });
    } else {
        DrawRectangleLinesEx(rect, 2.0f, (Color){ 42, 26, 12, 255 });
    }

    DrawText(tecla, (int)rect.x + 5, (int)rect.y + 4, 13, (Color){ 50, 40, 30, 255 });

    Rectangle area_icono = {
        rect.x + (rect.width - 48.0f) / 2.0f,
        rect.y + 12.0f,
        48.0f,
        46.0f
    };
    Color tinte_img = disponible ? WHITE : (Color){ 140, 140, 140, 255 };
    dibujar_textura_proporcional(textura, area_icono, false, 0.0f, tinte_img);

    const char *txt_coste = TextFormat("%d", coste);
    int w_coste = MeasureText(txt_coste, 16);
    Color col_coste = disponible ? (Color){ 25, 20, 15, 255 } : (Color){ 165, 30, 30, 255 };
    DrawText(txt_coste, (int)(rect.x + rect.width - w_coste - 7), (int)(rect.y + rect.height - 19), 16, col_coste);

    // Cortina oscura de enfriamiento cuando la planta esta recargando
    if (en_recarga && cd_max > 0.0f) {
        float ratio = cd_restante / cd_max;
        if (ratio > 1.0f) ratio = 1.0f;
        DrawRectangle((int)rect.x, (int)rect.y, (int)rect.width, (int)(rect.height * ratio), (Color){ 0, 0, 0, 155 });

        const char *txt_cd = TextFormat("%.1fs", cd_restante);
        int w_cd = MeasureText(txt_cd, 15);
        DrawText(txt_cd, (int)(rect.x + (rect.width - w_cd) / 2), (int)(rect.y + rect.height / 2 - 7), 15, WHITE);
    }
}

// Dibuja la ranura de la pala [X] en la barra superior
static void dibujar_casilla_pala(Rectangle rect, Texture2D tex_pala, bool activa) {
    Color fondo = activa ? (Color){ 225, 165, 95, 255 } : (Color){ 105, 76, 50, 255 };
    DrawRectangleRec(rect, fondo);
    DrawRectangleLinesEx(rect, activa ? 3.5f : 2.0f, activa ? (Color){ 255, 215, 0, 255 } : (Color){ 42, 26, 12, 255 });

    Color col_tecla = activa ? BLACK : (Color){ 240, 225, 195, 255 };
    DrawText("[  ]", (int)rect.x + 5, (int)rect.y + 4, 13, col_tecla);
    DrawLineEx((Vector2){ rect.x + 11.0f, rect.y + 6.5f }, (Vector2){ rect.x + 18.0f, rect.y + 14.5f }, 1.8f, col_tecla);
    DrawLineEx((Vector2){ rect.x + 18.0f, rect.y + 6.5f }, (Vector2){ rect.x + 11.0f, rect.y + 14.5f }, 1.8f, col_tecla);

    if (tex_pala.id > 0) {
        Rectangle area_pala = {
            rect.x + 9.0f,
            rect.y + 18.0f,
            rect.width - 18.0f,
            rect.height - 24.0f
        };
        dibujar_textura_proporcional(tex_pala, area_pala, false, 0.0f, WHITE);
    }
}

// Dibuja la silueta de la lapida del menu principal con arco redondeado y base de tierra
static void dibujar_cuerpo_lapida(float x, float y_rect, float w, float h_rect, float h_arco) {
    Color piedra = (Color){ 88, 94, 100, 255 };
    Color piedra_sombra = (Color){ 68, 73, 78, 255 };
    Color borde = (Color){ 52, 56, 62, 255 };

    float cx = x + w / 2.0f;
    float a = w / 2.0f;
    float b = h_arco;
    const int segs = 36;

    // Sombra proyectada de la lapida
    for (int i = 0; i < segs; i++) {
        float t1 = PI + (PI * (float)i) / (float)segs;
        float t2 = PI + (PI * (float)(i + 1)) / (float)segs;
        Vector2 p0 = { cx + 5.0f, y_rect + 4.0f };
        Vector2 p1 = { cx + 5.0f + cosf(t1) * a, y_rect + 4.0f + sinf(t1) * b };
        Vector2 p2 = { cx + 5.0f + cosf(t2) * a, y_rect + 4.0f + sinf(t2) * b };
        DrawTriangle(p0, p2, p1, (Color){ 20, 25, 18, 120 });
    }
    DrawRectangle((int)x + 5, (int)y_rect + 4, (int)w, (int)h_rect, (Color){ 20, 25, 18, 120 });

    // Cuerpo principal y arco superior de piedra
    for (int i = 0; i < segs; i++) {
        float t1 = PI + (PI * (float)i) / (float)segs;
        float t2 = PI + (PI * (float)(i + 1)) / (float)segs;
        Vector2 p0 = { cx, y_rect };
        Vector2 p1 = { cx + cosf(t1) * a, y_rect + sinf(t1) * b };
        Vector2 p2 = { cx + cosf(t2) * a, y_rect + sinf(t2) * b };
        DrawTriangle(p0, p2, p1, piedra);
    }
    DrawRectangle((int)x, (int)y_rect, (int)w, (int)h_rect, piedra);

    DrawRectangle((int)(x + w - 9.0f), (int)y_rect, 9, (int)h_rect, piedra_sombra);

    for (int i = 0; i < segs; i++) {
        float t1 = PI + (PI * (float)i) / (float)segs;
        float t2 = PI + (PI * (float)(i + 1)) / (float)segs;
        Vector2 p1 = { cx + cosf(t1) * a, y_rect + sinf(t1) * b };
        Vector2 p2 = { cx + cosf(t2) * a, y_rect + sinf(t2) * b };
        DrawLineEx(p1, p2, 2.5f, borde);
    }
    DrawLineEx((Vector2){ x, y_rect }, (Vector2){ x, y_rect + h_rect }, 2.5f, borde);
    DrawLineEx((Vector2){ x + w, y_rect }, (Vector2){ x + w, y_rect + h_rect }, 2.5f, borde);
    DrawLineEx((Vector2){ x, y_rect + h_rect }, (Vector2){ x + w, y_rect + h_rect }, 2.5f, borde);

    DrawRectangle((int)x - 12, (int)(y_rect + h_rect - 4), (int)w + 24, 15, (Color){ 68, 44, 24, 255 });
    DrawRectangleLinesEx((Rectangle){ x - 12.0f, y_rect + h_rect - 4.0f, w + 24.0f, 15.0f }, 2.0f, (Color){ 42, 26, 12, 255 });
}

// Carga todas las texturas del juego desde la carpeta assets/
TexturasJuego cargar_texturas(void) {
    TexturasJuego tex;
    tex.pasto = LoadTexture("assets/pasto.png");
    tex.girasol = cargar_sprite_suave("assets/sunflower.png");
    tex.lanzaguisantes = cargar_sprite_suave("assets/peashooter.png");
    tex.nuez = cargar_sprite_suave("assets/nutt.png");
    tex.zombie = cargar_sprite_suave("assets/zomb.png");
    tex.zombieBandera = cargar_sprite_suave("assets/flag.png");
    tex.sol = cargar_sprite_suave("assets/sun.png");
    tex.pala = cargar_sprite_suave("assets/shovel.png");
    return tex;
}

// Descarga las texturas de memoria de video al cerrar el programa
void liberar_texturas(TexturasJuego *tex) {
    UnloadTexture(tex->pasto);
    UnloadTexture(tex->girasol);
    UnloadTexture(tex->lanzaguisantes);
    UnloadTexture(tex->nuez);
    UnloadTexture(tex->zombie);
    UnloadTexture(tex->zombieBandera);
    UnloadTexture(tex->sol);
    UnloadTexture(tex->pala);
}

// Detecta clics en los botones de la lapida y en los submenus de Opciones y Manual
int actualizar_menu_principal(int ancho_pantalla, int alto_pantalla, SubMenuActual *submenu, bool hay_guardado, bool *pantalla_completa) {
    Vector2 mouse = obtener_raton_virtual(ancho_pantalla, alto_pantalla);

    if (*submenu == SUBMENU_OPCIONES) {
        float panel_w = 500.0f;
        float panel_h = 395.0f;
        float panel_x = (ancho_pantalla - panel_w) / 2.0f;
        float panel_y = (alto_pantalla - panel_h) / 2.0f;

        Rectangle btn_pantalla = { panel_x + 65.0f, panel_y + 68.0f,  panel_w - 130.0f, 48.0f };
        Rectangle btn_manual   = { panel_x + 65.0f, panel_y + 128.0f, panel_w - 130.0f, 48.0f };
        Rectangle btn_volver   = { panel_x + 150.0f, panel_y + 330.0f, panel_w - 300.0f, 44.0f };

        if (IsKeyPressed(KEY_ESCAPE)) {
            *submenu = SUBMENU_NINGUNO;
            return 0;
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (CheckCollisionPointRec(mouse, btn_pantalla)) {
                if (pantalla_completa != NULL) {
                    alternar_pantalla_completa(ancho_pantalla, alto_pantalla, pantalla_completa);
                }
            } else if (CheckCollisionPointRec(mouse, btn_manual)) {
                *submenu = SUBMENU_MANUAL;
            } else if (CheckCollisionPointRec(mouse, btn_volver)) {
                *submenu = SUBMENU_NINGUNO;
            }
        }
        return 0;
    }

    if (*submenu == SUBMENU_MANUAL) {
        float panel_w = 590.0f;
        float panel_h = 415.0f;
        float panel_x = (ancho_pantalla - panel_w) / 2.0f;
        float panel_y = (alto_pantalla - panel_h) / 2.0f;

        Rectangle btn_volver = { panel_x + 175.0f, panel_y + 348.0f, panel_w - 350.0f, 44.0f };
        if (IsKeyPressed(KEY_ESCAPE) ||
            (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse, btn_volver))) {
            *submenu = SUBMENU_OPCIONES;
        }
        return 0;
    }

    float lapida_w = 264.0f;
    float lapida_x = (float)ancho_pantalla - lapida_w - 32.0f;
    float y_cuerpo = 228.0f;

    Rectangle btn_nueva     = { lapida_x + 22.0f, y_cuerpo + 4.0f,   lapida_w - 44.0f, 50.0f };
    Rectangle btn_continuar = { lapida_x + 22.0f, y_cuerpo + 64.0f,  lapida_w - 44.0f, 46.0f };
    Rectangle btn_opciones  = { lapida_x + 32.0f, y_cuerpo + 120.0f, lapida_w - 64.0f, 42.0f };
    Rectangle btn_salir     = { lapida_x + 32.0f, y_cuerpo + 172.0f, lapida_w - 64.0f, 42.0f };

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (CheckCollisionPointRec(mouse, btn_nueva)) return 1;
        if (hay_guardado && CheckCollisionPointRec(mouse, btn_continuar)) return 2;
        if (CheckCollisionPointRec(mouse, btn_opciones)) {
            *submenu = SUBMENU_OPCIONES;
            return 0;
        }
        if (CheckCollisionPointRec(mouse, btn_salir)) return -1;
    }

    return 0;
}

// Dibuja el escenario del menu principal (cielo, pradera, personajes, lapida y ventanas modales)
void dibujar_menu_principal(TexturasJuego tex, int ancho_pantalla, int alto_pantalla, SubMenuActual submenu, bool hay_guardado, bool pantalla_completa) {
    Vector2 mouse = obtener_raton_virtual(ancho_pantalla, alto_pantalla);
    Vector2 mouse_lapida = (submenu == SUBMENU_NINGUNO) ? mouse : (Vector2){ -1.0f, -1.0f };
    float t = (float)GetTime();

    // Cielo degradado y sol animado en la parte superior
    DrawRectangleGradientV(0, 0, ancho_pantalla, alto_pantalla / 2 + 15,
                           (Color){ 72, 142, 212, 255 }, (Color){ 165, 215, 248, 255 });

    if (tex.sol.id > 0) {
        dibujar_textura_proporcional(tex.sol,
                                     (Rectangle){ 385.0f, 22.0f, 84.0f, 84.0f },
                                     false, sinf(t * 0.8f) * 12.0f, WHITE);
    } else {
        DrawCircle(425, 62, 28, (Color){ 255, 235, 60, 255 });
    }

    // Capa de cesped de fondo (plano medio)
    int inicio_pasto_fondo = alto_pantalla / 2 - 14;
    int inicio_pasto_frente = alto_pantalla - 85;
    int celda_fondo = 64;

    for (int y = inicio_pasto_fondo; y < inicio_pasto_frente + 8; y += celda_fondo) {
        for (int x = 0; x < ancho_pantalla; x += celda_fondo) {
            int fila_idx = (y - inicio_pasto_fondo) / celda_fondo;
            int col_idx = x / celda_fondo;
            Color c = ((fila_idx + col_idx) % 2 == 0)
                      ? (Color){ 110, 195, 100, 255 }
                      : (Color){ 82, 160, 76, 255 };
            Rectangle orig = { 0.0f, 0.0f, (float)tex.pasto.width, (float)tex.pasto.height };
            Rectangle dest = { (float)x, (float)y, (float)celda_fondo, (float)celda_fondo };
            DrawTexturePro(tex.pasto, orig, dest, (Vector2){0.0f, 0.0f}, 0.0f, c);
        }
    }
    DrawRectangle(0, inicio_pasto_fondo, ancho_pantalla, 3, (Color){ 32, 82, 26, 255 });

    // Cartel de madera con el titulo del juego
    Rectangle cartel = { 28, 24, 345, 136 };
    int centro_cartel = (int)(cartel.x + cartel.width / 2.0f);

    DrawRectangle((int)cartel.x + 5, (int)cartel.y + 6, (int)cartel.width, (int)cartel.height, (Color){ 25, 15, 5, 170 });
    DrawRectangleRec(cartel, (Color){ 88, 52, 24, 255 });
    DrawRectangleLinesEx(cartel, 3.0f, (Color){ 45, 24, 10, 255 });

    const char *txt_p = "PLANTAS";
    int w_p = MeasureText(txt_p, 35);
    DrawText(txt_p, centro_cartel - w_p / 2 + 2, 43, 35, (Color){ 25, 45, 12, 220 });
    DrawText(txt_p, centro_cartel - w_p / 2, 41, 35, (Color){ 135, 235, 70, 255 });

    const char *txt_vs = "vs";
    int w_vs = MeasureText(txt_vs, 20);
    DrawRectangle(centro_cartel - w_vs / 2 - 42, 91, 32, 2, (Color){ 190, 145, 85, 255 });
    DrawText(txt_vs, centro_cartel - w_vs / 2, 82, 20, GOLD);
    DrawRectangle(centro_cartel + w_vs / 2 + 10, 91, 32, 2, (Color){ 190, 145, 85, 255 });

    const char *txt_z = "ZOMBIS";
    int w_z = MeasureText(txt_z, 37);
    DrawText(txt_z, centro_cartel - w_z / 2 + 2, 111, 37, (Color){ 25, 25, 25, 220 });
    DrawText(txt_z, centro_cartel - w_z / 2, 109, 37, (Color){ 225, 230, 220, 255 });

    // Sprites decorativos de plantas y zombis sobre la pradera
    dibujar_textura_proporcional(tex.girasol,
                                 (Rectangle){ 44.0f, (float)(inicio_pasto_fondo + 42), 68.0f, 70.0f },
                                 true, 0.0f, WHITE);

    dibujar_textura_proporcional(tex.lanzaguisantes,
                                 (Rectangle){ 130.0f, (float)(inicio_pasto_fondo + 58), 70.0f, 70.0f },
                                 true, 0.0f, WHITE);

    dibujar_textura_proporcional(tex.zombie,
                                 (Rectangle){ 238.0f, (float)(inicio_pasto_fondo + 26), 84.0f, 94.0f },
                                 true, 0.0f, WHITE);

    dibujar_textura_proporcional(tex.zombieBandera,
                                 (Rectangle){ 300.0f, (float)(inicio_pasto_fondo + 40), 118.0f, 100.0f },
                                 true, 0.0f, WHITE);

    // Franja inferior de cesped en primer plano
    int celda_frente = 112;
    for (int y = inicio_pasto_frente; y < alto_pantalla; y += celda_frente) {
        for (int x = 0; x < ancho_pantalla; x += celda_frente) {
            int fila_idx = (y - inicio_pasto_frente) / celda_frente;
            int col_idx = x / celda_frente;
            Color c = ((fila_idx + col_idx) % 2 == 0)
                      ? (Color){ 130, 225, 115, 255 }
                      : (Color){ 105, 195, 95, 255 };
            Rectangle orig = { 0.0f, 0.0f, (float)tex.pasto.width, (float)tex.pasto.height };
            Rectangle dest = { (float)x, (float)y, (float)celda_frente, (float)celda_frente };
            DrawTexturePro(tex.pasto, orig, dest, (Vector2){0.0f, 0.0f}, 0.0f, c);
        }
    }
    DrawRectangle(0, inicio_pasto_frente - 4, ancho_pantalla, 4, (Color){ 24, 64, 18, 220 });
    DrawRectangle(0, inicio_pasto_frente, ancho_pantalla, 3, (Color){ 65, 155, 48, 255 });

    // Lapida derecha con las opciones principales del menu
    float lapida_w = 264.0f;
    float lapida_x = (float)ancho_pantalla - lapida_w - 32.0f;
    float y_cuerpo = 228.0f;
    float h_cuerpo = 230.0f;
    float h_arco   = 44.0f;

    dibujar_cuerpo_lapida(lapida_x, y_cuerpo, lapida_w, h_cuerpo, h_arco);

    Rectangle btn_nueva     = { lapida_x + 22.0f, y_cuerpo + 4.0f,   lapida_w - 44.0f, 50.0f };
    Rectangle btn_continuar = { lapida_x + 22.0f, y_cuerpo + 64.0f,  lapida_w - 44.0f, 46.0f };
    Rectangle btn_opciones  = { lapida_x + 32.0f, y_cuerpo + 120.0f, lapida_w - 64.0f, 42.0f };
    Rectangle btn_salir     = { lapida_x + 32.0f, y_cuerpo + 172.0f, lapida_w - 64.0f, 42.0f };

    dibujar_boton_lapida(btn_nueva, "NUEVA PARTIDA", 20,
                         (Color){ 68, 130, 42, 255 }, (Color){ 88, 165, 52, 255 },
                         (Color){ 32, 65, 18, 255 }, (Color){ 235, 255, 210, 255 },
                         mouse_lapida);

    if (hay_guardado) {
        dibujar_boton_lapida(btn_continuar, "CONTINUAR", 20,
                             (Color){ 48, 92, 132, 255 }, (Color){ 65, 118, 168, 255 },
                             (Color){ 22, 46, 68, 255 }, (Color){ 220, 240, 255, 255 },
                             mouse_lapida);
    } else {
        DrawRectangle((int)btn_continuar.x + 2, (int)btn_continuar.y + 3, (int)btn_continuar.width, (int)btn_continuar.height, (Color){ 20, 22, 25, 150 });
        DrawRectangleRec(btn_continuar, (Color){ 68, 72, 76, 255 });
        DrawRectangleLinesEx(btn_continuar, 2.0f, (Color){ 48, 52, 56, 255 });
        const char *txt_cont = "CONTINUAR";
        int w_c = MeasureText(txt_cont, 19);
        DrawText(txt_cont, (int)(btn_continuar.x + (btn_continuar.width - w_c) / 2), (int)(btn_continuar.y + 13), 19, (Color){ 130, 135, 140, 255 });
    }

    dibujar_boton_lapida(btn_opciones, "OPCIONES", 19,
                         (Color){ 62, 68, 74, 255 }, (Color){ 82, 90, 98, 255 },
                         (Color){ 38, 42, 46, 255 }, LIGHTGRAY,
                         mouse_lapida);

    dibujar_boton_lapida(btn_salir, "SALIR", 19,
                         (Color){ 115, 45, 42, 255 }, (Color){ 148, 55, 50, 255 },
                         (Color){ 65, 22, 20, 255 }, (Color){ 255, 220, 220, 255 },
                         mouse_lapida);

    // Ventana modal de Opciones (pantalla completa, acceso al manual y lista de controles)
    if (submenu == SUBMENU_OPCIONES) {
        DrawRectangle(0, 0, ancho_pantalla, alto_pantalla, (Color){ 0, 0, 0, 180 });

        float panel_w = 500.0f;
        float panel_h = 395.0f;
        float panel_x = (ancho_pantalla - panel_w) / 2.0f;
        float panel_y = (alto_pantalla - panel_h) / 2.0f;

        Rectangle panel = { panel_x, panel_y, panel_w, panel_h };
        DrawRectangle((int)panel_x + 5, (int)panel_y + 6, (int)panel_w, (int)panel_h, (Color){ 15, 10, 5, 200 });
        DrawRectangleRec(panel, (Color){ 68, 44, 24, 255 });
        DrawRectangleLinesEx(panel, 3.0f, (Color){ 155, 112, 64, 255 });

        const char *tit_op = "OPCIONES";
        DrawText(tit_op, ancho_pantalla / 2 - MeasureText(tit_op, 28) / 2, (int)panel_y + 18, 28, GOLD);

        Rectangle btn_pantalla = { panel_x + 65.0f, panel_y + 68.0f, panel_w - 130.0f, 48.0f };
        const char *lbl_pantalla = pantalla_completa
                                   ? "PANTALLA: COMPLETA (F11)"
                                   : "PANTALLA: VENTANA (F11)";
        Color col_pan_base  = pantalla_completa ? (Color){ 150, 95, 32, 255 } : (Color){ 58, 108, 52, 255 };
        Color col_pan_hover = pantalla_completa ? (Color){ 185, 120, 42, 255 } : (Color){ 78, 140, 68, 255 };
        dibujar_boton_lapida(btn_pantalla, lbl_pantalla, 19,
                             col_pan_base, col_pan_hover,
                             (Color){ 32, 22, 12, 255 }, WHITE, mouse);

        Rectangle btn_manual = { panel_x + 65.0f, panel_y + 128.0f, panel_w - 130.0f, 48.0f };
        dibujar_boton_lapida(btn_manual, "MANUAL / COMO JUGAR", 19,
                             (Color){ 52, 92, 130, 255 }, (Color){ 68, 118, 165, 255 },
                             (Color){ 24, 46, 68, 255 }, WHITE, mouse);

        Rectangle caja_ctrl = { panel_x + 36.0f, panel_y + 190.0f, panel_w - 72.0f, 124.0f };
        DrawRectangleRec(caja_ctrl, (Color){ 48, 30, 16, 230 });
        DrawRectangleLinesEx(caja_ctrl, 2.0f, (Color){ 120, 86, 48, 255 });

        DrawText("[1] [2] [3] : Seleccionar carta de planta", (int)caja_ctrl.x + 16, (int)caja_ctrl.y + 10, 15, RAYWHITE);
        DrawText("[X] : Activar pala para quitar una planta", (int)caja_ctrl.x + 16, (int)caja_ctrl.y + 32, 15, (Color){ 255, 215, 120, 255 });
        DrawText("[F11] : Alternar pantalla completa / ventana", (int)caja_ctrl.x + 16, (int)caja_ctrl.y + 54, 15, (Color){ 155, 235, 135, 255 });
        DrawText("Clic Izquierdo : Plantar o quitar con pala", (int)caja_ctrl.x + 16, (int)caja_ctrl.y + 76, 15, RAYWHITE);
        DrawText("[ESC] : Abrir menu de pausa y guardado", (int)caja_ctrl.x + 16, (int)caja_ctrl.y + 98, 15, (Color){ 235, 210, 150, 255 });

        Rectangle btn_volver = { panel_x + 150.0f, panel_y + 330.0f, panel_w - 300.0f, 44.0f };
        dibujar_boton_lapida(btn_volver, "VOLVER", 20,
                             (Color){ 68, 130, 42, 255 }, (Color){ 88, 165, 52, 255 },
                             (Color){ 32, 65, 18, 255 }, WHITE, mouse);
    }

    // Ventana modal del Manual de Juego
    else if (submenu == SUBMENU_MANUAL) {
        DrawRectangle(0, 0, ancho_pantalla, alto_pantalla, (Color){ 0, 0, 0, 185 });

        float panel_w = 590.0f;
        float panel_h = 415.0f;
        float panel_x = (ancho_pantalla - panel_w) / 2.0f;
        float panel_y = (alto_pantalla - panel_h) / 2.0f;

        Rectangle panel = { panel_x, panel_y, panel_w, panel_h };
        DrawRectangle((int)panel_x + 5, (int)panel_y + 6, (int)panel_w, (int)panel_h, (Color){ 15, 10, 5, 210 });
        DrawRectangleRec(panel, (Color){ 56, 38, 22, 255 });
        DrawRectangleLinesEx(panel, 3.0f, (Color){ 155, 112, 64, 255 });

        const char *tit_man = "MANUAL DE JUEGO";
        DrawText(tit_man, ancho_pantalla / 2 - MeasureText(tit_man, 26) / 2, (int)panel_y + 18, 26, GOLD);
        DrawRectangle((int)panel_x + 25, (int)panel_y + 52, (int)panel_w - 50, 2, (Color){ 120, 86, 48, 255 });

        int ix = (int)panel_x + 30;
        int tx = (int)panel_x + 92;
        int y0 = (int)panel_y + 68;

        dibujar_textura_proporcional(tex.girasol, (Rectangle){(float)ix, (float)y0, 46, 46}, false, 0.0f, WHITE);
        DrawText("1. TU CASA Y EL JARDIN (MATRIZ)", tx, y0, 17, (Color){ 140, 235, 80, 255 });
        DrawText("Tu casa esta en el lado IZQUIERDO. Usa soles para plantar", tx, y0 + 21, 15, RAYWHITE);
        DrawText("defensas dentro de las casillas del jardin.", tx, y0 + 39, 15, LIGHTGRAY);

        int y1 = y0 + 68;
        dibujar_textura_proporcional(tex.zombie, (Rectangle){(float)ix, (float)y1, 46, 46}, false, 0.0f, WHITE);
        DrawText("2. INVASION Y OLEADAS PROGRESIVAS", tx, y1, 17, ORANGE);
        DrawText("Los zombis entran por la DERECHA. Cada bandera desata una", tx, y1 + 21, 15, RAYWHITE);
        DrawText("horda cada vez mas numerosa con un zombi abanderado.", tx, y1 + 39, 15, LIGHTGRAY);

        int y2 = y1 + 68;
        dibujar_textura_proporcional(tex.lanzaguisantes, (Rectangle){(float)ix, (float)y2, 46, 46}, false, 0.0f, WHITE);
        DrawText("3. VICTORIA Y DERROTA", tx, y2, 17, SKYBLUE);
        DrawText("- GANAS si derrotas a todos los zombis y sobrevives las oleadas.", tx, y2 + 21, 15, (Color){ 170, 255, 170, 255 });
        DrawText("- PIERDES si un solo zombi logra cruzar y llegar a tu casa.", tx, y2 + 39, 15, (Color){ 255, 165, 165, 255 });

        int y3 = y2 + 68;
        dibujar_textura_proporcional(tex.nuez, (Rectangle){(float)ix, (float)y3, 46, 46}, false, 0.0f, WHITE);
        DrawText("4. CONTROLES Y GUARDADO", tx, y3, 17, YELLOW);
        DrawText("Teclas [1], [2], [3] eligen planta, [X] activa la pala para quitar", tx, y3 + 21, 15, RAYWHITE);
        DrawText("plantas y [ESC] abre el mini-menu para guardar y salir.", tx, y3 + 39, 15, LIGHTGRAY);

        Rectangle btn_volver = { panel_x + 175.0f, panel_y + 348.0f, panel_w - 350.0f, 44.0f };
        dibujar_boton_lapida(btn_volver, "VOLVER", 20,
                             (Color){ 68, 130, 42, 255 }, (Color){ 88, 165, 52, 255 },
                             (Color){ 32, 65, 18, 255 }, WHITE, mouse);
    }
}

// Detecta clics en el mini-menu de pausa (1 = reanudar, 2 = guardar y salir, 3 = salir)
int actualizar_menu_pausa(int ancho_pantalla, int alto_pantalla) {
    Vector2 mouse = obtener_raton_virtual(ancho_pantalla, alto_pantalla);

    float panel_w = 430.0f;
    float panel_h = 285.0f;
    float panel_x = (ancho_pantalla - panel_w) / 2.0f;
    float panel_y = (alto_pantalla - panel_h) / 2.0f;

    Rectangle btn_reanudar = { panel_x + 45.0f, panel_y + 72.0f,  panel_w - 90.0f, 50.0f };
    Rectangle btn_guardar  = { panel_x + 45.0f, panel_y + 138.0f, panel_w - 90.0f, 50.0f };
    Rectangle btn_salir    = { panel_x + 45.0f, panel_y + 204.0f, panel_w - 90.0f, 50.0f };

    if (IsKeyPressed(KEY_ESCAPE)) {
        return 1;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (CheckCollisionPointRec(mouse, btn_reanudar)) return 1;
        if (CheckCollisionPointRec(mouse, btn_guardar))  return 2;
        if (CheckCollisionPointRec(mouse, btn_salir))    return 3;
    }

    return 0;
}

// Verifica si se hizo clic en alguna carta de planta o en la pala del HUD superior
Elemento verificar_clic_cartas(Vector2 mouse, Elemento actual, bool *pala_activa) {
    Rectangle carta_girasol = { 114, 8, 96, 80 };
    Rectangle carta_lanza   = { 222, 8, 96, 80 };
    Rectangle carta_nuez    = { 330, 8, 96, 80 };
    Rectangle casilla_pala  = { 442, 8, 66, 80 };

    if (CheckCollisionPointRec(mouse, carta_girasol)) {
        if (pala_activa) *pala_activa = false;
        return PLANTA_GIRASOL;
    }
    if (CheckCollisionPointRec(mouse, carta_lanza)) {
        if (pala_activa) *pala_activa = false;
        return PLANTA_LANZAGUISANTES;
    }
    if (CheckCollisionPointRec(mouse, carta_nuez)) {
        if (pala_activa) *pala_activa = false;
        return PLANTA_NUEZ;
    }
    if (CheckCollisionPointRec(mouse, casilla_pala)) {
        if (pala_activa) *pala_activa = !(*pala_activa);
        return actual;
    }
    return actual;
}

// Verifica si el jugador presiono el boton "Menu" en la esquina inferior derecha
bool verificar_clic_boton_menu(Vector2 mouse, int ancho_pantalla, int alto_jardin) {
    int y_inf = ALTO_HUD + alto_jardin;
    Rectangle btn_menu = { (float)(ancho_pantalla - 86), (float)(y_inf + 9), 74, 28 };
    return CheckCollisionPointRec(mouse, btn_menu);
}

// Dibuja la pantalla completa de partida activa (HUD, tablero, barras de vida, rayos y progreso)
void dibujar_juego(EstadoJuego *juego,
                   TexturasJuego tex,
                   EfectosVisuales efectos,
                   Elemento planta_seleccionada,
                   bool pala_activa,
                   bool pausado,
                   const float cooldowns[3],
                   const float cooldowns_max[3],
                   float tiempo_transcurrido,
                   float tiempo_prep_restante,
                   float progreso_nivel,
                   int total_oleadas,
                   int oleada_actual) {
    int ancho_pantalla = juego->columnas * TAMANO_CELDA;
    int alto_jardin = juego->filas * TAMANO_CELDA;
    int alto_pantalla = ALTO_HUD + alto_jardin + ALTO_BARRA_INFERIOR;

    ClearBackground((Color){ 45, 30, 15, 255 });

    // Barra superior con contador de soles, cartas [1]-[3] y pala [X]
    DrawRectangle(0, 0, ancho_pantalla, ALTO_HUD, (Color){ 64, 40, 20, 255 });
    DrawRectangle(0, ALTO_HUD - 3, ancho_pantalla, 3, (Color){ 32, 18, 8, 255 });

    Rectangle caja_sol = { 12, 8, 88, 80 };
    DrawRectangleRec(caja_sol, (Color){ 42, 25, 12, 255 });
    DrawRectangleLinesEx(caja_sol, 2.0f, (Color){ 115, 78, 42, 255 });

    if (tex.sol.id > 0) {
        dibujar_textura_proporcional(tex.sol, (Rectangle){ 34.0f, 12.0f, 44.0f, 44.0f }, false, 0.0f, WHITE);
    } else {
        int sol_cx = 56, sol_cy = 34;
        DrawCircle(sol_cx, sol_cy, 17, (Color){ 255, 175, 0, 255 });
        DrawCircle(sol_cx, sol_cy, 13, (Color){ 255, 235, 60, 255 });
    }

    DrawRectangle(20, 58, 72, 23, (Color){ 245, 238, 210, 255 });
    const char *txt_soles = TextFormat("%d", juego->soles);
    int w_soles = MeasureText(txt_soles, 18);
    DrawText(txt_soles, 56 - w_soles / 2, 61, 18, (Color){ 30, 20, 10, 255 });

    dibujar_carta_planta((Rectangle){ 114, 8, 96, 80 }, tex.girasol, "[1]",
                         obtener_coste_planta(juego, PLANTA_GIRASOL),
                         juego->soles, !pala_activa && planta_seleccionada == PLANTA_GIRASOL,
                         cooldowns[0], cooldowns_max[0]);

    dibujar_carta_planta((Rectangle){ 222, 8, 96, 80 }, tex.lanzaguisantes, "[2]",
                         obtener_coste_planta(juego, PLANTA_LANZAGUISANTES),
                         juego->soles, !pala_activa && planta_seleccionada == PLANTA_LANZAGUISANTES,
                         cooldowns[1], cooldowns_max[1]);

    dibujar_carta_planta((Rectangle){ 330, 8, 96, 80 }, tex.nuez, "[3]",
                         obtener_coste_planta(juego, PLANTA_NUEZ),
                         juego->soles, !pala_activa && planta_seleccionada == PLANTA_NUEZ,
                         cooldowns[2], cooldowns_max[2]);

    dibujar_casilla_pala((Rectangle){ 442, 8, 66, 80 }, tex.pala, pala_activa);

    // Tablero de cesped y sprites de plantas/zombis por casilla
    Vector2 mouse = obtener_raton_virtual(ancho_pantalla, alto_pantalla);
    int col_hover = (int)(mouse.x / TAMANO_CELDA);
    int fila_hover = (int)((mouse.y - ALTO_HUD) / TAMANO_CELDA);

    for (int fila = 0; fila < juego->filas; fila++) {
        for (int col = 0; col < juego->columnas; col++) {
            int x = col * TAMANO_CELDA;
            int y = fila * TAMANO_CELDA + ALTO_HUD;

            Color colorTinte = ((fila + col) % 2 == 0)
                               ? (Color){ 125, 215, 110, 255 }
                               : (Color){ 90, 175, 80, 255 };
            Rectangle origenPasto = { 0.0f, 0.0f, (float)tex.pasto.width, (float)tex.pasto.height };
            Rectangle destinoPasto = { (float)x, (float)y, (float)TAMANO_CELDA, (float)TAMANO_CELDA };
            DrawTexturePro(tex.pasto, origenPasto, destinoPasto, (Vector2){0.0f, 0.0f}, 0.0f, colorTinte);

            Elemento tipo = juego->jardin[fila][col].tipo;

            // Resalta la celda bajo el cursor al plantar o usar la pala
            if (!pausado && mouse.y >= ALTO_HUD && mouse.y < ALTO_HUD + alto_jardin &&
                fila == fila_hover && col == col_hover) {
                if (pala_activa && (tipo == PLANTA_GIRASOL || tipo == PLANTA_LANZAGUISANTES || tipo == PLANTA_NUEZ)) {
                    DrawRectangle(x, y, TAMANO_CELDA, TAMANO_CELDA, (Color){ 235, 65, 55, 85 });
                } else if (!pala_activa && tipo == VACIO) {
                    DrawRectangle(x, y, TAMANO_CELDA, TAMANO_CELDA, (Color){ 255, 255, 255, 60 });
                }
            }

            DrawRectangleLines(x, y, TAMANO_CELDA, TAMANO_CELDA, BLACK);

            if (tipo != VACIO) {
                Texture2D texturaActual = { 0 };
                Rectangle area_sprite = { 0 };

                switch (tipo) {
                    case PLANTA_GIRASOL:
                        texturaActual = tex.girasol;
                        area_sprite = (Rectangle){ (float)(x + 9), (float)(y + 15), 62.0f, 62.0f };
                        break;
                    case PLANTA_LANZAGUISANTES:
                        texturaActual = tex.lanzaguisantes;
                        area_sprite = (Rectangle){ (float)(x + 9), (float)(y + 15), 62.0f, 62.0f };
                        break;
                    case PLANTA_NUEZ:
                        texturaActual = tex.nuez;
                        area_sprite = (Rectangle){ (float)(x + 13), (float)(y + 23), 54.0f, 54.0f };
                        break;
                    case ZOMBIE_BASE:
                        texturaActual = tex.zombie;
                        area_sprite = (Rectangle){ (float)(x + 1), (float)(y + 2), 78.0f, 75.0f };
                        break;
                    case ZOMBIE_BANDERA:
                        texturaActual = tex.zombieBandera;
                        area_sprite = (Rectangle){ (float)(x - 14), (float)(y - 3), 98.0f, 80.0f };
                        break;
                    default:
                        break;
                }

                dibujar_textura_proporcional(texturaActual, area_sprite, true, 0.0f, WHITE);
            }
        }
    }

    // Barras de vida superiores e indicadores numericos de HP por entidad
    for (int fila = 0; fila < juego->filas; fila++) {
        for (int col = 0; col < juego->columnas; col++) {
            Elemento tipo = juego->jardin[fila][col].tipo;
            int vida_actual = juego->jardin[fila][col].vida;
            if (tipo != VACIO && vida_actual > 0) {
                int x = col * TAMANO_CELDA;
                int y = fila * TAMANO_CELDA + ALTO_HUD;
                bool es_zombie = (tipo == ZOMBIE_BASE || tipo == ZOMBIE_BANDERA);

                int vida_max = obtener_vida_maxima(juego, tipo);
                if (vida_max < vida_actual) vida_max = vida_actual;
                float porcentaje = (float)vida_actual / (float)vida_max;

                int ancho_barra = TAMANO_CELDA - 14;
                int barra_x = x + 7;
                int barra_y = y + 4;
                int barra_h = 6;

                DrawRectangle(barra_x - 2, barra_y - 2, ancho_barra + 4, barra_h + 4, BLACK);
                DrawRectangle(barra_x, barra_y, ancho_barra, barra_h, (Color){ 32, 32, 32, 240 });

                Color color_barra = es_zombie ? RED : LIME;
                if (porcentaje <= 0.35f) color_barra = ORANGE;
                int ancho_relleno = (int)(ancho_barra * porcentaje);
                if (ancho_relleno > 0) {
                    DrawRectangle(barra_x, barra_y, ancho_relleno, barra_h, color_barra);
                    DrawRectangle(barra_x, barra_y, ancho_relleno, 2, Fade(WHITE, 0.35f));
                }

                const char *texto_hp = TextFormat("%d", vida_actual);
                int ancho_txt = MeasureText(texto_hp, 14);
                int caja_w = ancho_txt + 8;
                DrawRectangle(x + TAMANO_CELDA - caja_w - 3, y + TAMANO_CELDA - 19, caja_w, 16, (Color){ 0, 0, 0, 195 });
                DrawText(texto_hp, x + TAMANO_CELDA - caja_w + 1, y + TAMANO_CELDA - 18, 14, es_zombie ? ORANGE : GREEN);
            }
        }
    }

    // Rayos de disparo de los lanzaguisantes centrados desde su boca hasta el zombi objetivo
    for (int fila = 0; fila < juego->filas && fila < MAX_FILAS; fila++) {
        if (efectos.timer_disparo[fila] > 0.0f) {
            float alpha = efectos.timer_disparo[fila] / 0.20f;
            if (alpha > 1.0f) alpha = 1.0f;

            int col_fin = efectos.col_objetivo[fila];
            float fin_x = col_fin * TAMANO_CELDA + TAMANO_CELDA * 0.45f;
            float fin_y = fila * TAMANO_CELDA + ALTO_HUD + TAMANO_CELDA * 0.46f;

            for (int col = 0; col < col_fin; col++) {
                if (juego->jardin[fila][col].tipo == PLANTA_LANZAGUISANTES) {
                    float ini_x = col * TAMANO_CELDA + TAMANO_CELDA * 0.76f;
                    float ini_y = fila * TAMANO_CELDA + ALTO_HUD + TAMANO_CELDA * 0.44f;

                    DrawLineEx((Vector2){ini_x, ini_y}, (Vector2){fin_x, fin_y}, 8.0f, Fade(LIME, alpha * 0.45f));
                    DrawLineEx((Vector2){ini_x, ini_y}, (Vector2){fin_x, fin_y}, 3.5f, Fade(GREEN, alpha));
                    DrawLineEx((Vector2){ini_x, ini_y}, (Vector2){fin_x, fin_y}, 1.5f, Fade(YELLOW, alpha));
                }
            }
            DrawCircle((int)fin_x, (int)fin_y, 12.0f * alpha, Fade(LIME, alpha * 0.8f));
        }
    }

    // Barra inferior con cronometro, barra de progreso con banderas y boton Menu
    int y_inf = ALTO_HUD + alto_jardin;
    DrawRectangle(0, y_inf, ancho_pantalla, ALTO_BARRA_INFERIOR, (Color){ 42, 26, 12, 255 });
    DrawRectangle(0, y_inf, ancho_pantalla, 2, (Color){ 25, 14, 6, 255 });

    int min_actual = (int)tiempo_transcurrido / 60;
    int seg_actual = (int)tiempo_transcurrido % 60;
    DrawText(TextFormat("Tiempo: %02d:%02d", min_actual, seg_actual), 15, y_inf + 14, 19, WHITE);

    if (tiempo_prep_restante > 0.0f) {
        DrawText(TextFormat("Prep.: %.0fs", tiempo_prep_restante), 165, y_inf + 14, 18, YELLOW);
    }

    int bar_w = 180;
    int bar_h = 18;
    int bar_x = ancho_pantalla - bar_w - 105;
    int bar_y = y_inf + 15;

    DrawText("Progreso:", bar_x - 86, y_inf + 14, 17, LIGHTGRAY);

    DrawRectangle(bar_x - 3, bar_y - 3, bar_w + 6, bar_h + 6, (Color){ 22, 16, 10, 255 });
    DrawRectangle(bar_x, bar_y, bar_w, bar_h, (Color){ 65, 55, 45, 255 });

    int fill_w = (int)(bar_w * progreso_nivel);
    int fill_x = bar_x + (bar_w - fill_w);
    if (fill_w > 0) {
        DrawRectangle(fill_x, bar_y, fill_w, bar_h, (Color){ 115, 215, 50, 255 });
        DrawRectangle(fill_x, bar_y + 2, fill_w, 5, (Color){ 165, 245, 95, 255 });
    }

    for (int w = 1; w <= total_oleadas; w++) {
        float hito = ((float)(w - 1) + 0.88f) / (float)total_oleadas;
        int flag_x = bar_x + bar_w - (int)(bar_w * hito);
        int elevacion = (progreso_nivel >= hito - 0.005f || oleada_actual >= w) ? 6 : 0;

        DrawRectangle(flag_x, bar_y - 11 - elevacion, 2, 13, WHITE);
        DrawTriangle(
            (Vector2){ (float)flag_x, (float)(bar_y - 11 - elevacion) },
            (Vector2){ (float)(flag_x - 11), (float)(bar_y - 6 - elevacion) },
            (Vector2){ (float)flag_x, (float)(bar_y - 1 - elevacion) },
            RED
        );
    }

    int head_y = bar_y + bar_h / 2;
    DrawCircle(fill_x, head_y, 8.5f, (Color){ 40, 55, 35, 255 });
    DrawCircle(fill_x, head_y, 7.0f, (Color){ 125, 160, 110, 255 });
    DrawCircle(fill_x - 2, head_y - 1, 1.5f, BLACK);
    DrawCircle(fill_x + 2, head_y - 1, 1.5f, BLACK);

    Rectangle btn_menu = { (float)(ancho_pantalla - 86), (float)(y_inf + 9), 74, 28 };
    bool hover_menu = !pausado && CheckCollisionPointRec(mouse, btn_menu);
    DrawRectangleRec(btn_menu, hover_menu ? (Color){ 115, 78, 42, 255 } : (Color){ 78, 52, 26, 255 });
    DrawRectangleLinesEx(btn_menu, 1.5f, hover_menu ? GOLD : (Color){ 140, 100, 55, 255 });
    DrawText("Menu", (int)btn_menu.x + 17, (int)btn_menu.y + 6, 16, WHITE);

    // Cartel de alerta de horda, ventana modal de pausa y mensajes de victoria o derrota
    if (efectos.timer_alerta_horda > 0.0f && juego->juego_terminado == 0) {
        int banner_y = ALTO_HUD + alto_jardin / 2 - 28;
        DrawRectangle(0, banner_y, ancho_pantalla, 56, (Color){ 0, 0, 0, 195 });
        const char *msg_horda = efectos.es_horda_final
                                ? "¡SE APROXIMA UNA GRAN HORDA FINAL DE ZOMBIS!"
                                : "¡SE APROXIMA UNA HORDA DE ZOMBIS!";
        int msg_w = MeasureText(msg_horda, 23);
        DrawText(msg_horda, (ancho_pantalla - msg_w) / 2, banner_y + 16, 23, RED);
    }

    if (pausado && juego->juego_terminado == 0) {
        DrawRectangle(0, 0, ancho_pantalla, alto_pantalla, (Color){ 0, 0, 0, 180 });

        float panel_w = 430.0f;
        float panel_h = 285.0f;
        float panel_x = (ancho_pantalla - panel_w) / 2.0f;
        float panel_y = (alto_pantalla - panel_h) / 2.0f;

        Rectangle panel = { panel_x, panel_y, panel_w, panel_h };
        DrawRectangle((int)panel_x + 5, (int)panel_y + 6, (int)panel_w, (int)panel_h, (Color){ 15, 10, 5, 210 });
        DrawRectangleRec(panel, (Color){ 68, 44, 24, 255 });
        DrawRectangleLinesEx(panel, 3.0f, (Color){ 155, 112, 64, 255 });

        const char *tit_pausa = "JUEGO EN PAUSA";
        DrawText(tit_pausa, ancho_pantalla / 2 - MeasureText(tit_pausa, 26) / 2, (int)panel_y + 20, 26, GOLD);

        Rectangle btn_reanudar = { panel_x + 45.0f, panel_y + 72.0f,  panel_w - 90.0f, 50.0f };
        Rectangle btn_guardar  = { panel_x + 45.0f, panel_y + 138.0f, panel_w - 90.0f, 50.0f };
        Rectangle btn_salir    = { panel_x + 45.0f, panel_y + 204.0f, panel_w - 90.0f, 50.0f };

        dibujar_boton_lapida(btn_reanudar, "CONTINUAR PARTIDA", 20,
                             (Color){ 68, 130, 42, 255 }, (Color){ 88, 165, 52, 255 },
                             (Color){ 32, 65, 18, 255 }, WHITE, mouse);

        dibujar_boton_lapida(btn_guardar, "GUARDAR Y SALIR AL MENU", 19,
                             (Color){ 48, 92, 132, 255 }, (Color){ 65, 118, 168, 255 },
                             (Color){ 22, 46, 68, 255 }, WHITE, mouse);

        dibujar_boton_lapida(btn_salir, "VOLVER AL MENU SIN GUARDAR", 18,
                             (Color){ 125, 48, 44, 255 }, (Color){ 158, 58, 52, 255 },
                             (Color){ 65, 22, 20, 255 }, (Color){ 255, 225, 225, 255 }, mouse);
    }

    if (juego->juego_terminado != 0) {
        DrawRectangle(0, 0, ancho_pantalla, alto_pantalla, (Color){ 0, 0, 0, 185 });
        if (juego->juego_terminado == 1) {
            const char *msg = "¡LOS ZOMBIES COMIERON TU CEREBRO!";
            DrawText(msg, (ancho_pantalla - MeasureText(msg, 24)) / 2, alto_pantalla / 2 - 25, 24, RED);
        } else if (juego->juego_terminado == 2) {
            const char *msg = "¡VICTORIA! SOBREVIVISTE A LAS OLEADAS";
            DrawText(msg, (ancho_pantalla - MeasureText(msg, 24)) / 2, alto_pantalla / 2 - 25, 24, GREEN);
        }
        const char *sub = "Presiona [ESC] para volver al Menu Principal";
        DrawText(sub, (ancho_pantalla - MeasureText(sub, 18)) / 2, alto_pantalla / 2 + 20, 18, WHITE);
    }
}