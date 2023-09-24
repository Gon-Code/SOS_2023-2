#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "equipo.h"

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int ticket = 0; // Tiket que se entrega a cada jugador para atender de forma ordenada
int display = 0; // Indica al jugador que se esta atendiendo
char ***equipos = NULL; // Inicialmente, no hay equipos
int max_equipos = 1; 
int jugadores_por_equipo = 5;

// Funcion que pide mas espacio en memoria para equipos para inscribir un nuevo equipo
void crear_equipo(int num_equipo) {
    equipos[num_equipo] = malloc(jugadores_por_equipo * sizeof(char*));
    for (int j = 0; j < jugadores_por_equipo; j++) {
        equipos[num_equipo][j] = NULL;
    }
}

char **hay_equipo(char *nombre) {
    pthread_mutex_lock(&mutex); // Cerramos el mutex
    int my_num = ticket++; // Tomamos un numero y aumentamos el display
    while (my_num != display) { // Mientras no sea mi turno espero 
        pthread_cond_wait(&cond, &mutex);
    }
    int equipo_actual = my_num / jugadores_por_equipo; // Calculamos el equipo actual
    int posicion = my_num % jugadores_por_equipo; // Calculamos su posicion para guardarlo

    if (equipos == NULL) { // Esto solo ocurre con el primer equipo
        equipos = malloc(max_equipos * sizeof(char**)); // Se crea solo un equipo
        crear_equipo(0); // Creamos el espacio para los 5 nuevos jugadores
    } else if (equipo_actual == max_equipos) { // Cuando el equipo actual es igual al maximo
        max_equipos++; // Aumentamos la cantidad max de equipos
        equipos = realloc(equipos, max_equipos * sizeof(char**)); // Le añadimos espacio de memoria a equipos con realloc, aumenta en 1 los equipos
        crear_equipo(equipo_actual);// Creamos el espacio para los 5 nuevos jugadores
    }

    char **equipo = equipos[equipo_actual]; // Creamos un puntero al equipo actual, el cual se encuentra en equipos[equipo_actual]
    equipo[posicion] = nombre; // Escribimos su nombre
    display++; // Aumentamos el display
    // Si hay 5 jugadores en espera, los despertamos a todos
    if (display % jugadores_por_equipo == 0) {
        pthread_cond_broadcast(&cond);
    } else { // Si hay menos de 5 los hacemos esperar con wait
        pthread_cond_wait(&cond, &mutex);
    }

    pthread_mutex_unlock(&mutex); // Abrimos el mutex
    return equipo;
}

// No uso esta funcion porque se corre una vez en el main durante la ejecucion y no se adapta a mi solucion
void init_equipo(void) {

}
// Lo mismo para end equipo
void end_equipo(void) {
  
}


