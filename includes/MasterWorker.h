#ifndef MASTERWORKER_H_
#define MASTERWORKER_H_

#include <conn.h>
#include <util.h>
#include <threadpool.h>
#include <Worker.h>
#include <Collector.h>

#define MAXFILENAME 255

extern long connfd;
extern pthread_mutex_t termina_mtx;
extern pthread_mutex_t connessione_mtx;
extern int termina;


//funzione che attende un tot di tempo e poi aggiunge il task al threadpool
void addTask(int delay, char pathname[], threadpool_t *threadpool);

//funzione ricorsiva che naviga le directory ed inserisce i task nel threadpool
void navigateDirectory(char dir[], int delay, threadpool_t *threadpool);

//funzione che controlla se siamo nel caso di "." o ".."
int isdot(char dir[]);

//funzione che prende in modo thread safe una copia della variabile termina 
int getTermina(int *termina);

//funzione che chiude connessione e aspetta la terminazione del collector
void cleanup();

//Thread che gestisce i vari segnali bloccati in maniera sincrona
void *SignalHandler(void *arg);

//funzione per dire all'utente cosa digitare
void print_usage(const char *programname);

#endif