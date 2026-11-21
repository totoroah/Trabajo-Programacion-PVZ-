#include "controlador.h" // Se incluye la cabecera de controlador.h que contiene la estructura 'Aplicacion' y las 3 funciones principales

int main(void) {
    Aplicacion app; // Reserva en el Stack una variable app tipo Aplicacion (que contiene todo el estado del juego)

    if (!inicializar_aplicacion(&app)) { // Llama inicializar_aplicacion con 'app' para rellenar sus campos, creando la ventana, cargando texturas en memoria y leyendo configuracion.txt y jardin.txt
        return 1;
    }

    ejecutar_bucle_principal(&app); // Inicio del ciclo principal del juego que se mantiene activo manejando todo
    liberar_aplicacion(&app); // Una vez finalizado el bucle principal, libera de forma ordenada toda la memoria dinamica reservada en el Heap, descarga texturas, etc

    return 0;
}