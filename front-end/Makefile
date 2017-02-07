CC = clang -g -Wall -O0
CFLAG = -c
LDFLAG = -lX11 -lXcomposite -lXrender -lpthread

BIN  = frontend
OBJ = $(patsubst %.c,%.o,$(wildcard *.c)) 

all : $(BIN)
$(BIN) : ${OBJ}
	$(CC) $^ -o $@ $(LDFLAG) && rm -f *.o *~

$(OBJ) : %.o : %.c
	$(CC) $(CFLAG) $< -o $@

clean :
	-rm $(BIN)

install:
	-rm /usr/local/bin/$(BIN)
	cp $(BIN) /usr/local/bin/$(BIN)
	chmod +s /usr/local/bin/$(BIN)
