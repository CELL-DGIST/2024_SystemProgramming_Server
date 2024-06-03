#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <semaphore.h>
#include "server.h"

sem_t mapLock;

static inline int makeRandomInt(int max, int min) {
    return min + rand() % (max - min + 1);
}

// Thread entry function to handle each client
void* handleClient(void *arg) {

	tmpDGIST* tmpDG = (tmpDGIST*)arg;
    DGIST* dgist = tmpDG->dgist;
    client_info* client;
    int client_socket;
    pthread_t tid;
	int pId = tmpDG->cIndex;

	client = &(dgist->players[pId]);

    client_socket = client->socket;
    ClientAction cAction;

    int row, col, valRead;

    while (1) {

        if ((valRead = read(client_socket, &cAction, sizeof(ClientAction))) == 0) {
            printf("Client disconnected, socket fd is %d\n", client_socket);
            close(client_socket);
            pthread_exit(NULL);
        }

        row = cAction.row;
        col = cAction.col;

		if(row != client->row || col != client->col){
			sem_wait(&mapLock);
			if(row >= 0 && row < MAP_ROW && col >= 0 && col < MAP_COL){
				switch ((dgist->map[row][col]).item.status) {
					case nothing:
						printf("Player%d reach (%d, %d)\n", pId+1, row, col);
						client->row = row;
						client->col = col;
						break;
					case item:
						printf("Player%d reach (%d, %d) and get %dpoints\n", pId+1, row, col,(dgist->map[row][col]).item.score);
						client->score += (dgist->map[row][col]).item.score;
						(dgist->map[row][col]).item.status = nothing;
						client->row = row;
						client->col = col;
						break;
					case trap:
						printf("Player%d reach (%d, %d) and get trap\n", pId+1, row, col);
						client->score -= SCORE_DEDUCTION;
						(dgist->map[row][col]).item.status = nothing;
						client->row = row;
						client->col = col;
						break;
				}

				if (client->bomb > 0 && cAction.action == setBomb){
					(dgist->map[row][col]).item.status = trap;
					client->bomb -= 1;
				} 
			}
			sem_post(&mapLock);
			pthread_create(&tid, NULL, broadcastInformation, (void *)dgist);
			pthread_join(tid, NULL);
			printMap(dgist);
			printPlayer(dgist);
		}

    }

    return NULL;
}

int setItem(DGIST* dPtr) {

    int row, col, score;

    sem_wait(&mapLock);

    do {
        row = makeRandomInt(MAP_ROW - 1, 0);
        col = makeRandomInt(MAP_COL - 1, 0);
    } while (dPtr->map[row][col].item.status != nothing);

    score = makeRandomInt(MAX_SCORE, 1);

	if(((row-col == 0) && (row == 4 || row == 0))){
		sem_post(&mapLock);
		return 0;
	} 

    dPtr->map[row][col].item.status = item;
    dPtr->map[row][col].item.score = score;

    sem_post(&mapLock);
	printMap(dPtr);
	return 1;
}

// Broadcast map information to all clients
void* broadcastInformation(void* arg) {

    DGIST* dgist = (DGIST*)arg;

    client_info client;
    int client_socket;

    sem_wait(&mapLock);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        client = (dgist->players)[i];
		if(client.socket != -1){
			client_socket = client.socket;
			send(client_socket, dgist, sizeof(DGIST), 0);
		}
    }
    sem_post(&mapLock);

    return NULL;
}

void* handleItem(void* arg) {

    DGIST* dgist = (DGIST*)arg;
    pthread_t tid;
    while (1) {
        sleep(SETTING_PERIOD);
        setItem(dgist);
        pthread_create(&tid, NULL, broadcastInformation, (void *)dgist);
        pthread_join(tid, NULL);
    }
}

// Print the map
void* printMap(void* arg) {

    DGIST* dgist = (DGIST*)arg;
    Item tmpItem;

	printf("==========PRINT MAP==========\n");
    sem_wait(&mapLock);
	for (int i = 0; i < MAP_ROW; i++) {
		for (int j = 0; j < MAP_COL; j++) {
            tmpItem = (dgist->map[i][j]).item;
            switch (tmpItem.status) {
                case nothing:
                    printf("- ");
                    break;
                case item:
                    printf("%d ", tmpItem.score);
                    break;
                case trap:
                    printf("x ");
                    break;
            }
        }
        printf("\n");
    }
    sem_post(&mapLock);
	printf("==========PRINT DONE==========\n");

    return NULL;
}

void printPlayer(void* arg){

    DGIST* dgist = (DGIST*)arg;
	client_info client;
	printf("==========PRINT PLYAERS==========\n");
	for(int i=0; i < MAX_CLIENTS; i++){
		client = dgist->players[i];
		printf("++++++++++Player %d++++++++++\n",i+1);
		printf("Location: (%d,%d)\n",client.row, client.col);
		printf("Score: %d\n",client.score);
		printf("Bomb: %d\n",client.bomb);
	}
	printf("==========PRINT DONE==========\n");
}

int main(int argc, char *argv[]) {

    int server_fd;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    pthread_t tid;
    int numClient = 0;
	
	srand(time(NULL));

	if (argc != 2) {
        fprintf(stderr, "Usage: %s <number>\n", argv[0]);
        return 1;
    }

	const int PORT = atoi(argv[1]);

    // Initialize semaphore
    sem_init(&mapLock, 0, 1);

    // Racing map + player information
    DGIST dgist;

    // Initialize map
    sem_wait(&mapLock);
    for (int i = 0; i < MAP_ROW; i++) {
        for (int j = 0; j < MAP_COL; j++) {
            (dgist.map[i][j]).row = i;
            (dgist.map[i][j]).col = j;
            (dgist.map[i][j]).item.status = nothing;
        }
    }
    sem_post(&mapLock);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        dgist.players[i].socket = -1;
    }

    // Place items in the map
	int k = 0;

	while(k < INITIAL_ITEM){
		k += setItem(&dgist);
	}

    printf("SERVER DATA INITIALIZING COMPLETE\n");

    // Setup socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
	
	printf("Server IP address:%s\n", inet_ntoa(address.sin_addr));

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);

    // Start item handling thread
    pthread_create(&tid, NULL, handleItem, (void *)&dgist);

	Dictionary* clientDict = create_dictionary();

    // Get clients
    while (1) {
        int new_socket;
		int ipLast;
		int cIndex;
        client_info newClient;

        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        printf("New connection, socket fd is %d, ip is : %s, port : %d\n",
            new_socket,
            inet_ntoa(address.sin_addr),
            ntohs(address.sin_port)
        );

		ipLast = get_last_part_as_int(inet_ntoa(address.sin_addr));
		cIndex = dictionary_get(clientDict, ipLast);

		if(cIndex != -1){ //client exist
			dgist.players[cIndex].socket = new_socket;;
			newClient.address = address;
		}else{//client does not exist
			cIndex = numClient;
			dictionary_add(clientDict, ipLast, cIndex);
			newClient.socket = new_socket;
			newClient.address = address;
			newClient.score = 0;
			if(cIndex == 0){
				newClient.row = 0;
				newClient.col = 0;
			}
			else{
				newClient.row = 4;
				newClient.col = 4;
			}
			newClient.bomb = INITIAL_BOMB;
			dgist.players[cIndex] = newClient;
			numClient++;
		} 
		

		if(numClient > MAX_CLIENTS){
            perror("MAX CLIENT EXCCEDED");
            exit(EXIT_FAILURE);
		}


	printPlayer(&dgist);
	tmpDGIST tDG;
	tDG.dgist = &dgist;
	tDG.cIndex = cIndex;
        pthread_create(&tid, NULL, handleClient, (void *)&tDG);
    }

    return 0;
}

