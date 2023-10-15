#define _XOPEN_SOURCE 500
#include "nthread-impl.h"
#include "pss.h"
#include "h2o.h"


// Crearemos 2 colas globales, una para el hidrogeno y otra para el oxigeno
NthQueue *cola_h;
NthQueue *cola_o;

void initH2O(void) {
  cola_h = nth_makeQueue();
  cola_o = nth_makeQueue(); 
}

void endH2O(void) {
  nth_destroyQueue(cola_h);
  nth_destroyQueue(cola_o);
}

H2O *nCombineOxy(Oxygen *o, int timeout) {
  //Inhibe las interrupciones/señales
  START_CRITICAL;
  //Creamos el puntero a la molecula de agua
  H2O * agua = NULL;
  //Si hay menos de 2 hidrogenos, no podemos crear la molecula, asi que el nthread debe esperar
  if ((nth_queueLength(cola_h)<2)){
    //Obtenemos el nThread actual
    nThread thisTh=nSelf();
    //Le asociamos el oxigeno 
    thisTh->ptr=o;
    //Añadimos el nThread a la cola de oxigeno
    nth_putBack(cola_o, thisTh);
    //Mientras el status del nThread no sea READY, no le entregamos core
    if (thisTh->status!=READY){
      suspend(WAIT_PUB);
    }
    schedule();
    //Este es el caso cuando no se pudo formar la molecula :c
    agua=thisTh->ptr;
    END_CRITICAL;
    return agua;

  }// En esta caso hay suficientes hidrogenos para crear la molecula
  else{
    //Tomamos los 2 primero nThreads de la cola hidrogeno
    nThread th_h1 = nth_getFront(cola_h); 
    nThread th_h2 = nth_getFront(cola_h);
    //Obtenemos sus hidrogenos asociados
    Hydrogen* h1 = th_h1->ptr;
    Hydrogen* h2 = th_h2->ptr;
    //Creamos la molecula de agua
    agua= makeH2O(h1,h2,o);
    //Guardamos la molecula de agua en los punteros de los nThreads que traian los hidrogenos
    th_h1->ptr=agua;
    th_h2->ptr=agua;
    //Cambiamos el status de los threads a READY
    setReady(th_h1);
    setReady(th_h2);
    //Entregamos el control al schedule
    schedule();
    END_CRITICAL;
    return agua;

  }
}

H2O *nCombineHydro(Hydrogen *h) {
  START_CRITICAL;
  //Creamos el puntero a la molecula de agua
  H2O * agua = NULL;
  //En este caso no hay suficientes elementos para crear una molecula de agua
  // O bien no hay hidrogenos, o bien no hay oxigenos
  if(nth_emptyQueue(cola_h) || nth_emptyQueue(cola_o)){
    //Obtenemos el nThread actual
    nThread thisTh=nSelf();
    //Le asociamos el hidrogeno actual
    thisTh->ptr=h;
    //Ubicamos el nThread en la cola de hidrogeno
    nth_putBack(cola_h, thisTh);
    //Mientras sus status no sea READY, no le entregamos core
    if (thisTh->status!=READY){
      suspend(WAIT_PUB);
    }
    schedule();
    //Este es el caso donde no se pudo formar la molecula
    agua = thisTh->ptr;
    END_CRITICAL;
    return agua;
  }else{
    //Tomamos los primeros nThreads de cada cola
    nThread th_h1 = nth_getFront(cola_h);
    nThread th_o1 = nth_getFront(cola_o);
    //Obtenemos su hidrogeno y oxigeno asociados respectivamente
    Hydrogen* h1 = th_h1->ptr;
    Oxygen* o1 = th_o1->ptr;
    //Creamos la molecula de agua
    agua= makeH2O(h1,h,o1);
    //Guardamos la molecula de agua en los nThreads que trajeron 1 hidrogeno y 1 oxigeno
    th_h1->ptr=agua;
    th_o1->ptr=agua;
    //Cambiamos el status de los nThreads a READY
    setReady(th_h1);
    setReady(th_o1);
    schedule();
    END_CRITICAL;
    return agua;
  }
}

