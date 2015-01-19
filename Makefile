all:
	gcc -Wall -Wextra -lXpm -lXext -lX11 -lasound wmamixer.c -o wmamixer


