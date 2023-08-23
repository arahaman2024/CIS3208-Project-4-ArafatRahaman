server: server.o
	gcc -o server server.o -Wall -Werror -lpthread

client: client.o
	gcc -o client client.o -Wall -Werror

server.o: server.c
	gcc -c server.c -Wall -Werror -lpthread

client.o: client.c
	gcc -c client.c -Wall -Werror

clean:
	rm -rf *.o