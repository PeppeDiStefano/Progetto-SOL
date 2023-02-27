#ifndef QUEUE_H_
#define QUEUE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct node {

	long result;
	char file[256];
	struct node *next;

} Node;

typedef Node *NodePtr;

//funzione che inserisce un elemento in testa alla coda e in maniera ordinata
void insert(NodePtr *sPtr, long n, char*file);

//funzione che restituisce 1 se la coda è vuota, altrimento 0
int isEmpty(NodePtr sPtr);

//funzione che stampa gli elementi della coda
void print(NodePtr sPtr);

//funzione che libera la memoria allocata
void delete(NodePtr *sPtr);

#endif