pingencoder : 
	gcc -Wall -Wextra -pedantic -Werror ping.c -o bin/pingencoder -Os

clean :
	rm bin/*
