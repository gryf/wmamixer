all:
	gcc wmamixer.c -Wall -Wextra -lm -lXpm -lXext -lX11 -lasound -o wmamixer
	strip wmamixer
clean:
	rm wmamixer
