typedef struct subasta *Subasta;

void fatalError( char *procname, char *format, ... );

Subasta nuevaSubasta(int unidades);
int ofrecer(Subasta s, double precio);
double adjudicar(Subasta s, int *punidades);
void destruirSubasta(Subasta s);

enum { FALSE= 0, TRUE= 1 };
