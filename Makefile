
client: networkingMc.o client.c
	gcc $(CFLAGS) client.c networkingMc.o -o client -lz

networkingMc.o: networkingMc.c
	gcc $(CFLAGS) networkingMc.c -o networkingMc.o -c 

client.exe: networkingMc.ow client.c
	x86_64-w64-mingw32-gcc-win32 $(CFLAGS) client.c -lws2_32 networkingMc.ow -lws2_32 -o client -lz

networkingMc.ow: networkingMc.c
	x86_64-w64-mingw32-gcc-win32 $(CFLAGS) networkingMc.c -o networkingMc.ow -c 

clean:
	rm -f *.o
	rm -f *.ow