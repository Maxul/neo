CC = gcc -g -Wall
CFLAG = -c
LDFLAG = -lX11 -lXcomposite -lXrender -lpthread -lXtst -lXdamage

BIN  = client
OBJ = $(patsubst %.c,%.o,$(wildcard *.c)) 

all : $(BIN)
$(BIN) : ${OBJ}
	$(CC) $^ -o $@ $(LDFLAG) && rm -f *.o *~ *.orig

$(OBJ) : %.o : %.c
	$(CC) $(CFLAG) $< -o $@

clean :
	rm -f $(BIN)

install:
	rm -f /usr/local/bin/$(BIN)
	cp $(BIN) /usr/local/bin/$(BIN)
	chmod +s /usr/local/bin/$(BIN)
