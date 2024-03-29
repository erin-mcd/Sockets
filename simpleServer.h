#ifndef _SIMPLE_SERVER_H
#define _SIMPLE_SERVER_H
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <limits.h>
#include <pthread.h>
#define BUF_LEN 512
#define DICTIONARY_SIZE 99171
#define Q_SIZE 10

struct Queue
{
	// first in first out
	int size;
	int capacity;
	int first;
	int last;
	int* store;
};

struct logQueue
{
	// first in first out
	int size;
	int capacity;
	int first;
	int last;
	char** store;
};

int isFull(struct Queue *q);
void *logwork(void *arg);
int open_listenfd(int);
void put(int socket, struct Queue* workQ);
int get(struct Queue* workQ);
void *worker(void *arg);
void initQueue(struct Queue* q, int capacity);
void enqueue(int socket , struct Queue* q);
int size(struct Queue* q);
int dequeue(struct Queue* q);
int isEmpty(struct Queue *q);
void service(char *wordArray[], int clientSocket);
void printQ(struct Queue* q);
void enqueuelog(char *result, struct logQueue* logQ);
char* dequeuelog(struct logQueue* q);
int islogEmpty(struct logQueue *q);
void initlogQueue(struct logQueue* q, int capacity);
void printlogQ(struct logQueue* q);
int logsize(struct logQueue* q);

#endif
