all: phistogram thistogram


phistogram: syn_phistogram.c
	gcc syn_phistogram.c -o syn_phistogram -lpthread -lrt


thistogram: syn_thistogram.c
	gcc syn_thistogram.c -lpthread -lrt -o syn_thistogram 
