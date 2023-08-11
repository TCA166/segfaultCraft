
client: networkingMc.o mcTypes.o gamestateComplete.o client.c
	gcc $(CFLAGS) client.c networkingMc.o mcTypes.o gamestateComplete.o -o client -lz

networkingMc.o: networkingMc.c
	gcc $(CFLAGS) networkingMc.c -o networkingMc.o -c 

mcTypes.o: mcTypes.c
	gcc $(CFLAGS) mcTypes.c -o mcTypes.o -c

gamestateComplete.o: gamestateMc.c cNBT.o
	gcc $(CFLAGS) gamestateMc.c -o gamestateMc.o -c
	ld -relocatable gamestateMc.o cNBT.o -o gamestateComplete.o

cNBT.o: cNBT/buffer.c cNBT/nbt_parsing.c cNBT/nbt_treeops.c cNBT/nbt_util.c
	gcc cNBT/buffer.c -o cNBT/buffer.o -c $(CFLAGS)
	gcc cNBT/nbt_parsing.c -o cNBT/nbt_parsing.o -c $(CFLAGS)
	gcc cNBT/nbt_treeops.c -o cNBT/nbt_treeops.o -c $(CFLAGS)
	gcc cNBT/nbt_util.c -o cNBT/nbt_util.o -c $(CFLAGS)
	ld -relocatable cNBT/buffer.o cNBT/nbt_parsing.o cNBT/nbt_treeops.o cNBT/nbt_util.o -o cNBT.o

client.exe: networkingMc.ow mcTypes.ow client.c
	x86_64-w64-mingw32-gcc-win32 $(CFLAGS) client.c -lws2_32 networkingMc.ow -lws2_32 mcTypes.ow -o client -lz -lbcrypt

networkingMc.ow: networkingMc.c
	x86_64-w64-mingw32-gcc-win32 $(CFLAGS) networkingMc.c -o networkingMc.ow -c 

mcTypes.ow: mcTypes.c
	x86_64-w64-mingw32-gcc-win32 $(CFLAGS) mcTypes.c -o mcTypes.ow -c 

clean:
	rm -f *.o
	rm -f *.ow