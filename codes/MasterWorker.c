#include <MasterWorker.h>

//variabili globali che dovranno essere accedibili da cleanup
int collectorPid = 0;
long connfd = 0;

int termina = 0;    //variabile per terminare il programma

pthread_mutex_t termina_mtx = PTHREAD_MUTEX_INITIALIZER;    
pthread_mutex_t connessione_mtx = PTHREAD_MUTEX_INITIALIZER;


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


//Main Thread = Master Thread
int main(int argc, char *argv[]){

    int numThread = 4;  //numero di thread Worker (default = 4) 
    int qlen = 8;  //lunghezza della coda concorrente (default = 8)
    int delay = 0;  //delay 
    char dir[MAXFILENAME + 1];  //directory in cui sono contenuti file binari  
    dir[0] = '\0';
    long number;
    int c;

    //controllo il numero di argomenti
    if(argc < 2){
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    //parsing degli argomenti
    while((c = getopt(argc, argv, ":n:q:d:t:h")) != -1){
        switch(c){
        
        case 'n':
            if(isNumber(optarg, &(number)) != 0 || number <= 0){
                printf("Argomento n non valido\n");
                exit(EXIT_FAILURE);
            }
            else{
                numThread = (int)number;
            }
            break;
        case 'q':
            if(isNumber(optarg, &(number)) != 0 || number <= 0){
                printf("Argomento q non valido\n");
                exit(EXIT_FAILURE);
            }
            else{
                qlen = (int)number;
            }
            break;
        case 'd':
            strncpy(dir, optarg, MAXFILENAME + 1);
            break;
        case 't':
            if(isNumber(optarg, &(number)) != 0 || number < 0){
                printf("Argomento t non valido\n");
                exit(EXIT_FAILURE);
            }
            else{
                delay = (int)number;
            }
            break;
        case 'h':
            print_usage(argv[0]);
            exit(EXIT_FAILURE);
            break;
        case ':':   //caso in cui manca un argomento
            printf("L'opzione '-%c' richiede un argomento\n", optopt);
            print_usage(argv[0]);
            exit(EXIT_FAILURE);
            break;
        case '?':   //caso in cui non riconosce l'opzione
            printf("L'opzione '-%c' non è gestita\n", optopt);
            print_usage(argv[0]);
            exit(EXIT_FAILURE);
            break;
        default:    //caso in cui non viene riconosciuto un comando
            printf("Comando non riconosciuto\n");
            print_usage(argv[0]);
            exit(EXIT_FAILURE);
            break;
        }
    }

    //gestisco i vari segnali, bloccandoli inizialmente per evitare di ricevere interruzioni
    sigset_t mask, oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGQUIT);
    sigaddset(&mask, SIGHUP);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGUSR1);

    if(pthread_sigmask(SIG_BLOCK, &mask, &oldmask) == -1){
        perror("pthread_sigmask");
        exit(EXIT_FAILURE);
    }

    //ignoro SIGPIPE per evitare di essere terminato da una scrittura su un socket
    struct sigaction s;
    memset(&s, 0, sizeof(s));   //è buona norma inizializzare queste strutture a 0 per evitare flag/campi settati di default
    s.sa_handler = SIG_IGN;
    
    if((sigaction(SIGPIPE, &s, NULL)) == -1){
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    //fork per creare il processo Collector
    if((collectorPid = fork()) == -1){
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if(collectorPid == 0){
        int collectorReturn = functionCollector();
        exit(collectorReturn);
    }

    //uso atexit per poi fare la cleanup 
    if(atexit(cleanup) != 0){
        perror("atexit");
        exit(EXIT_FAILURE); 
    }

    //creo il thread signal handler, passandogli la maschera con i segnali bloccati
    pthread_t sighandler_thread;
    if(pthread_create(&sighandler_thread, NULL, SignalHandler, &mask) != 0){
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }

    //creo il socket 
    int listenfd;
    SYSCALL_EXIT("socket", listenfd, socket(AF_UNIX, SOCK_STREAM, 0), "socket", "");

    //setto l'indirizzo
    struct sockaddr_un serv_addr;
    memset(&serv_addr, '0', sizeof(serv_addr));   //è buona norma inizializzare queste strutture a 0 per evitare flag/campi settati di default
    serv_addr.sun_family = AF_UNIX;
    strncpy(serv_addr.sun_path, SOCKNAME, strlen(SOCKNAME) + 1);

    //assegno l'indirizzo al socket
    int notused;
    SYSCALL_EXIT("bind", notused, bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)), "bind", "");
    
    //setto il socket in modalita' passiva 
    SYSCALL_EXIT("listen", notused, listen(listenfd, 1), "listen", "");
    
    //accetto la connessione
    SYSCALL_EXIT("accept", connfd, accept(listenfd, (struct sockaddr *)NULL, NULL), "accept", "");
    close(listenfd);

    threadpool_t *threadpool = NULL;

	//creo il threadpool con n thread e q dimensione della coda
	if((threadpool = createThreadPool(numThread, qlen)) == NULL){
		perror("createThreadPool"); 
        destroyThreadPool(threadpool, 0);
		exit(EXIT_FAILURE);
	}

    //navigo la directory passata come argomento tramite -d
	if(dir != NULL && strnlen(dir, MAXFILENAME + 1) > 0){
		navigateDirectory(dir, delay, threadpool);
	}

    //aggiungo alla coda dei task i file inseriti singolarmente
	struct stat statbuf;
	for(int index = optind; index < argc && getTermina(&termina) == 0; index++){

		//controllo se è un file regolare
		if((stat(argv[index], &statbuf) == 0) && S_ISREG(statbuf.st_mode)){
			addTask(delay, argv[index], threadpool);    //se lo è lo aggiungo alla coda
		}
		else{
			perror("stat");
		}
	}

    destroyThreadPool(threadpool, 0);   //notifico che i thread dovranno uscire 
    
    int error;
    pthread_cancel(sighandler_thread);
    if((error = (pthread_join(sighandler_thread, NULL))) != 0){     //aspetto la terminazione del signal handler thread
        errno = error;
        perror("pthread_join");
    }
    return 0;
}


//funzione per dire all'utente cosa digitare
void print_usage(const char *programname){
  printf("usage: %s -n <num.thread> -q <qlen> -d <directory name> -t <delay> <file name>\n", programname);
  fflush(stdout);
}


//Thread che gestisce i vari segnali bloccati in maniera sincrona
void *SignalHandler(void *arg){

    //preparo il set dei segnali 
    sigset_t *set = (sigset_t *)arg;

    //finchè non viene richiesta la terminazione
    while (getTermina(&termina) == 0){
        int sig;
        int r = sigwait(set, &sig); //si mette in attesa sincrona sulla sigwait
        if(r != 0){
            errno = r;
            perror("sigwait");
            return NULL;
        }
        
        switch(sig){
        //notifico al Collector di stampare i risultati ricevuti fino a quel momento
        case SIGINT:
        case SIGQUIT:
        case SIGHUP:
        case SIGTERM:
        case SIGUSR1:

            //invio un codice speciale (-1) 
            LOCK(&connessione_mtx);
            int notused;
            int c = -1;
            SYSCALL_EXIT("write MasterWorker", notused, writen(connfd, &c, sizeof(int)), "writen", "")
            UNLOCK(&connessione_mtx);
			LOCK(&termina_mtx);
			termina = 1;
			UNLOCK(&termina_mtx);
            break;
        default:
            break;
        }
    }
    return NULL;
}


//funzione che chiude connessione e aspetta la terminazione del collector
void cleanup(){

    //chiudo connessione e  faccio unlink per rimuovere il file farm.sck
    close(connfd);
    unlink(SOCKNAME);
 
    int status;
    if(waitpid(collectorPid, &status, 0) == -1){ //attendo la terminazione del Collector
        perror("waitpid");
    }
}

//funzione che prende in modo thread safe una copia della variabile termina 
int getTermina(int *termina){
    LOCK(&termina_mtx);
    int tmp = *termina;
    UNLOCK(&termina_mtx);
    return tmp;
}


//funzione che controlla se siamo nel caso di "." o ".."
int isdot(char dir[]){
	int len = strnlen(dir, MAXFILENAME + 1);
	if((len > 0 && dir[len - 1] == '.'))
		return 1;
	return 0;
}


//funzione ricorsiva che naviga le directory ed inserisce i task nel threadpool
void navigateDirectory(char dir[], int delay, threadpool_t *threadpool){

	//apro la directory
	DIR *directory;
	if((directory = opendir(dir)) == NULL){
		perror("opendir");
		return;
	}
	else{
        //navigo la directory stando attento a settare errno prima di fare la chiamata readdir
		struct dirent *file;
		while(getTermina(&termina) == 0 && (errno = 0, file = readdir(directory)) != NULL){ 

			struct stat statbuf;
			char pathname[MAXFILENAME + 1];

			//costruisco una stringa fatta come:
			strncpy(pathname, dir, MAXFILENAME);    //la directory in cui mi trovo,
			strncat(pathname, "/", MAXFILENAME);    // "/" 
			strncat(pathname, file->d_name, MAXFILENAME);   //e il file di cui voglio fare la stat

			//faccio la stat
			if(stat(pathname, &statbuf) == -1){
				perror("stat");
				return;
			}

			//controllo se è una directory 
			if(S_ISDIR(statbuf.st_mode)){ 
				if(!isdot(pathname))   
					navigateDirectory(pathname, delay, threadpool);  //se è una directory richiamo navigateDirectory 
			}
			else{
				addTask(delay, pathname, threadpool); //altrimenti è un file e lo posso passare al threadpool
			}
		}

		//controllo se si sono verificati errori 
		if(errno != 0) perror("readdir");
		
        closedir(directory); //chiudo la directory
	}
}


//funzione che attende un tot di tempo e poi aggiunge il task al threadpool
void addTask(int delay, char pathname[], threadpool_t *threadpool){

	//attendo tot tempo in millisecondi
    struct timespec ts;
    ts.tv_sec  = delay / 1000;
    ts.tv_nsec = (delay % 1000) * 1000000;
    nanosleep(&ts, NULL);
	
    //controllo se è stata richiesta la terminazione
	if(getTermina(&termina) == 0){

		char *filename = malloc((MAXFILENAME + 1) * sizeof(char));
		if(filename == NULL){
			perror("malloc");
		}
		else{
			strncpy(filename, pathname, MAXFILENAME + 1);
			//int r;
			if((/*r =*/ addToThreadPool(threadpool, threadF, (void *)filename)) != 0){  //inserisco la coppia(funzione del worker,file)
				free(pathname);
				perror("threadpool");
                destroyThreadPool(threadpool, 0);
			}
		}
	}
}