#include "simpleServer.h"


//An extremely simple server that connects to a given port.
//Once the server is connected to the port, it will listen on that port
//for a user connection.
//A user will connect through telnet, the user also needs to know the port number.
//If the user connects successfully, a socket descriptor will be created.
//The socket descriptor works exactly like a file descriptor, allowing us to read/write on it.
//The server will then use the socket descriptor to communicate with the user, sending and 
//receiving messages.
FILE* dictionaryfile;
FILE* logtext;
char *wordArray[99999];
pthread_cond_t empty, fill = PTHREAD_COND_INITIALIZER;
pthread_cond_t logfill = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t logmutex = PTHREAD_MUTEX_INITIALIZER;

struct Queue* workQ;
struct logQueue* logQ;
int first = 1;


int main(int argc, char** argv)
{


	workQ = malloc(sizeof(struct Queue));	
	initQueue(workQ, 1000);
	logQ = malloc(sizeof(struct Queue));	
	initlogQueue(logQ, 1000);

	dictionaryfile = fopen("dictionary.txt", "r");
	int linenum = 0;
	char line[256];
	//put dictionary into array of string
	while(fgets(line, sizeof(line), dictionaryfile)  != NULL){
		wordArray[linenum] = strdup(line);
	//		printf("%s\n", wordArray[linenum]);
		linenum++;
	
	}

	fclose(dictionaryfile);
	if(argc == 1){
		printf("No port number entered.\n");
		return -1;
	}
	//sockaddr_in holds information about the user connection. 
	//We don't need it, but it needs to be passed into accept().
	struct sockaddr_in client;
	int clientLen = sizeof(client);
	int connectionPort = atoi(argv[1]);
	int connectionSocket, clientSocket, bytesReturned;
	char recvBuffer[BUF_LEN];
	recvBuffer[0] = '\0';

	connectionPort = atoi(argv[1]);

	//We can't use ports below 1024 and ports above 65535 don't exist.
	if(connectionPort < 1024 || connectionPort > 65535){
		printf("Port number is either too low(below 1024), or too high(above 65535).\n");
		return -1;
	}
	//Does all the hard work for us.
	connectionSocket = open_listenfd(connectionPort);
	if(connectionSocket == -1){
		printf("Could not connect to %s, maybe try another port number?\n", argv[1]);
		return -1;
	}

	//accept() waits until a user connects to the server, writing information about that server
	//into the sockaddr_in client.
	//If the connection is successful, we obtain A SECOND socket descriptor. 
	//There are two socket descriptors being used now:
	//One by the server to listen for incoming connections.
	//The second that was just created that will be used to communicate with 
	//the connected user.
	pthread_t work;
	pthread_create(&work, NULL, worker, NULL) ;
	pthread_t work2;
	pthread_create(&work2, NULL, worker, NULL) ;
	pthread_t work3;
	pthread_create(&work3, NULL, worker, NULL) ;
	pthread_t work4;
	pthread_create(&work4, NULL, worker, NULL) ;
	pthread_t work5;
	pthread_create(&work5, NULL, worker, NULL) ;
	pthread_t logger;
	pthread_create(&logger, NULL, logwork, NULL) ;
	while(1){
		if((clientSocket = accept(connectionSocket, (struct sockaddr*)&client, &clientLen)) == -1){// the accept(), returns connected descriptor
			printf("Error connecting to client.\n");
			return -1;
		}
		pthread_mutex_lock(&mutex); 
		while (isFull(workQ)) //while full, wait for empty condition
			pthread_cond_wait(&empty, &mutex); 
		put(clientSocket, workQ); //add conected_socke to work queue
		printQ(workQ);
		pthread_cond_signal(&fill); //signal sleeping workers that there's a  new socket in queue
		pthread_mutex_unlock(&mutex); 
	}
	pthread_join(work, NULL);
	pthread_join(work2, NULL);
	pthread_join(work3, NULL);
	pthread_join(work4, NULL);
	pthread_join(work5, NULL);
	pthread_join(logger, NULL);
	return 0;
}

void put(int socket, struct Queue* workQ) {
	enqueue(socket,workQ);
}

int get(struct Queue* workQ) {
	int tmp = dequeue(workQ);
	return tmp;
 }


 void *logwork(void *arg){
 	while(1){
		pthread_mutex_lock(&logmutex); 
		 while (islogEmpty(logQ)) //while empty, wait for fill condition
			 pthread_cond_wait(&logfill, &logmutex);
		 char* result = dequeuelog(logQ);//remove a socket from the queue
		 //printf("QUEUE AFTER GET:");
		// printQ(workQ);
		 if(first == 1){
		 logtext = fopen("logtext.txt", "w+");
		 first = 0;
		}
		else{
			logtext = fopen("logtext.txt", "a+");
		}
		 fputs(result,logtext); 
		 fclose(logtext);
		 pthread_mutex_unlock(&logmutex);
		
	}

 }

 void *worker(void *arg) { //consumer
 	while(1){
		pthread_mutex_lock(&mutex); 
		 while (isEmpty(workQ)) //while empty, wait for fill condition
			 pthread_cond_wait(&fill, &mutex);
		 int tmpsocket = get(workQ);//remove a socket from the queue
		 pthread_cond_signal(&empty);//notify that there's an empty spot in the queue
		 //service client
		 pthread_mutex_unlock(&mutex);
		 service(wordArray, tmpsocket); 
	}
 }

void service(char *wordArray[], int clientSocket){
	int bytesReturned;
	char recvBuffer[BUF_LEN];
	recvBuffer[0] = '\0';
	printf("Connection success!\n");
	char* clientMessage = "Hello! I hope you can see this.\n";
	char* msgRequest = "Send me some text and I'll tell you if it's spelled correctly.\nSend the escape key to close the connection.\n";
	char* msgResponse = "I actually don't have anything interesting to say...but I know you sent ";
	char* msgOK = " OK\n";
	char* msgMISSPELLED = " MISSPELLED\n";
	char* msgPrompt = ">>>";
	char* msgError = "I didn't get your message. ):\n";
	char* msgClose = "Goodbye!\n";

	//send()...sends a message.
	//We specify the socket we want to send, the message and it's length, the 
	//last parameter are flags.
	send(clientSocket, clientMessage, strlen(clientMessage), 0);
	send(clientSocket, msgRequest, strlen(msgRequest), 0);
	//Begin sending and receiving messages.
	while(1){
		send(clientSocket, msgPrompt, strlen(msgPrompt), 0);
		//recv() will store the message from the user in the buffer, returning
		//how many bytes we received.
		bytesReturned = recv(clientSocket, recvBuffer, BUF_LEN, 0);
		//Check if we got a message, send a message back or quit if the
		//user specified it.
		if(bytesReturned == -1){
			send(clientSocket, msgError, strlen(msgError), 0);
		}
		//'27' is the escape key.
		else if(recvBuffer[0] == 27){

			send(clientSocket, msgClose, strlen(msgClose), 0);
			close(clientSocket);
			break;
		}
		else{
			//if a file is given to get the dictionary from			
			int inDictionary = 0;	
			//make sure received word ends at right place
			int i = 0;	
			while((isalpha(recvBuffer[i]) || recvBuffer[i] == 39)){
				i++;

			}
			recvBuffer[i] = '\0';	
			//if(argc > 1){		
				 //dictionaryfile = fopen(argv[1], "r");
			//}
			//else{
		
			//}
			//open file						   
			for(int j = 0; j < DICTIONARY_SIZE; j++) {


				char *thisword = wordArray[j];
				char *pos;
				if ((pos=strchr(thisword, '\n')) != NULL){
			  	  *pos = '\0';
				}
				//	printf("'%s' and '%s\n", recvBuffer, thisword);
				if(strcmp(recvBuffer, thisword) == 0){
					inDictionary = 1;
					break;
				}
			}
			if(inDictionary == 1){
				//printf("'%s'\n", recvBuffer);
				char *tempMsgOK = strcat(recvBuffer, msgOK);
				//OK
				send(clientSocket, tempMsgOK, strlen(tempMsgOK), 0);

			   	char buf[50];
		    	snprintf(buf, 50, "socket %d: %s",clientSocket, tempMsgOK); // puts string into buffer
		  	
				enqueuelog(buf, logQ);
			}
			else{
				char *tempMsgMIS = strcat(recvBuffer, msgMISSPELLED);
				//MISSPELLED
				send(clientSocket, tempMsgMIS, strlen(tempMsgMIS), 0);
				char buf[50];
		    	snprintf(buf, 50, "socket %d: %s",clientSocket, tempMsgMIS); // puts string into buffer
		  	
		
				enqueuelog(buf, logQ);
			}
			pthread_cond_signal(&logfill);
			//printlogQ(logQ);
			
		}
	}
}




void initQueue(struct Queue* q, int capacity) { 
    q->capacity = capacity; 
    q->first =  0;  
    q->size = 0; 
    q->last = 0; 
    q->store = malloc(q->capacity * sizeof(int)); 
} 

//**FIFO QUEUE
void enqueue(int socket , struct Queue* q){
	if(q->last == q->capacity){
		printf("FIFO queue->size == capacity");
	}
	q->size++;
	q->store[q->last] = socket;
	q->last++;
//	printf("ENQUEUE SOCKET: %d\n", socket);
	
		//printf("ENQUEUE Q->SIZE: %d\n",q->size);
		//printf("ENQUEUE Q->STORE[Q->SIZE]: %d\n",q->store[q->size]);
}

int size(struct Queue* q){
 	return q->last;
}

int dequeue(struct Queue* q){
	if (q->size == 0){
		return -1;
	}
	int socket;
	socket = (q->store[q->first]);
	q->first++;
	q->size--;
	return socket;
}

int isFull(struct Queue *q){
	if (q->size == q->capacity){
		return 1;
	}
	else{
		return 0;
	}
}

int isEmpty(struct Queue *q){
	if(q->size == 0){
		return 1;
	}
	else{
		return 0;
	}
}

void printQ(struct Queue* q){
	printf("\n");
	for(int i = q->first; i < size(q); i++){
		
		printf("%d, ", q->store[i]);
	}
		printf("\n");
}

void initlogQueue(struct logQueue* q, int capacity) { 
    q->capacity = capacity; 
    q->first =  0;  
    q->size = 0; 
    q->last = 0; 
    q->store = malloc(q->capacity * sizeof(char**)); 
} 


void enqueuelog(char *result, struct logQueue* logQ){
	if(logQ->last == logQ->capacity){
		printf("FIFO queue->size == capacity");
	}
	logQ->size++;
	logQ->store[logQ->last] = strdup(result);
	logQ->last++;
}

char* dequeuelog(struct logQueue* logQ){
	if (logQ->size == 0){
		return "WRONG";
	}
	char *result = (logQ->store[logQ->first]);
	logQ->first++;
	logQ->size--;
	return result;
}

int islogEmpty(struct logQueue *q){
	if(q->size == 0){
		return 1;
	}
	else{
		return 0;
	}
}

void printlogQ(struct logQueue* q){

	printf("\n");
	for(int i = q->first; i < logsize(q); i++){
		printf("%s, ", q->store[i]);
	}
		printf("\n");
}

int logsize(struct logQueue* q){
 	return q->last;
}
