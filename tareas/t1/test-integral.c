#include <stdio.h>
#include <pthread.h>
#include <math.h>
#include <sys/time.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "integral.h"

#ifdef OPT

#define N_INTENTOS 4
#define TOLERANCIA 1.5

#else

#define N_INTENTOS 1
#define TOLERANCIA 0.1

#endif

#pragma GCC diagnostic ignored "-Wunused-function"

// ----------------------------------------------------
// Funcion que entrega el tiempo transcurrido desde el lanzamiento del
// programa en milisegundos

static int time0= 0;

static int getTime0() {
    struct timeval Timeval;
    gettimeofday(&Timeval, NULL);
    return Timeval.tv_sec*1000+Timeval.tv_usec/1000;
}

void resetTime() {
  time0= getTime0();
}

int getTime() {
  return getTime0()-time0;
}

// ----------------------------------------------------

int leer(int fd, void *vbuf, int n) {
  char *buf= vbuf;
  do {
    int rc= read(fd, buf, n);
    if (rc<=0)
      return 1; /* fracaso: error o fin del archivo/pipe/socket */
    n-= rc; /* descontamos los bytes leídos */
    buf+= rc; /* avanzamos el buffer para no reescribir lo leido previamente */
  } while (n>0); /* mientras no leamos todo lo que esperamos */
  return 0; /* exito */
}

// ----------------------------------------------------

// integral: calcula integral f(ptr,x) en rango [xi,xf] aproximando
// el area bajo la curva como n trapecios
double integral(Funcion f, void *ptr, double xi, double xf, int n) {
  int k;
  double h= (xf-xi)/n;
  double f_xi_xf= ( (*f)(ptr, xi) + (*f)(ptr, xf) ) / 2;
  double sum= 0.0;
  for (k= 1; k<n; k++)
    sum+= (*f)(ptr, xi+k*h);
  double area= h*(f_xi_xf + sum);
  printf("integral en [%f,%f] con %d intervalos= %f\n", xi, xf, n, area);
  return area;
}

double integral_int(Funcion f, void *ptr, double xi, double xf, int n) {
  int k;
  double h= (xf-xi)/n;
  double f_xi_xf= ( (*f)(ptr, xi) + (*f)(ptr, xf) ) / 2;
  double sum= 0.0;
  for (k= 1; k<n; k++)
    sum+= (*f)(ptr, xi+k*h);
  double area= h*(f_xi_xf + sum);
  return area;
}

double g(double x, double y) {
  return sin(x+2*y);
}

typedef struct {
  double xi, xf;
  int n;
} Param;

double g_aux(void *ptr, double x) {
 double y= *(double *)ptr;
 return g(x, y);
}

// integral_g_dx: calcula integral sin(x+2*y) dx en [xi,xf] aproximando
// con n trapecios
double integral_g_dx(double xi, double xf, double y, int n) {
  double area= integral_int(g_aux, &y, xi, xf, n);
  return area;
}

double integral_g_dx_aux(void *ptr, double y) {
  Param *param= (Param*)ptr;
  double area= integral_g_dx(param->xi, param->xf, y, param->n);
  return area;
}

// integral_g_dx_dy: calcula secuencialmente
// integral integral sin(x+2*y) dx dy con y en [yi,yf], x en [xi,xf]
// n es la cantidad de trapecios para x, m es para y
// Llama al integrar secuencial.
double integral_g_dx_dy(double xi, double xf, int n,
                        double yi, double yf, int m) {
  Param param= { xi, xf, n };
  double area= integral(integral_g_dx_aux, &param, yi, yf, m);
  // printf("integral en [%f,%f] con %d intervalos= %f\n", yi, yf, n, area);
  return area;
}

// integral_g_dx_dy_par: calcula lo mismo en paralelo con p threads
// llamando a integral_par
double integral_g_dx_dy_par(double xi, double xf, int n,
                        double yi, double yf, int m, int p) {
  Param param= { xi, xf, n };
  double area= integral_par(integral_g_dx_aux, &param, yi, yf, m, p);
  return area;
}

// La integral del ejemplo tiene solucion algebraica:
// integral_g_dx(a,b,y,n) ~= integral sin(x+2*y) dx en [a,b]
//                  = integral sin(x) dx en [xi+2*y, xf+2*y]
//                  = cos (2*y + a) − cos ( 2*y + b)
// Calculada en https://www.calculadora-de-integrales.com/

// integral_g_dx_dy(a,f,n,c,d,m) ~=
//   integral ( integral sin(x+2*y) dx en [a,b] ) dy en [c,d]
//                  = 1/2*(sin(d+2*b)-sin(d+2*a)-sin(c+2*b)+sin(c+2*a))
// Calculada en https://www.calculadora-de-integrales.com/

#define N 10000
#define M 20000
#define P 4
int main() {
  double a= 0.0, b= 0.5;
  double c= -0.2, d= 0.6;

  double algebraico= -(sin(2*d+b)-sin(2*d+a)-sin(2*c+b)+sin(2*c+a))/2;

  int tiempo_sec, tiempo_par;
  double sec, par;
  double speedUp= 0;
  int i;
  
  printf("Calculando secuencialmente 2 veces\n");
  for (i= 0; i<2; i++) {
    resetTime();
    sec= integral_g_dx_dy(a, b, N, c, d, M);
    tiempo_sec= getTime();
    double err= fabs(sec-algebraico);
    printf("integral= %f, valor esperado= %f, error= %f, tiempo= %d\n",
        sec, algebraico, err, tiempo_sec);
  }
  
  printf("Calculando en paralelo hasta %d veces\n", N_INTENTOS);
  for (i= 0; i<N_INTENTOS; i++) {
    resetTime();
    par= integral_g_dx_dy_par(a, b, N, c, d, M, P);
    tiempo_par= getTime();
    double err= fabs(par-algebraico);
    speedUp= (double)tiempo_sec/tiempo_par;
    printf("integral par= %f, valor esperado= %f, error= %f, "
           "tiempo= %d, speedup= %f\n",
        par, algebraico, err, tiempo_par, speedUp);
    if (err>0.000001) {
       fprintf(stderr, "El error es demasiado grande.  "
                       "La integral en paralelo esta mal calculada. Revise.\n");
       exit(1);
    }
    if (speedUp>=TOLERANCIA)
      break;
  }
  if (i>=N_INTENTOS) {
    fprintf(stderr, "Despues de %d intentos no obtuvo un speedup de 1.5x\n",
            N_INTENTOS);
    fprintf(stderr, "Revise la paralelizacion.\n");
    exit(1);
  }
#ifdef OPT
  printf("Felicitaciones: el speedup es mejor que 1.5x.  Su tarea funciona.\n");
#else
  printf("Felicitaciones: Su tarea funciona.\n");
#endif
  
  return 0;
}

