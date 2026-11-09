// Aurora Matamoros B84707
#include "pgm.h"
#include <pthread.h>

#define REPETICIONES 5

// Filtro blur
float blur[3][3] = {
    {1.0f/9.0f, 1.0f/9.0f, 1.0f/9.0f},
    {1.0f/9.0f, 1.0f/9.0f, 1.0f/9.0f},
    {1.0f/9.0f, 1.0f/9.0f, 1.0f/9.0f}
};

// Filtro gauss
float gauss[5][5] = {
    {1.0f/273.0f, 4.0f/273.0f, 7.0f/273.0f, 4.0f/273.0f, 1.0f/273.0f},
    {4.0f/273.0f, 16.0f/273.0f, 26.0f/273.0f, 16.0f/273.0f, 4.0f/273.0f},
    {7.0f/273.0f, 26.0f/273.0f, 41.0f/273.0f, 26.0f/273.0f, 7.0f/273.0f},
    {4.0f/273.0f, 16.0f/273.0f, 26.0f/273.0f, 16.0f/273.0f, 4.0f/273.0f},
    {1.0f/273.0f, 4.0f/273.0f, 7.0f/273.0f, 4.0f/273.0f, 1.0f/273.0f}
};

// Filtro sobel
float sobel[3][3] = {
    {-1.0f, 0.0f, 1.0f},
    {-2.0f, 0.0f, 2.0f},
    {-1.0f, 0.0f, 1.0f}
};

typedef struct {
    const unsigned char *entrada;
    unsigned char *salida;
    int ancho;
    int alto;
    int y_inicio;
    int y_fin;
    const float *filtro;
    int tam;
} DatosHilo; // v2

typedef struct {
    const unsigned char *entrada;
    unsigned char *salida;
    int ancho;
    int alto;
    int y_inicio;
    int y_fin;
    const float *filtro;
    int tam;
    float *sumas;
} DatosHiloV3; 

// Convolucion secuencial V0
void convolucion_v0(
    const unsigned char *entrada,
    unsigned char *salida,
    int ancho,
    int alto,
    const float *filtro,
    int tam
) {
    int radio = tam / 2;

    for (int y = 0; y < alto; y++) {
        for (int x = 0; x < ancho; x++) {

            float suma = 0.0f;

            for (int i = -radio; i <= radio; i++) {
                for (int j = -radio; j <= radio; j++) {

                    int f = y + i;
                    int c = x + j;

                    // Correccion de bordes
                    if (f < 0) f = 0;
                    if (f >= alto) f = alto - 1;

                    if (c < 0) c = 0;
                    if (c >= ancho) c = ancho - 1;

                    int indice = f * ancho + c;

                    int fi = (i + radio) * tam + (j + radio);

                    suma += filtro[fi] * entrada[indice];
                }
            }

            // Valor absoluto
            if (suma < 0.0f) {
                suma = -suma;
            }

            // Saturacion
            if (suma > 255.0f) {
                suma = 255.0f;
            }

            // Redondeo
            salida[y * ancho + x] = (unsigned char)(suma + 0.5f);
        }
    }
}

void convolucion_v1(const unsigned char *entrada, unsigned char *salida,
                    int ancho, int alto, const float *filtro, int tam,
                    float *sumas) {

    int radio = tam / 2;

    for (int y = 0; y < alto; y++) {

        for (int x = 0; x < ancho; x++) {
            sumas[x] = 0.0f;
        }

        for (int i = -radio; i <= radio; i++) {

            int f = y + i;

            if (f < 0) f = 0;
            if (f >= alto) f = alto - 1;

            for (int j = -radio; j <= radio; j++) {

                int fi = (i + radio) * tam + (j + radio);
                float coef = filtro[fi];

                for (int x = 0; x < ancho; x++) {

                    int c = x + j;

                    if (c < 0) c = 0;
                    if (c >= ancho) c = ancho - 1;

                    sumas[x] += coef * entrada[f * ancho + c];
                }
            }
        }

        for (int x = 0; x < ancho; x++) {

            float suma = sumas[x];

            if (suma < 0) suma = -suma;
            if (suma > 255) suma = 255;

            salida[y * ancho + x] = (unsigned char)(suma + 0.5f);
        }
    }
}


void *trabajar_v2(void *arg) {

    DatosHilo *datos = (DatosHilo *)arg;

    int radio = datos->tam / 2;

    for (int y = datos->y_inicio; y < datos->y_fin; y++) {

        for (int x = 0; x < datos->ancho; x++) {

            float suma = 0.0f;

            for (int i = -radio; i <= radio; i++) {
                for (int j = -radio; j <= radio; j++) {

                    int f = y + i;
                    int c = x + j;

                    if (f < 0) f = 0;
                    if (f >= datos->alto) f = datos->alto - 1;

                    if (c < 0) c = 0;
                    if (c >= datos->ancho) c = datos->ancho - 1;

                    int indice = f * datos->ancho + c;
                    int fi = (i + radio) * datos->tam + (j + radio);

                    suma += datos->filtro[fi] * datos->entrada[indice];
                }
            }

            if (suma < 0) suma = -suma;
            if (suma > 255) suma = 255;

            datos->salida[y * datos->ancho + x] =
                (unsigned char)(suma + 0.5f);
        }
    }

    return NULL;
}

int convolucion_v2(const unsigned char *entrada, unsigned char *salida,int ancho, int alto, const float *filtro, int tam,int num_hilos) {

    pthread_t *hilos = malloc(num_hilos * sizeof(pthread_t));
    DatosHilo *datos = malloc(num_hilos * sizeof(DatosHilo));

    if (hilos == NULL || datos == NULL) {
        free(hilos);
        free(datos);
        return -1;
    }

    int creados = 0;

    
    for (int t = 0; t < num_hilos; t++) {

        datos[t].entrada = entrada;
        datos[t].salida = salida;
        datos[t].ancho = ancho;
        datos[t].alto = alto;
        datos[t].y_inicio = t * alto / num_hilos;
        datos[t].y_fin = (t + 1) * alto / num_hilos;
        datos[t].filtro = filtro;
        datos[t].tam = tam;

        if (pthread_create(&hilos[t], NULL, trabajar_v2, &datos[t]) != 0) {
            printf("Error al crear un hilo.\n");
            break;
        }

        creados++;
    }

    for (int t = 0; t < creados; t++) {
        pthread_join(hilos[t], NULL);
    }

    free(hilos);
    free(datos);

    if (creados != num_hilos) {
        printf("Error: el procesamiento de V2 incompleto.");
        return -1;
    }
        return 0; 

} 


void *trabajar_v3(void *arg) {

    DatosHiloV3 *datos = (DatosHiloV3 *)arg;

    int radio = datos->tam / 2;

    for (int y = datos->y_inicio; y < datos->y_fin; y++) {

        for (int x = 0; x < datos->ancho; x++) {
            datos->sumas[x] = 0.0f;
        }

        for (int i = -radio; i <= radio; i++) {

            int f = y + i;

            if (f < 0) f = 0;
            if (f >= datos->alto) f = datos->alto - 1;

            for (int j = -radio; j <= radio; j++) {

                int fi = (i + radio) * datos->tam + (j + radio);
                float coef = datos->filtro[fi];

                for (int x = 0; x < datos->ancho; x++) {

                    int c = x + j;

                    if (c < 0) c = 0;
                    if (c >= datos->ancho) c = datos->ancho - 1;

                    datos->sumas[x] += coef *
                        datos->entrada[f * datos->ancho + c];
                }
            }
        }

        for (int x = 0; x < datos->ancho; x++) {

            float suma = datos->sumas[x];

            if (suma < 0) suma = -suma;
            if (suma > 255) suma = 255;

            datos->salida[y * datos->ancho + x] =
                (unsigned char)(suma + 0.5f);
        }
    }

    return NULL;
}

int convolucion_v3(const unsigned char *entrada, unsigned char *salida,int ancho, int alto, const float *filtro, int tam,int num_hilos) {

    if (num_hilos <= 0) {
        return -1;
    }

    pthread_t *hilos = malloc(num_hilos * sizeof(pthread_t));
    DatosHiloV3 *datos = malloc(num_hilos * sizeof(DatosHiloV3));
    float *sumas = malloc((size_t)num_hilos * ancho * sizeof(float));

    if (hilos == NULL || datos == NULL || sumas == NULL) {
        free(hilos);
        free(datos);
        free(sumas);
        return -1;
    }

    int creados = 0;

    for (int t = 0; t < num_hilos; t++) {

        datos[t].entrada = entrada;
        datos[t].salida = salida;
        datos[t].ancho = ancho;
        datos[t].alto = alto;
        datos[t].y_inicio = t * alto / num_hilos;
        datos[t].y_fin = (t + 1) * alto / num_hilos;
        datos[t].filtro = filtro;
        datos[t].tam = tam;
        datos[t].sumas = &sumas[(size_t)t * ancho];

        if (pthread_create(&hilos[t], NULL, trabajar_v3, &datos[t]) != 0) {
            printf("Error al crear un hilo.\n");
            break;
        }

        creados++;
    }

    for (int t = 0; t < creados; t++) {
        pthread_join(hilos[t], NULL);
    }

    free(hilos);
    free(datos);
    free(sumas);

    if (creados != num_hilos) {
            printf("Error: el procesamiento de V3 incompleto.");
            return -1;
        }

        return 0;
}

int main(int argc, char *argv[]) {

    if (argc != 2) {
        printf("Uso: %s imagen.pgm\n", argv[0]);
        return 1;
    }

    int ancho, alto;

    // Leer imagen
    unsigned char *entrada = leer_pgm(argv[1], &ancho, &alto);

    // Reservar memoria
    size_t n = (size_t)ancho * alto;

    unsigned char *salida = malloc(n);
    float *sumas = malloc((size_t)ancho * sizeof(float));

    if (salida == NULL || sumas == NULL) {
        printf("Error al reservar memoria.\n");
        free(entrada);
        free(salida);
        free(sumas);
        return 1;
    }

    double tiempos[REPETICIONES];

    // Ejecutar los tres filtros
    const char *nombres[] = {"blur", "gauss", "sobel"};
    const float *filtros[] = {
        &blur[0][0],
        &gauss[0][0],
        &sobel[0][0]
    };
    int tamanos[] = {3, 5, 3};

    for (int k = 0; k < 3; k++) {

        printf("\nFiltro: %s\n", nombres[k]);

        for (int r = 0; r < REPETICIONES; r++) {

            double inicio = ahora();

            convolucion_v0(entrada,salida,ancho,alto,filtros[k],tamanos[k]);
            //convolucion_v1(entrada, salida, ancho, alto, filtros[k], tamanos[k], sumas);
            //if (convolucion_v2(entrada, salida, ancho, alto,filtros[k], tamanos[k], 8) != 0) {// el valor despues de tamaño sn los hilos

            //    printf("No se pudo completar la ejecucion de V2.\n");

            //    free(entrada);
             //   free(salida);
            //   free(sumas);

             //   return 1;}
            //if (convolucion_v3(entrada, salida, ancho, alto,filtros[k], tamanos[k], 8) != 0) {// el valor despues de tamaño sn los hilos

            //    printf("No se pudo completar la ejecucion de V3.\n");

            //    free(entrada);
            //    free(salida);
            //    free(sumas);

            //    return 1;}



            tiempos[r] = ahora() - inicio;
        }

        // Calcular checksum
        uint64_t suma = checksum(salida, n);

        // Calcular mediana
        double tiempo = mediana(tiempos, REPETICIONES);

        printf("Checksum: %" PRIu64 "\n", suma);
        printf("Mediana: %.6f segundos\n", tiempo);

        // Guardar imagen
        char nombre[100];
        snprintf(nombre, sizeof(nombre), "%s_v0.pgm", nombres[k]);
        //snprintf(nombre, sizeof(nombre), "%s_v1.pgm", nombres[k]);
        //snprintf(nombre, sizeof(nombre), "%s_v2.pgm", nombres[k]);
        //snprintf(nombre, sizeof(nombre), "%s_v3.pgm", nombres[k]);

        escribir_pgm(nombre, salida, ancho, alto);
    }

    free(entrada);
    free(salida);
    free(sumas);

    return 0;
}
