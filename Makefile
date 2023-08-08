
client: networkingMc.o client.c
	gcc $(CFLAGS) client.c networkingMc.o -o client -lz

networkingMc.o: networkingMc.c
	gcc $(CFLAGS) networkingMc.c -o networkingMc.o -c 

clean:
	rm -f *.o
	rm -f *.ow