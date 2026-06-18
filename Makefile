all:
	gcc -g -Wall linux_mimir.c

clean:
	rm -rf ./a.out ./a.out.dSYM
