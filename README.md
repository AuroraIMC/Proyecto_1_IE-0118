Proyecto 1 de Arquitecturas paralelas y compiladores 
Elaborado por : Aurora Isabel Matamoros Cuadra B84707
Enlace a git:(https://github.com/AuroraIMC/Proyecto_1_IE-0118)

v0
gcc -std=c11 -O2 -fno-tree-vectorize -ffp-contract=off proyecto.c -o proyecto.exe

v1
gcc -std=c11 -O3 -march=native -ffp-contract=off -pthread proyecto.c -o proyecto.exe

v2
gcc -std=c11 -O3 -march=native -ffp-contract=off -pthread proyecto.c -o proyecto.exe

v3
gcc -std=c11 -O3 -march=native -ffp-contract=off  -pthread proyecto.c -o proyecto.exe


para las pruebas de las imagenes 

./proyecto.exe img2048.pgm
./proyecto.exe img4096.pgm

para las versiones v2 y v3 hay que cambiar los hilos manualmente.