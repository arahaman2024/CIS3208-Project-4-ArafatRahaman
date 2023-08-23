#include <stdio.h>
#include <string.h>	//strlen
#include <stdlib.h>	//strlen
#include <sys/socket.h>
#include <arpa/inet.h>	//inet_addr
#include <unistd.h>	//write
#include <sys/wait.h> 
#include <time.h>
#include <fcntl.h>
#include <pthread.h> //for threading , link with lpthread


// define sizes
#define TOTAL_PRODUCTS 150
#define PRODUCT_1_BUFFER_SIZE 10
#define PRODUCT_2_BUFFER_SIZE 15

// struct to recieve
typedef struct {
    int productType;    
    int count;          
} ProducerItem;

int fdOut;

void logItem(const char *logInfo) {
    write(fdOut, logInfo, strlen(logInfo));
    fsync(fdOut);
}

int rp1 = 0;
int rp2 = 0;

// Used to messaage terminal
int ks1p = 0;
int ks2p = 0;

// used in consumer to let other thread know when the first thread recieves the killSwitch
int cks1 = 0;
int cks2 = 0;

// used to keep track of product. It could be initilized to 0, but initialized it to -1 because when first product is printed it will have
// product number 0, and the consumer count will also be 0;
int consumer1_count = -1;
int consumer2_count = -1;


// buffers to store products
ProducerItem buffer1[PRODUCT_1_BUFFER_SIZE];
ProducerItem buffer2[PRODUCT_2_BUFFER_SIZE];


int buffer1_count = 0;      // total items in buffer - used to determine if buffer is either full or empty
int buffer1_use_ptr = 0;    // used in consumer thread to determine which index of buffer to consume
int buffer1_fill_ptr = 0;   // used in distributor thread to determine which index of buffer to store product

int buffer2_count = 0;      // total items in buffer - used to determine if buffer is either full or empty
int buffer2_use_ptr = 0;    // used in consumer thread to determine which index of buffer to consume
int buffer2_fill_ptr = 0;   // used in distributor thread to determine which index of buffer to store product


// locks for buffers. Mutual excelusion.
pthread_mutex_t buffer1_mutex;
pthread_mutex_t buffer2_mutex;

// conidition variables. Need to wait and signal distributor and consumers so they know when to wake up, consume, or distribute
pthread_cond_t buffer1_empty;
pthread_cond_t buffer1_fill;
pthread_cond_t buffer2_empty;
pthread_cond_t buffer2_fill;

void *connection_handler(void *);

void *consumer1(void *arg) {
    
    // indefinite while loop
    while(1) {
        
        // lock buffer
        pthread_mutex_lock(&buffer1_mutex);

        // while empty we wait BUT ONLY IF cks1 == 0. If the kill switch was passed then return null.
        while(buffer1_count == 0) {
            if (cks1 == 0) {
                pthread_cond_wait(&buffer1_fill, &buffer1_mutex);
            }else{
                return NULL;
            }
        }

        // create an instance fo the ProducerItem
        ProducerItem item;
        item = buffer1[buffer1_use_ptr];
        int temp = item.count;

        if (temp == -1) {
            cks1 = 1;
            pthread_mutex_unlock(&buffer1_mutex);
            pthread_cond_signal(&buffer1_fill);
            return NULL;
        }

        // takes care of a circular queue. we move the consumerr_use pointer over 1 and if its at the end of the buffer move it to the front again. decrease the buffer count too
        consumer1_count++;
        buffer1_use_ptr = (buffer1_use_ptr + 1) % PRODUCT_1_BUFFER_SIZE;
        buffer1_count--;

        // get the thread iD
        pthread_t thread_id = pthread_self();

        // print and log
        // printf("Type[1], Product[%d], ThreadiD <%lu> --- consumer1_count[%d]\n", temp, thread_id, consumer1_count);
        
        char logMessage[4096];
        snprintf(logMessage, sizeof(logMessage), "Type[1], Product[%d], ThreadiD <%lu> --- consumer1_count[%d]\n", temp, thread_id, consumer1_count);
        logItem(logMessage);

        // signal there has been a consumption (empty) and unlock buffer
        pthread_cond_signal(&buffer1_empty);
        pthread_mutex_unlock(&buffer1_mutex);
    }
    
    // never gets here
    return NULL;
}


void *consumer2(void *arg) {
    
    while (1) {

        pthread_mutex_lock(&buffer2_mutex);

        while(buffer2_count == 0) {
            if (cks2 == 0) {
                pthread_cond_wait(&buffer2_fill, &buffer2_mutex);
            }else{
                return NULL;
            }
        }

        ProducerItem item;
        item = buffer2[buffer2_use_ptr];
        int temp = item.count;

        if (temp == -1) {
            cks2 = 1;
            pthread_mutex_unlock(&buffer2_mutex);
            pthread_cond_signal(&buffer2_fill);
            return NULL;
        }

        consumer2_count++;
        buffer2_use_ptr = (buffer2_use_ptr + 1) % PRODUCT_2_BUFFER_SIZE;
        buffer2_count--;

        pthread_t thread_id = pthread_self();

        // printf("Type[2], Product[%d], ThreadiD <%lu> --- consumer2_count[%d]\n", temp, thread_id, consumer2_count);

        char logMessage[4096];
        snprintf(logMessage, sizeof(logMessage), "Type[2], Product[%d], ThreadiD <%lu> --- consumer2_count[%d]\n", temp, thread_id, consumer2_count);
        logItem(logMessage);

        pthread_cond_signal(&buffer2_empty);
        pthread_mutex_unlock(&buffer2_mutex);
    }
    
    return NULL;
}

int main(int argc , char *argv[]) {

	// open or create if not already existing. Flags passed to clear the file first and then go on about logging
    fdOut = open("out.txt", O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

    // create thread id storage
    pthread_t consumer_1_1;
    pthread_t consumer_1_2;
    pthread_t consumer_2_1;
    pthread_t consumer_2_2;

    // create and tell the threads to run the consumer programs
    pthread_create(&consumer_1_1, NULL, consumer1, NULL);
    pthread_create(&consumer_1_2, NULL, consumer1, NULL);
    pthread_create(&consumer_2_1, NULL, consumer2, NULL);
    pthread_create(&consumer_2_2, NULL, consumer2, NULL);

	int socket_desc , new_socket , c , *new_sock;
	struct sockaddr_in server , client;
	char *message;
	
	//Create socket
	socket_desc = socket(AF_INET , SOCK_STREAM , 0);
	if (socket_desc == -1)
	{
		printf("Could not create socket");
	}
	
	//Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons( 1234 );
	
	//Bind
	if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
	{
		puts("bind failed");
		return 1;
	}
	puts("bind done");
	
	//Listen
	listen(socket_desc , 3);
	
	//Accept and incoming connection
	puts("Waiting for incoming connections...");
	c = sizeof(struct sockaddr_in);

	while( (new_socket = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) )
	{
		puts("Connection accepted");
		
		//Reply to the client
		message = "Hello Client , I have received your connection. And now I will assign a handler for you\n";
		write(new_socket , message , strlen(message));
		
		pthread_t sniffer_thread;
		new_sock = malloc(1);
		*new_sock = new_socket;
		
		if( pthread_create( &sniffer_thread , NULL ,  connection_handler , (void*) new_sock) < 0)
		{
			perror("could not create thread");
			return 1;
		}
		
        puts("Handler assigned");
		// Now join the thread , so that we dont terminate before the thread
		// pthread_join( sniffer_thread , NULL);
	}
	
	if (new_socket<0)
	{
		perror("accept failed");
		return 1;
	}

	// call join, allowing for the threads to finish and return
    pthread_join(consumer_1_1, NULL);
    pthread_join(consumer_2_1, NULL);
    pthread_join(consumer_1_2, NULL);
    pthread_join(consumer_2_2, NULL);

    // can close the file discriptor at this point
    close(fdOut);
	
	return 0;
}

/*
 * This will handle connection for each client
 * 
 */
 
 // distributor thread stuff
void *connection_handler(void *socket_desc)
{
	//Get the socket descriptor
	int sock = *(int*)socket_desc;
	int read_size;

    ProducerItem item;
	while( (read_size = recv(sock , &item , sizeof(ProducerItem), 0)) > 0 )
	{

        // essentially deals with if another connection 1 or 2 comes in after the 1st product_type_1 and product_type_2s
        if (item.productType == 1 && item.count == 0 && rp1 == 0) {
            rp1 = 1;
        }else if(item.productType == 1 && item.count == 0 && rp1 == 1) {
            printf("Client sent in product 1 again. Will not process...\n");
            return 0;
        }

        if (item.productType == 2 && item.count == 0 && rp2 == 0) {
            rp2 = 1;
        }else if(item.productType == 2 && item.count == 0 && rp2 == 1) {
            printf("Client sent in product 2 again. Will not process...\n");
            return 0;
        }

        printf("Recieved: item.productType[%d], item.count[%d].\n", item.productType, item.count);
		

		if (item.productType == 1 && item.count == -1) {
            ks1p = 1;
        }

        if (item.productType == 2 && item.count == -1) {
            ks2p = 1;
        }

        

		// distributor code;
        if (item.productType == 1) {
            pthread_mutex_lock(&buffer1_mutex);                                         // lock buffer 1
            while(buffer1_count == PRODUCT_1_BUFFER_SIZE) {                             // check if buffer if full
                pthread_cond_wait(&buffer1_empty, &buffer1_mutex);                      // if so, WAIT till empty signal
            }
            buffer1[buffer1_fill_ptr] = item;                                     // place item at the index of fill_ptr
            buffer1_fill_ptr = (buffer1_fill_ptr + 1) % PRODUCT_1_BUFFER_SIZE;          // move the fill_ptr
            buffer1_count++;                                                            // increment count
            pthread_cond_signal(&buffer1_fill);                                         // signal a fill
            pthread_mutex_unlock(&buffer1_mutex);                                       // unlock critical section
        }

        if (item.productType == 2) {
            pthread_mutex_lock(&buffer2_mutex);                                         // lock buffer 2
            while(buffer2_count == PRODUCT_2_BUFFER_SIZE) {                             // check if buffer if full
                pthread_cond_wait(&buffer2_empty, &buffer2_mutex);                      // if so, WAIT until empty signal
            }
            buffer2[buffer2_fill_ptr] = item;                                     // place item at the index of fill_ptr
            buffer2_fill_ptr = (buffer2_fill_ptr + 1) % PRODUCT_2_BUFFER_SIZE;          // move the fill_ptr
            buffer2_count++;                                                            // increment count
            pthread_cond_signal(&buffer2_fill);                                         // signal a fill
            pthread_mutex_unlock(&buffer2_mutex);                                       // unclock critical section
        }
	}
	
	if(read_size == 0)
	{
		puts("Client disconnected");
		fflush(stdout);
	}
	else if(read_size == -1 && (ks1p == 1 || ks2p == 1))
	{
		printf("Kill Switch Passed [%d]. Done Listening.\n", item.productType);
	}
    else if(read_size == -1) {
        perror("rec failed");
    }
		
	//Free the socket pointer
	free(socket_desc);
	
	return 0;
}