#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include <MasterWorker.h>

//funzione eseguita dal Worker del threadpool
void threadF(void *filename){

	char *file = (char *)filename;
	FILE *fd = NULL;

	//apro il file binario in lettura
	if((fd = fopen(file, "rb")) == NULL){
		perror("fopen");
	}
	else{

		long i = 0;
		long result = 0;
		unsigned char buffer[8];
		while(fread(buffer, sizeof(buffer), 1, fd) == 1){	//finchè ci sono long da leggere
			//calcolo del risultato
			long number = *((long *)buffer);
			result = (i++) * number + result;
		}
		fclose(fd);

		//acquisico lock per evitare che altri Worker usino la connessione
		LOCK(&connessione_mtx);
		int len = strnlen(file, MAXFILENAME) + 1;

		if(writen(connfd, &len, sizeof(int)) == -1){
			UNLOCK(&connessione_mtx);	//rilascio la lock
			free(file);
			LOCK(&termina_mtx);
			termina = 1;	//modifico termina
			UNLOCK(&termina_mtx);
		}

		if(writen(connfd, file, len * sizeof(char)) == -1){
			UNLOCK(&connessione_mtx);	//rilascio la lock
			free(file);
			LOCK(&termina_mtx);
			termina = 1;	//modifico termina
			UNLOCK(&termina_mtx);
		}

		if(writen(connfd, &result, sizeof(long)) == -1){
			UNLOCK(&connessione_mtx);	//rilascio la lock
			free(file);
			LOCK(&termina_mtx);
			termina = 1;	//modifico termina
			UNLOCK(&termina_mtx);
		}

		//rilascio la lock 
		UNLOCK(&connessione_mtx);
		free(file);
	}
}