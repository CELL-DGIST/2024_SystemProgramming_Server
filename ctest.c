#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <time.h>
#include "server.h"

void pMap(DGIST *dgist) {
    for (int i = 0; i < MAP_ROW; i++) {
        for (int j = 0; j < MAP_COL; j++) {
            printf("%d ", dgist->map[i][j].item.status);
        }
        printf("\n");
    }
}

int main(int argc, char* argv[]) {
    struct sockaddr_in serv_addr;
    int sockfd;
	int PORT = atoi(argv[2]);

    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
	inet_pton(AF_INET, argv[1], &serv_addr.sin_addr);

    // Connect to server
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    // Client action loop
    while (1) {
		srand(time(NULL));
        ClientAction action;
		int tmpRow, tmpCol;
        action.row = rand() % MAP_ROW;
        action.col = rand() % MAP_COL;
        action.action = rand() % 2 == 0 ? move : setBomb;

		if(tmpRow != action.row || tmpCol != action.col){

			printf("Request sended %d, %d\n", action.row, action.col);
			send(sockfd, &action, sizeof(action), 0);
			usleep(500000); // Wait for 1 second

			// Receive DGIST struct from server
			DGIST dgist;
			recv(sockfd, &dgist, sizeof(dgist), 0);

			tmpRow = action.row;
			tmpCol = action.col;
			// Print map information
			printf("Map from server:\n");
			pMap(&dgist);
			printf("\n");

		}
        // Send action to server
    }

    close(sockfd);

    return 0;
}

