all:
	gcc -Wall -Wextra -lm -lXpm -lXext -lX11 -lasound wmamixer.c -o wmamixer
	strip wmamixer
dev:
	gcc -ggdb -Wall -Wextra -lm -lXpm -lXext -lX11 -lasound wmamixer.c -o wmamixer
clean:
	rm wmamixer
