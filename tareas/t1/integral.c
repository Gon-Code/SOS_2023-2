#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

#include "integral.h"

// Definimos nuestra estructura
typedef struct {
    Funcion function;
    void* ptr;
    double xi, xf;
    int n;
    int p;
    double *resultado;
} Args;

// Funcion que ocupa el thread
void *thread_function(void* params){
    //Rescatamos nuestros parametros
    Args *pm = (Args *) params;
    Funcion f = pm->function;
    void * ptr = pm->ptr;
    double xi = pm->xi;
    double xf = pm->xf;
    int n = pm->n;
    int p = pm->p;
    double area = integral_par(f, ptr, xi, xf, n, p);
    *(pm->resultado) = area; // Actualizar el área en el puntero resultado
    return NULL;
}


double integral_par(Funcion f, void *ptr, double xi, double xf, int n, int p) {
    if (xi < xf) {
        // Caso base : Se llama a integral secuencial
        if (p == 1) {
            return integral(f, ptr, xi, xf, n);
        } else { // Se divide y se calcula a la izq y derecha
            pthread_t thread;
            double largo = (xf - xi) / 2;
            double area_left, area_right;
            Args arg = {f, ptr, xi, xi + largo, n / 2, p - p / 2, &area_left}; // Notese que xf ahora es la mitad
            pthread_create(&thread, NULL, thread_function, &arg); // Calcula a la izquierda con el thread function, la cual tambien llama a integral par
            area_right = integral_par(f, ptr, xi + largo, xf, n / 2, p / 2); // Calcula a la derecha recursivamente, ahora xi es la mitad
            pthread_join(thread, NULL); // Esperamos a los threads
            area_left = *(arg.resultado); // Suma el area que calculo el thread
            return area_left + area_right; // Se retorna el area de la derecha con el de la izquierda
        }
    }
    return 0.0;
}

