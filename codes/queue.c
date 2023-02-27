#include <queue.h>

//funzione che inserisce un elemento in testa alla coda e in maniera ordinata
void insert(NodePtr *sPtr, long result, char *file){

	//creo il nodo
	NodePtr newPtr = malloc(sizeof(Node));

	if(newPtr != NULL){	//se c'è spazio disponibile
		
		//riempimento del nuovo nodo 
		newPtr->result = result;
		strncpy(newPtr->file, file, 256);
		newPtr->next = NULL;	//il nuovo nodo non è collegato ad altri nodi

		NodePtr previous = NULL;
		NodePtr current = *sPtr;
		//ripeto il ciclo per trovare la posizione corretta 
		while(current != NULL && result > current->result){
			previous = current;	//vado avanti...
			current = current->next;	//...al nodo successivo
		}

		//inserisco il nuovo nodo in testa alla coda
		if(previous == NULL){
			newPtr->next = *sPtr;
			*sPtr = newPtr;
		}
		else{	//inserisco il nuovo nodo tra previous e current
			previous->next = newPtr;
			newPtr->next = current;
		}		
	}
	else{
		// Stampa per segnalare l'errore 
		printf("Error: Problems during memory allocation\n");
	}
}

//funzione che restituisce 1 se la coda è vuota, altrimento 0
int isEmpty(NodePtr sPtr){
	return sPtr == NULL;
}

//funzione che stampa gli elementi della coda
void print(NodePtr sPtr){
	if(isEmpty(sPtr)){	//se la coda è vuota
		printf("Coda vuota");
	}
	else{
		while(sPtr != NULL){	//finchè non si raggiunge la fine della lista
			printf("%ld %s\n", sPtr->result, sPtr->file);
			sPtr = sPtr->next;
		}
	}
}

//funzione che libera la memoria allocata
void delete(NodePtr *sPtr){
	NodePtr tmpPtr = NULL;
	while(*sPtr != NULL){
		tmpPtr = *sPtr;
		*sPtr = (*sPtr)->next;
		free(tmpPtr);
	}
}