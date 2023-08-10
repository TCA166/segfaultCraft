
client: networkingMc.o mcTypes.o client.c
	gcc $(CFLAGS) client.c networkingMc.o mcTypes.o -o client -lz

networkingMc.o: networkingMc.c
	gcc $(CFLAGS) networkingMc.c -o networkingMc.o -c 

mcTypes.o: mcTypes.c
	gcc $(CFLAGS) mcTypes.c -o mcTypes.o -c

client.exe: networkingMc.ow mcTypes.ow client.c
	x86_64-w64-mingw32-gcc-win32 $(CFLAGS) client.c -lws2_32 networkingMc.ow -lws2_32 mcTypes.ow -o client -lz

networkingMc.ow: networkingMc.c
	x86_64-w64-mingw32-gcc-win32 $(CFLAGS) networkingMc.c -o networkingMc.ow -c 

mcTypes.ow: mcTypes.c
	x86_64-w64-mingw32-gcc-win32 $(CFLAGS) mcTypes.c -o mcTypes.ow -c 

clean:
	rm -f *.o
	rm -f *.ow