all:
	gcc -o supervisor supervisor.c -lpthread
	gcc -o pushcmd pushcmd.c
	
clean:
	find ./* -executable | xargs rm
