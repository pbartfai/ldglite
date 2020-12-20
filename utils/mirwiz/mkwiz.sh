gcc -ggdb -DUSE_OPENGL -DUNIX -I ../../app -c mirwiz.c
gcc -ggdb -o mirwiz mirwiz.o ../../app/L3Math.o ../../app/platform.o -lm
