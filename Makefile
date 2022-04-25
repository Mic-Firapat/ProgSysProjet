#ALIAS

CC = gcc
OPTIONS = -g -W -Wall -pedantic -o3
EXEC = initial chefs ouvrier client
TOCLEAN = *.o *~
CLES = cle.file.cc cle.file.co


#MAIN
all: $(EXEC)
	make clean


#REGLES CONSTRUCTION
initial: initial.o
	$(CC) $(OPTIONS) $^ -o $@

chefs: chefs.o
	$(CC) $(OPTIONS) $^ -o $@

ouvrier: ouvrier.o
	$(CC) $(OPTIONS) $^ -o $@

client: client.o
	$(CC) $(OPTIONS) $^ -o $@

#DEPENDANCE

%.o: %.c
	$(CC) $(OPTIONS) -c $< -o $@



#UTILITAIRE :

.PHONY:clean
clean:
	rm -f $(TOCLEAN)

clean_all: clean
	rm -f $(EXEC) $(CLES)
