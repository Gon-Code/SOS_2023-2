
// El tipo concreto de struct hydrogen, struct oxygen y struct h2o
// esta definido en test-h2o.c
typedef struct hydrogen Hydrogen;
typedef struct oxygen Oxygen;
typedef struct h2o H2O;

// Prodedimiento dado en testh2o.c:
H2O *makeH2O(Hydrogen *h1, Hydrogen *h2, Oxygen *o);

// Procedimientos que Ud. debe programar en h2o.c
void initH2O(void);
void endH2O(void);
H2O *nCombineOxy(Oxygen *o, int timeout);
H2O *nCombineHydro(Hydrogen *h);

// Esto es para el test de prueba en test-h2o.c

#define combineOxy(o) nCombineOxy(o, -1)
#define combineHydro(h) nCombineHydro(h)
