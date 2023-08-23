/*

Arafat Rahaman
client-server-producer-consumer
DOCUMENTATION

*/

[GitHub]
    github: https://github.com/CIS-3207-S23/project-4-s23-arafatrahaman1-3

[Context]

    This project extends Project 3 by introducing communication between producers and consumers across a network. In project 3, the producers and consumers were on the same computer system. In this project the use of the pipe for communication is removed and communication through an internet socket is used. The purpose of the assignment is to gain some exposure to writing programs that communicate across networks.

[CLIENT/PRODUCER_SIDE]

    How to compile:
        gcc -o client client.c -Wall -Werror
    How to run:
        ./client [1 or 2] &
        You have to pass an argument when running, and it only accepts 1 or 2 as those are the specified product-types in the lab

    On the client side, you have the producer. The client sends products over to a server via the network for processeing. The client creates a TCP connection. If a connection is successful, it will then attempt to send products over via a socket. To send these products via socket a struct is used, and this struct is avialable to both sides (client and server). The struct contains a product_type (1 or 2), and a product_number (0-149 150 products + -1 a kill switch).

    For this assignment, you run this client twice, once with the argument 1 and another time argument 2 which represents two product types. The server, per run, will only accept one of each. Depending on which is sent it will then begin consumption. For the function that is used for the producer it is the following:

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
    
    The function is passed the product_type, and the socket_descriptor to which it can write to over the socket via send().

[SERVER/CONSUMER_SIDE]
    How to compile:
        gcc -o server server.c -Wall -Werror -lpthread
    How to run:
        ./client
    Note:
        You must close/kill this program with ctr+C or it will continue to listen for incoming connections.
    
    On the server side you have dispatcher/connection-handling/distrubutor threads(per connection, 2), consumer threads (4), and the main thread for listening (1). Total of 7 threads. You also have buffers (used to store items), mutexes, condition variables, and other stuff...

    main thread for listening (1)
        The main thread is responsible for opening and listening for a TCP connection. It is also repsonisble for creating the 4 consumer threads, and the two dispatcher/distributor threads. When the client side connects via the network, we are sure that it has either passed a products type 1 or 2. When this is passed, the main thread creates two connection-handler/distributor threads which deal with the connections. 
    dispatcher/connection-handling/distrubutor threads(per connection, 2)
        void *connection_handler(void *socket_desc)
        This thread is passed into the pointer to a socket_descriptor which it will use to read from using recieve(). Here the products are "proccessed" and placed into the consumer buffers for concumption. This thead stays open until the kill switch has been passed.
    consumer threads:
        Very similar to lab 3 consumer threads. Actually it's the same thing. The consumer threads uses the consumer[no.] functions. The function runs indefineitly for respective products. Two threads are using each function and does the following. It locks the buffer, if the buffer is empty it waits for the signal that there has been a fill for the respective buffer. Once there is a product they wake up and consume that product. And keep consuming as long as they can keep gaining access to the buffer and there is stuff to consume. The complicated part is the killSwitch. So two threads use one consumer function. What happens when the killSwitch is passed and 1 of the 2 consumer gains access to it first? Well the other one is left sleeping. To fix this, what we do is simply put an if-statement in the while-loop conddition for if there is a product in the buffer. When one of the threads gains the kill switch it, returns NULL, wakes up the other thread using the same function with and unlock to the buffer, and a signal that there has been a fill (there hasn't been a fill but is needed to wake and stop having the other thread sleeping). How the consumption works is it uses a indexing-pointer-number to keep track of what to eat next. When it is consumed the log function is called to have the information about the consumption be printed to an out.txt file. It prints product type, product number, the consumer thread id, and the consumer count for that respective product.

[makefile]
    - make server: compiles server
    - make client: compiles client

[FUNCTIONS]
    void sleep_rand() - used to put the producer to sleep at random times.
    void logItem(const char *logInfo) - used to log info to out.txt file
    void producer(int type, int socket_fd) - used to create products, and send through a TCP socket
    void *consumer1(void *arg) - consume product1
    void *consumer2(void *arg) - consume product2

[LOCKS_AND_CONDITIONS]
    Locks
        pthread_mutex_t buffer1_mutex; - used to lock buffer1 up so that producer, and consumer 1 and consumer 2 don't all have access to it at the same time
        pthread_mutex_t buffer2_mutex; - used to lock buffer2 up so that producer, and consumer 1 and consumer 2 don't all have access to it at the same time

    Conditions
        pthread_cond_t buffer1_empty; - need an empty wait/signal so distribors and consumers know when to wait and go
        pthread_cond_t buffer1_fill; - need a fill ait/signal so distribors and consumers know when to wait and go
        pthread_cond_t buffer2_empty; - need an empty wait/signal so distribors and consumers know when to wait and go
        pthread_cond_t buffer2_fill;  - need a fill ait/signal so distribors and consumers know when to wait and go

[MISCELANEOUS]
    int fdOut - used to point to the out file.
    rp1 & rp2 - used to know if product 1 or 2 was recieved. Used to reject, if another client tries to a pass a product already recieved
    ks1 & ks2 - used in distributor to know when to stop reading from socket.
    cks1 & cks2 - used to let other thread (the one that did not get the killSwitch) to stop.

[TESTING]
    - Testing was done as the code was being built. Before the implementation of the outfile, everything was getting printed to the terminal.
    - A way to check is also just to compare the product number to the consumer count, if they match in the logs then everything synced up
    - Tried different numbers for buffer size, producer size, etc.
    - Made sure there were 4 unique threadIDs, and 2 thread IDs only consumed product 1, and the other two consumed product 2
    - Kept printing everything to the teriminal in the final implementation.
    - Used print statements to keep track of where the kill switch product was at all times

[FINAL_NOTES]
    This was literally like ripping out the pipe from the old lab and putting in a socket. Making me feel like a mechanic. One of the requirements of this lab was to test the connection through a network (like from different nodes in the server) but we were not able to do that, and professor took that away as a test requirment. Thus, all we were responsible for was to test it on one node.

[REFERENCES]
    Binary Tides Link
    Chapter 30 OSTEP (GOAT Textbook)
    Ryan-TA (GOAT TA) - helped with a bug



    
    

    


