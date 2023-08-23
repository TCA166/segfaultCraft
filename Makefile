
client: segfaultCraft.o cJSON.o client.c
	gcc $(CFLAGS) client.c segfaultCraft.o cJSON.o -o client -lz

segfaultCraft.o: networkingMc.o mcTypes.o gamestateMc.o list.o cNBT.o
	ld -relocatable networkingMc.o mcTypes.o gamestateMc.o list.o cNBT.o -o segfaultCraft.o

networkingMc.o: networkingMc.c
	gcc $(CFLAGS) networkingMc.c -o networkingMc.o -c 

mcTypes.o: mcTypes.c
	gcc $(CFLAGS) mcTypes.c -o mcTypes.o -c

gamestateMc.o: gamestateMc.c 
	gcc $(CFLAGS) gamestateMc.c -o gamestateMc.o -c

list.o: list.c
	gcc $(CFLAGS) list.c -o list.o -c

cNBT.o: cNBT/buffer.c cNBT/nbt_parsing.c cNBT/nbt_treeops.c cNBT/nbt_util.c
	gcc cNBT/buffer.c -o cNBT/buffer.o -c $(CFLAGS)
	gcc cNBT/nbt_parsing.c -o cNBT/nbt_parsing.o -c $(CFLAGS)
	gcc cNBT/nbt_treeops.c -o cNBT/nbt_treeops.o -c $(CFLAGS)
	gcc cNBT/nbt_util.c -o cNBT/nbt_util.o -c $(CFLAGS)
	ld -relocatable cNBT/buffer.o cNBT/nbt_parsing.o cNBT/nbt_treeops.o cNBT/nbt_util.o -o cNBT.o

cJSON.o: cJSON/cJSON.c
	gcc cJSON/cJSON.c -o cJSON.o -c $(CFLAGS)

client.exe: client.c segfaultCraft.ow cJSON.ow
	x86_64-w64-mingw32-gcc-win32 $(CFLAGS) client.c -lws2_32 segfaultCraft.ow -lws2_32 cJSON.ow -o client -lz -lbcrypt

segfaultCraft.ow: networkingMc.ow mcTypes.ow gamestateMc.ow list.ow cNBT.ow
	x86_64-w64-mingw32-ld -relocatable networkingMc.ow mcTypes.ow gamestateMc.ow cNBT.ow list.ow -o segfaultCraft.ow

networkingMc.ow: networkingMc.c
	x86_64-w64-mingw32-gcc-win32 $(CFLAGS) networkingMc.c -o networkingMc.ow -c 

mcTypes.ow: mcTypes.c
	x86_64-w64-mingw32-gcc-win32 $(CFLAGS) mcTypes.c -o mcTypes.ow -c 

gamestateMc.ow: gamestateMc.c
	x86_64-w64-mingw32-gcc-win32 $(CFLAGS) gamestateMc.c -o gamestateMc.ow -c

cNBT.ow: cNBT/buffer.c cNBT/nbt_parsing.c cNBT/nbt_treeops.c cNBT/nbt_util.c
	x86_64-w64-mingw32-gcc-win32 cNBT/buffer.c -o cNBT/buffer.ow -c $(CFLAGS)
	x86_64-w64-mingw32-gcc-win32 cNBT/nbt_parsing.c -o cNBT/nbt_parsing.ow -c $(CFLAGS)
	x86_64-w64-mingw32-gcc-win32 cNBT/nbt_treeops.c -o cNBT/nbt_treeops.ow -c $(CFLAGS)
	x86_64-w64-mingw32-gcc-win32 cNBT/nbt_util.c -o cNBT/nbt_util.ow -c $(CFLAGS)
	x86_64-w64-mingw32-ld -relocatable cNBT/buffer.ow cNBT/nbt_parsing.ow cNBT/nbt_treeops.ow cNBT/nbt_util.ow -o cNBT.ow

cJSON.ow: cJSON/cJSON.c
	x86_64-w64-mingw32-gcc-win32 cJSON/cJSON.c -o cJSON.ow -c $(CFLAGS)

list.ow: list.c
	x86_64-w64-mingw32-gcc-win32 $(CFLAGS) list.c -o list.ow -c

clean:
	rm -rf *.o
	rm -rf *.ow