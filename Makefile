all:
	gcc -std=c99 -g -pedantic -Wall -Wextra -Werror murmur_callbacks.c -o murmur_callbacks
clean:
	rm -r murmur_callbacks
