#sudo setfont Uni3-Terminus14
CC=gcc -O2 -s -Wall -std=c99
STRIP=strip
LIBS=-lncurses

# I use these to debug
#CC=gcc -g -O0
#STRIP=ls
#LIBS=-lncurses 

all:	clean ansiu

ansiu:	
	$(CC) -o ./ansiu ./ansiu.c $(LIBS)
	$(STRIP) ./ansiu

install:	
	cp ./ansiu /usr/local/bin

clean:
	rm -f ./ansiu
	rm -f *.bmk
