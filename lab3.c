#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/wait.h> 
#include <time.h>

// define sizes
#define TOTAL_PRODUCTS 150
#define PRODUCT_1_BUFFER_SIZE 10
#define PRODUCT_2_BUFFER_SIZE 15

// used as a product-pakage
typedef struct {
    int productType;    
    int count;          
} ProducerItem;

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

// used in producer function to sleep between 0.01 to 0.20
void sleep_rand() {
    srand(time(0));
    int rand_num = rand() % 190000 + 10000;
    usleep(rand_num);
}

// global set so consumers can have access to the output file discriptor to write() to.
int fdOut;

void logItem(const char *logInfo) {
    write(fdOut, logInfo, strlen(logInfo));
    fsync(fdOut);
}

// used to produce products
void producer(int type, int fd) {
    ProducerItem item; 
    for (int i = 0; i < TOTAL_PRODUCTS; i++) {
        item.productType = type;
        item.count = i;
        write(fd, &item, sizeof(ProducerItem));
        sleep_rand();
    }
    // kill switch
    item.productType = type;
    item.count = -1;
    write(fd, &item, sizeof(ProducerItem));
}


void *consumer1(void *arg) {
    
    // indefinite while loop
    while(1) {
        
        // lock buffer
        pthread_mutex_lock(&buffer1_mutex);

        // if between locking the buffer and the cks1 was changed we return null
        if (cks1 == 1) {
            return NULL;
        }

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
        printf("Type[1], Product[%d], ThreadiD <%lu> --- consumer1_count[%d]\n", temp, thread_id, consumer1_count);
        
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
        if (cks2 == 1) {
            return NULL;
        }

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

        printf("Type[2], Product[%d], ThreadiD <%lu> --- consumer2_count[%d]\n", temp, thread_id, consumer2_count);

        char logMessage[4096];
        snprintf(logMessage, sizeof(logMessage), "Type[2], Product[%d], ThreadiD <%lu> --- consumer2_count[%d]\n", temp, thread_id, consumer2_count);
        logItem(logMessage);

        pthread_cond_signal(&buffer2_empty);
        pthread_mutex_unlock(&buffer2_mutex);
    }
    
    return NULL;
}




int main(int argc, char *argv[]) {

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

    // producer code
    // pipe used for the children proccess to send the product after it has been created to the parent for distribution
    int fd[2];
    pipe(fd);

    // used to call waitpid after all this
    int pid1;
    int pid2;
    
    if ((pid1 = fork()) < 0) {
        perror("1st fork failed.\n");
        exit(1);
    } else if(pid1 == 0) {
        producer(1, fd[1]);
        exit(0);
    }

    if ((pid2 = fork()) < 0) {
        perror("2nd fork failed.\n");
        exit(1);
    } else if(pid2 == 0) {
        producer(2, fd[1]);
        exit(0);
    }
    

    // distributor code
    ProducerItem item;
    int ks1 = 0;
    int ks2 = 0;
    // int i = 0;
    while(ks1 == 0 || ks2 == 0 ) {
        
        // keep reading as long as both kill switch hasnt been passed
        read(fd[0], &item, sizeof(ProducerItem));
        
        // acknowledge kS has been passed
        if (item.productType == 1 && item.count == -1) {
            ks1 = 1;
        }

        if (item.productType == 2 && item.count == -1) {
            ks2 = 1;
        }

        // printf("productType = [%d] and count = [%d]\n", item.productType, item.count); 
        
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

    // wait for proccess to finish
    int status;
    waitpid(pid1, &status, 0);
    waitpid(pid2, &status, 0);

    // call join, allowing for the threads to finish and return
    pthread_join(consumer_1_1, NULL);
    pthread_join(consumer_2_1, NULL);
    pthread_join(consumer_1_2, NULL);
    pthread_join(consumer_2_2, NULL);

    // can close the file discriptor at this point
    close(fdOut);

    // print done to the terminal allowing us to know when it's all done.
    printf("done\n");

    return 0;
}