#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "subasta.h"

// =============================================================
// Use esta cola de prioridades para resolver el problema
// =============================================================

// Puede almacenar hasta 100 elementos.  No se necesita mas para el test.

#define MAXQSZ 100

typedef struct {
  void **vec;
  double *ofertas;
  int size, maxsize;
} *PriQueue;

PriQueue MakePriQueue(int size);    // Constructor
void DestroyPriQueue(PriQueue pq);  // Destructor

// Las operaciones
void *PriGet(PriQueue pq);          // Extraer elemento de mejor prioridad
void PriPut(PriQueue pq, void* t, double pri); // Agregar con prioridad
double PriBest(PriQueue pq);           // Prioridad del mejor elemento
int EmptyPriQueue(PriQueue pq);     // Verdadero si la cola esta vacia
int PriLength(PriQueue pq);         // Largo de la cola

// =============================================================
// Implemente a partir de aca el tipo Subasta
// =============================================================

//defina aca la estructura de datos para la subasta
struct subasta{
    int items;    //Cantidad de objetos a subastar
    pthread_mutex_t mutex; // Mutex de la subasta
    PriQueue cola;    // Cola de prioridad de la subasta, contiene toda la información para resolver el problema
};

//Cada ofertante ingresa un request, en donde va su condicion y su ready.
typedef struct{
    int ready;
    pthread_cond_t c;
} Request;

//... programe aca las funciones solicitadas: nuevaSubasta, adjudicar, etc. ...

// Inicializa una nueva subasta
Subasta nuevaSubasta(int unidades){
    Subasta subasta = (Subasta)malloc(sizeof(struct subasta));
    subasta->items = unidades;
    pthread_mutex_init(&subasta->mutex,NULL);
    subasta->cola=MakePriQueue(100);
    return subasta;
}

int ofrecer(Subasta s, double precio){
    pthread_mutex_lock(&s->mutex); // Cerramos el mutex
    int cnt_ofertas = PriLength(s->cola); // Contador de ofertas
    double peor_oferta = PriBest(s->cola);
    Request nreq = {0,PTHREAD_COND_INITIALIZER}; // Nuevo Request que añadiremos a la cola
    //Si la cola esta llena 
    if(cnt_ofertas == s->items){
        if(peor_oferta < precio){ // La nueva oferta es mejor que la peor
            void* req = PriGet(s->cola); // Sacamos el mejor elemento ( la peor oferta )
            PriPut(s->cola,&nreq,precio); // Añadimos la nueva oferta
            Request* reqp = (Request*)req;
            reqp->ready=1;
            pthread_cond_signal(&reqp->c); // Despertamos al mutex
        }
        else{ // Nuestra oferta es peor que la peor, asi que no entramos y retornamos
            pthread_mutex_unlock(&s->mutex);
            return FALSE;
        
        }
    } 
    //La cola no esta llena
    if(cnt_ofertas  <  s->items){ // Si aun no hay suficientes ofertas para todos los objetos, entramos directamente a la cola
        PriPut(s->cola,&nreq,precio); // Añadimos la nueva oferta        
    }
    
    //Si el programa llega hasta aca es porque nuestra oferta ha sido añadida
    // Ahora esperamos hasta que nuestra oferta sea superada O la subasta termine
   
    while(nreq.ready==0){
        pthread_cond_wait(&nreq.c,&s->mutex); // Esperamos y entregamos el mutex
    }
    // Si estoy aqui es porque me despertaron, y debo saber por qué 
    // Soy mejor que la cola
    if(precio >= PriBest(s->cola)){ // Me despertaron con adjudicar, asi que retorno verdadero
        pthread_mutex_unlock(&s->mutex);
        return TRUE;
    }
    else{ //Me despertaron porque mi oferta es la peor
        pthread_mutex_unlock(&s->mutex);
        return FALSE;
    }
}

// La mision de adjudicar es despertar a todas las condiciones para que ofrecer retorne
// Debe RETORNAR el monto recaudado, y en punidades las que no se vendieron
double adjudicar(Subasta s, int* punidades) {   
    pthread_mutex_lock(&s->mutex);  // Bloquea el mutex de la subasta antes de realizar operaciones en ella
    double recaudacion = 0;
    *punidades = s->items;
    int largo = PriLength(s->cola);
    double* ofertas = (s->cola)->ofertas;
    void** t = (s->cola)->vec;
    for (int i = 0; i < largo; i++) {
        void* req = t[i];
        Request* nreq = (Request*)req;
        recaudacion += ofertas[i];
        (*punidades)--;
        nreq->ready = 1;
        pthread_cond_signal(&nreq->c);  // Despierta a los hilos que esperan en esta condición
    }
    pthread_mutex_unlock(&s->mutex);  // Desbloquea el mutex de la subasta después de terminar las operaciones

    return recaudacion;
}


void destruirSubasta(Subasta s){
    
    DestroyPriQueue(s->cola);
    free(s);
}


//Subasta nuevaSubasta(int unidades);
//int ofrecer(Subasta s, double precio);
//double adjudicar(Subasta s, int *punidades);
//void destruirSubasta(Subasta s);


// =============================================================
// No toque nada a partir de aca: es la implementacion de la cola
// de prioridades
// =============================================================

PriQueue MakePriQueue(int maxsize) {
  PriQueue pq= malloc(sizeof(*pq));
  pq->maxsize= maxsize;
  pq->vec= malloc(sizeof(void*)*(maxsize+1));
  pq->ofertas= malloc(sizeof(double)*(maxsize+1));
  pq->size= 0;
  return pq;
}

void DestroyPriQueue(PriQueue pq) {
  free(pq->vec);
  free(pq->ofertas);
  free(pq);
}

void *PriGet(PriQueue pq) {
  void *t;
  int k;
  if (pq->size==0)
    return NULL;
  t= pq->vec[0];
  pq->size--;
  for (k= 0; k<pq->size; k++) {
    pq->vec[k]= pq->vec[k+1];
    pq->ofertas[k]= pq->ofertas[k+1];
  }
  return t;
}

void PriPut(PriQueue pq, void *t, double oferta) {
  if (pq->size==pq->maxsize)
    fatalError("PriPut", "Desborde de la cola de prioridad\n");
  int k;
  for (k= pq->size-1; k>=0; k--) {
    if (oferta > pq->ofertas[k])
      break;
    else {
      pq->vec[k+1]= pq->vec[k];
      pq->ofertas[k+1]= pq->ofertas[k];
    }
  }
  pq->vec[k+1]= t;
  pq->ofertas[k+1]= oferta;
  pq->size++;
}

double PriBest(PriQueue pq) {
  return pq->size==0 ? 0 : pq->ofertas[0];
}

int EmptyPriQueue(PriQueue pq) {
  return pq->size==0;
}

int PriLength(PriQueue pq) {
  return pq->size;
}
