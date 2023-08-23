#include<stdio.h>
#include<string.h>	//strlen
#include<stdlib.h>	//strlen
#include<sys/socket.h>
#include<arpa/inet.h>	//inet_addr
#include <unistd.h>
#include <sys/wait.h> 
#include <time.h>
#include <fcntl.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        puts("You did not enter a product type number. Exiting Program...");
        exit(0);
    }

    int product_type = atoi(argv[1]);

    if (product_type == 0) {
        printf("'%s' is not a valid product type. Exiting Program...\n", argv[1]);
        exit(0);
    }

    printf("Product type = %d\n", product_type);

    return 0;
}