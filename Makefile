all: rbc evc

rbc: rbc.c rbc.h
	gcc -o rbc rbc.c

evc: evc.c
	gcc -o evc evc.c

clean:
	rm -f evc rbc
