#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <nthread.h>

#include "pss.h"
#include "equipo.h"

#define STEP 400

#ifndef OPT
#define N 1000
// Cuidado: M debe ser multiplo de 5
#define M 100
#else
#define N 5000
// Cuidado: M debe ser multiplo de 5
#define M 500
#endif

pthread_mutex_t mut= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t tabm= PTHREAD_MUTEX_INITIALIZER;

// ==================================================
// Funcion para reportar un error y detener la ejecucion

static __thread char *thread_nom;

static void setTaskName(char *nom) {
  thread_nom= nom;
}

static char *getTaskName(void) {
  return thread_nom;
}

void fatalError( char *procname, char *format, ... ) {
  va_list ap;

  fprintf(stderr,"Error Fatal en la rutina %s y el thread \n", procname);
  fprintf(stderr, "%s\n", getTaskName());
  va_start(ap, format);
  vfprintf(stderr, format, ap);
  va_end(ap);

  exit(1); /* shutdown */
}

// ==================================================

static void registrar_equipo(char **equipo, int crc);

typedef struct {
  char *nom;
  char ***pequipo;
}  Param;

static void *jugador(void *ptr) {
  Param *pparm= ptr;
  char *nom= pparm->nom;
  setTaskName(nom);
  char ***pequipo= pparm->pequipo;
  free(pparm);
  char **equipo= hay_equipo(nom);
  int i= 5;
  int crc= 0;

  for (int k= 0; k<5; k++) {
    char *s= equipo[k];
    while (*s)
      crc += *s++;
    if (strcmp(equipo[k], nom)==0)
      i= k;
  }

  if (i==5)
    fatalError("jugador", "%s no esta en el equipo\n", nom);

  registrar_equipo(equipo, crc);

  if (pequipo!=NULL)
    *pequipo= equipo;

  return 0;
}

pthread_t createJugador(char *nom, char ***pequipo) {
   Param parm= {nom, pequipo};
   Param *pparm= malloc(sizeof(Param));
   *pparm= parm;
   pthread_t t;
   pthread_create(&t, NULL, jugador, pparm);
   return t;
}

static char **noms;
static int prox;
static int tot_equipos= 0, tot_jugadores= 0;

typedef struct {
  char **equipo;
  int crc;
} EquipoCrc;

static EquipoCrc tabla_equipos[N*M];

static void registrar_equipo(char **equipo, int crc) {
  int hash= ((intptr_t)equipo / (5*sizeof(char*)))%(N*M);
  EquipoCrc *eqcrc= &tabla_equipos[hash];
  int vuelta= 0;
  pthread_mutex_lock(&tabm);
  tot_jugadores++;
  while (eqcrc->equipo!=NULL && eqcrc->equipo!=equipo) {
    hash++;
    eqcrc++;
    if (hash==N*M) {
      if (vuelta)
        fatalError("registrar_equipo", "Demasiados equipos\n");
      vuelta= 1;
      hash= 0;
      eqcrc= &tabla_equipos[0];
    }
  }
  if (eqcrc->equipo==NULL) {
    eqcrc->equipo= equipo;
    eqcrc->crc= crc;
    tot_equipos++;
    // printf(".");
  }
  else if (eqcrc->crc!=crc) {
    fprintf(stderr, "El equipo muto despues del retorno de hay_equipo.\n");
    fprintf(stderr, "Esto significa que hay_equipo retorno con el equipo\n");
    fprintf(stderr, "incompleto, es decir antes de la llegada de todos los\n");
    fprintf(stderr, "jugadores.  Esto es incorrecto.\n");
    fatalError("registrar_equipo", "Revise su tarea\n");
  }
  pthread_mutex_unlock(&tabm);
}

void *mult_jug(void *ptr) {

  pthread_mutex_lock(&mut);
  while (prox<N*M) {
    char *nom= noms[prox++];
    pthread_mutex_unlock(&mut);
    Param parm= {nom, NULL};
    Param *pparm= malloc(sizeof(Param));
    *pparm= parm;
    jugador(pparm);
    pthread_mutex_lock(&mut);
  }
  pthread_mutex_unlock(&mut);
  return 0;
}

pthread_t createMultJug(void) {
  pthread_t t;
  pthread_create(&t, NULL, mult_jug, NULL);
  return t;
}

int main() {

    int njug= N*M;
    for (int j=0; j<njug; j++) {
      tabla_equipos[j].equipo= NULL;
    }
    init_equipo();
  
#if 1
    {
      char **equipos[10];
  
      printf("Test 1: el del enunciado\n");
      pthread_t jugadores[10];
      char *noms[]= {"pedro", "juan", "alberto", "enrique", "diego",
                      "jaime", "jorge", "andres", "jose", "luis"};
      for (int i=0; i<10; i++) {
        usleep(STEP*1000);
        printf("lanzando %s\n", noms[i]);
        jugadores[i]= createJugador(noms[i], &equipos[i]);
      }
      for (int i=0; i<10; i++) {
        pthread_join(jugadores[i], NULL);
        printf("terminado %s\n", noms[i]);
      }
      for (int i= 1; i<5; i++) {
        if (equipos[i]!=equipos[0])
          fatalError("main", "%s no esta en el mismo equipo que %s\n",
                  noms[i], noms[0]);
      }
      for (int i= 6; i<10; i++) {
        if (equipos[i]!=equipos[5])
          fatalError("nMain", "%s no esta en el mismo equipo que %s\n",
                  noms[i], noms[5]);
      }
      if (equipos[0]==equipos[5])
        fatalError("nMain", "No pueden ser los mismos equipos\n");
   
      // put(fq, equipos[0]);
      // put(fq, equipos[5]);
      
      printf("Test 1: aprobado\n");
    }
#endif
  
    // nSetTimeSlice(1); // No funciona bien todavia el cambio de scheduler
  
#if 1
    {
      printf("Test 2: busca dataraces.\n");
      printf("Se demoro 20 segundos en mi computador con mi solucion.\n");
      printf("Si termina en segmentation fault podria significar que hubo un\n");
      printf("datarace en el que hay_equipo retorno un equipo incompleto y\n");
      printf("el test intento leer el nombre de un jugador que resulto ser\n");
      printf("una direccion invalida.\n");
      noms= malloc(N*M*sizeof(char*));
      pthread_t *jugadores= malloc(M*sizeof(pthread_t));
      for (int i= 0; i<N*M; i++) {
        char *nom= malloc(10);
        sprintf(nom, "j%08d", i);
        noms[i]= nom;
      }
      for (int i= 0; i<M; i++) {
        jugadores[i]= createMultJug();
      }
      printf("\nEsperando\n");
      for (int i= 0; i<M; i++) {
        pthread_join(jugadores[i], NULL);
      }
      printf("Test 2: aprobado\n");
      free(jugadores);
      for (int i= 0; i<N*M; i++)
        free(noms[i]);
      free(noms);
    }
#endif
  
    printf("Total equipos = %d\n", tot_equipos);
    printf("Total jugadores = %d\n", tot_jugadores);
  
    end_equipo();
  
    for (int k= 0; k<N*M; k++) {
      char **equipo= tabla_equipos[k].equipo;
      if (equipo!=NULL)
        free(equipo);
    }

    printf("Felicitaciones, su tarea paso todos los tests\n");
    return 0;
}
