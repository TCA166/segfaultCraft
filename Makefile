
client: networkingMc.o mcTypes.o gamestateComplete.o client.c
	gcc $(CFLAGS) client.c networkingMc.o mcTypes.o gamestateComplete.o -o client -lz

networkingMc.o: networkingMc.c
	gcc $(CFLAGS) networkingMc.c -o networkingMc.o -c 

mcTypes.o: mcTypes.c
	gcc $(CFLAGS) mcTypes.c -o mcTypes.o -c

gamestateComplete.o: gamestateMc.c cNBT.o list.o
	gcc $(CFLAGS) gamestateMc.c -o gamestateMc.o -c
	ld -relocatable gamestateMc.o cNBT.o list.o -o gamestateComplete.o

list.o: list.c
	gcc $(CFLAGS) list.c -o list.o -c

cNBT.o: cNBT/buffer.c cNBT/nbt_parsing.c cNBT/nbt_treeops.c cNBT/nbt_util.c
	gcc cNBT/buffer.c -o cNBT/buffer.o -c $(CFLAGS)
	gcc cNBT/nbt_parsing.c -o cNBT/nbt_parsing.o -c $(CFLAGS)
	gcc cNBT/nbt_treeops.c -o cNBT/nbt_treeops.o -c $(CFLAGS)
	gcc cNBT/nbt_util.c -o cNBT/nbt_util.o -c $(CFLAGS)
	ld -relocatable cNBT/buffer.o cNBT/nbt_parsing.o cNBT/nbt_treeops.o cNBT/nbt_util.o -o cNBT.o

client.exe: networkingMc.ow mcTypes.ow client.c gamestateComplete.ow
	x86_64-w64-mingw32-gcc-win32 $(CFLAGS) client.c -lws2_32 networkingMc.ow -lws2_32 mcTypes.ow gamestateComplete.ow -o client -lz -lbcrypt

networkingMc.ow: networkingMc.c
	x86_64-w64-mingw32-gcc-win32 $(CFLAGS) networkingMc.c -o networkingMc.ow -c 

mcTypes.ow: mcTypes.c
	x86_64-w64-mingw32-gcc-win32 $(CFLAGS) mcTypes.c -o mcTypes.ow -c 

gamestateComplete.ow: gamestateMc.c cNBT.ow list.ow
	x86_64-w64-mingw32-gcc-win32 $(CFLAGS) gamestateMc.c -o gamestateMc.ow -c
	x86_64-w64-mingw32-ld -relocatable gamestateMc.ow cNBT.ow list.ow -o gamestateComplete.ow

cNBT.ow: cNBT/buffer.c cNBT/nbt_parsing.c cNBT/nbt_treeops.c cNBT/nbt_util.c
	x86_64-w64-mingw32-gcc-win32 cNBT/buffer.c -o cNBT/buffer.ow -c $(CFLAGS)
	x86_64-w64-mingw32-gcc-win32 cNBT/nbt_parsing.c -o cNBT/nbt_parsing.ow -c $(CFLAGS)
	x86_64-w64-mingw32-gcc-win32 cNBT/nbt_treeops.c -o cNBT/nbt_treeops.ow -c $(CFLAGS)
	x86_64-w64-mingw32-gcc-win32 cNBT/nbt_util.c -o cNBT/nbt_util.ow -c $(CFLAGS)
	x86_64-w64-mingw32-ld -relocatable cNBT/buffer.ow cNBT/nbt_parsing.ow cNBT/nbt_treeops.ow cNBT/nbt_util.ow -o cNBT.ow

list.ow: list.c
	x86_64-w64-mingw32-gcc-win32 $(CFLAGS) list.c -o list.ow -c

clean:
	rm -rf *.o
	rm -rf *.ow