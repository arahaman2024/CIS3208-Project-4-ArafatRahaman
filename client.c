#include<stdio.h>
#include<string.h>	//strlen
#include<stdlib.h>	//strlen
#include<sys/socket.h>
#include<arpa/inet.h>	//inet_addr
#include <unistd.h>
#include <sys/wait.h> 
#include <time.h>
#include <fcntl.h>

// define sizes
#define TOTAL_PRODUCTS 150

// used as a product-pakage
typedef struct {
    int productType;    
    int count;          
} ProducerItem;

void sleep_rand() {
    srand(time(0));
    int rand_num = rand() % 190000 + 10000;
    usleep(rand_num);
}

void producer(int type, int sock_fd) {
    ProducerItem item; 
    for (int i = 0; i < TOTAL_PRODUCTS; i++) {
        item.productType = type;
        item.count = i;
        send(sock_fd, &item, sizeof(ProducerItem), 0);
        sleep_rand();
    }
    // kill switch
    item.productType = type;
    item.count = -1;
    send(sock_fd, &item, sizeof(ProducerItem), 0);
}

int main(int argc , char *argv[]) {

	if (argc != 2) {
        puts("You did not enter a product type number. Exiting Program...");
        exit(0);
    }

    int product_type = atoi(argv[1]);

    if (product_type == 0 || product_type > 2 || product_type < 1) {
        printf("'%s' is not a valid product type. Please enter either product type 1 or product type 2. Exiting Program...\n", argv[1]);
        exit(0);
    }

	int socket_desc;
	struct sockaddr_in server;
	// char *message; 
	
	//Create socket
	socket_desc = socket(AF_INET , SOCK_STREAM , 0);
	if (socket_desc == -1)
	{
		printf("Could not create socket");
	}
		
	server.sin_addr.s_addr = inet_addr("127.0.0.1");
	server.sin_family = AF_INET;
	server.sin_port = htons( 1234 );

	//Connect to remote server
	if (connect(socket_desc , (struct sockaddr *)&server , sizeof(server)) < 0)
	{
		puts("connect error");
		return 1;
	}
	
	puts("Connected\n");
	

	producer(product_type, socket_desc);

	puts("Data Sent.\n");
	

	return 0;
}