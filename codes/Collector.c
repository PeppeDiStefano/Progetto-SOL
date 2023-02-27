#include <util.h>
#include <conn.h>
#include <queue.h>
#include <MasterWorker.h>

//variabili globali che dovranno essere accedibili da cleanup
int sockfd;
NodePtr sPtr = NULL;	//inizialmente non ci sono elementi

//funzione che chiude la connessione e libera la memoria allocata
void cleanupCollector();

//funzione eseguita dal processo Collector
int functionCollector(){

	//uso atexit per poi fare la cleanup
	if(atexit(cleanupCollector) != 0){
		perror("atexit");
		exit(EXIT_FAILURE);
	}

	//creo il socket		
	SYSCALL_EXIT("socket", sockfd, socket(AF_UNIX, SOCK_STREAM, 0), "socket", "");

	//setto l'indirizzo
	struct sockaddr_un serv_addr;	
	memset(&serv_addr, '0', sizeof(serv_addr));	//è buona norma inizializzare queste strutture a 0 per evitare flag/campi settati di default
	serv_addr.sun_family = AF_UNIX;
	strncpy(serv_addr.sun_path, SOCKNAME, strlen(SOCKNAME) + 1);

	//finchè non viene aperta una connessione tento
	while(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1){

		//controllo se il socket non esiste ancora o è stata rifiutata connessione
		if(errno == ENOENT || errno == ECONNREFUSED){ 
			sleep(2);	//attendo 2 secondi prima di ritentare
		}
		else{
			perror("connect");
			exit(EXIT_FAILURE);
		}
	}

	int len;
	int notused;

	//leggo ls lunghezza del nome del file che è stato inviato da MasterWorker
	SYSCALL_EXIT("readn 1 COLLECTOR", notused, readn(sockfd, &len, sizeof(int)), "readn", "");

	//finchè non leggo EOF o una lunghezza non valida
	while(notused > 0 && len != 0){
		if(len == -1){ //se è stato inviato -1
			print(sPtr);	//stampo la coda
		}
		else{
			char file[MAXFILENAME];
			long result;
			SYSCALL_EXIT("readn 2 COLLECTOR", notused, readn(sockfd, file, len * sizeof(char)), "readn", "");	//leggo il nome del file
			SYSCALL_EXIT("readn 3 COLLECTOR", notused, readn(sockfd, &result, sizeof(long)), "readn", "");	//ed il risultato

			insert(&sPtr, result, file);	//inserisco nella coda
		}

		//rileggo la lunghezza del nome del file che è stato inviato da MasterWorker
		SYSCALL_EXIT("readn 1 COLLECTOR", notused, readn(sockfd, &len, sizeof(int)), "readn", "");
	}

	//stampo la coda
	print(sPtr);
	return 0;
}

//funzione che chiude la connessione e libera la memoria allocata
void cleanupCollector(){
	close(sockfd);	//chiudo la conessione
	delete(&sPtr);	//libero la memoria allocata
}