server: queue.h queue.c http.h http.c util.h server.c
	gcc -pthread -o server queue.c http.c server.c

clean:
	rm -f server *~