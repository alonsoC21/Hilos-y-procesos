//Equipo doritos nacho 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <time.h>

#define NUM_PROCESOS 3
#define NUM_HILOS 2
#define SIZE 15

int array[SIZE];      // Array aleatorio
int objetivo;         // Número a buscar
int *encontrado;      // Variable en memoria compartida
pthread_mutex_t lock; // Mutex para proteger la variable 'encontrado'

// Función que ejecutará cada hilo para la búsqueda binaria en su sub-arreglo
void* busqueda_binaria_hilo(void* arg) {
    int* datos = (int*) arg;
    int inicio = datos[0];
    int fin = datos[1];

    while (inicio <= fin) {
        int medio = inicio + (fin - inicio) / 2;

        pthread_mutex_lock(&lock);
        if (*encontrado != -1) {  // Si ya se encontró el número en otro hilo
            pthread_mutex_unlock(&lock);
            pthread_exit(NULL);
        }
        pthread_mutex_unlock(&lock);

        if (array[medio] == objetivo) {
            pthread_mutex_lock(&lock);
            *encontrado = medio;  // Se encontró el número
            pthread_mutex_unlock(&lock);
            pthread_exit(NULL);
        }

        if (array[medio] < objetivo) {
            inicio = medio + 1;
        } else {
            fin = medio - 1;
        }
    }

    pthread_exit(NULL);
}

// Función para generar un arreglo aleatorio ordenado
void generar_array_aleatorio() {
    srand(time(NULL));  // Inicializar semilla para números aleatorios
    array[0] = rand() % 10;  // Primer número aleatorio

    // Generar el resto de los números de manera ordenada
    for (int i = 1; i < SIZE; i++) {
        array[i] = array[i - 1] + (rand() % 10 + 1);
    }
}

int main() {
    // Crear memoria compartida para la variable 'encontrado'
    encontrado = mmap(NULL, sizeof(*encontrado), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (encontrado == MAP_FAILED) {
        perror("Error creando memoria compartida");
        exit(EXIT_FAILURE);
    }
    *encontrado = -1;  // Inicializar como "no encontrado"

    pthread_mutex_init(&lock, NULL);  // Inicializar el mutex

    generar_array_aleatorio();  // Generar el array aleatorio

    // Mostrar el array generado
    printf("Array generado: ");
    for (int i = 0; i < SIZE; i++) {
        printf("%d ", array[i]);
    }
    printf("\n");

    // Pedir al usuario que ingrese el número a buscar
    printf("Introduce el número a buscar: ");
    scanf("%d", &objetivo);

    // Crear procesos
    pid_t pids[NUM_PROCESOS];
    int segmento = SIZE / NUM_PROCESOS;

    for (int i = 0; i < NUM_PROCESOS; i++) {
        pids[i] = fork();

        if (pids[i] == 0) {  // Código del proceso hijo
            printf("Proceso hijo %d iniciado con PID %d\n", i, getpid());

            pthread_t hilos[NUM_HILOS];
            int hilo_data[NUM_HILOS][2];  // Almacenar el rango de búsqueda de cada hilo

            int inicio = i * segmento;
            int fin = (i == NUM_PROCESOS - 1) ? SIZE - 1 : (inicio + segmento - 1);
            int sub_segmento = (fin - inicio + 1) / NUM_HILOS;

            for (int j = 0; j < NUM_HILOS; j++) {
                hilo_data[j][0] = inicio + j * sub_segmento;
                hilo_data[j][1] = (j == NUM_HILOS - 1) ? fin : (hilo_data[j][0] + sub_segmento - 1);

                if (pthread_create(&hilos[j], NULL, busqueda_binaria_hilo, &hilo_data[j]) != 0) {
                    perror("Error creando hilo");
                    exit(EXIT_FAILURE);
                }
            }

            // Esperar a que los hilos terminen
            for (int j = 0; j < NUM_HILOS; j++) {
                pthread_join(hilos[j], NULL);
            }

            printf("Proceso %d terminando.\n", getpid());
            exit(0);  // Termina el proceso hijo
        } else if (pids[i] < 0) {  // Error al crear el proceso
            perror("Error creando proceso");
            exit(EXIT_FAILURE);
        }
    }

    // Esperar a que todos los procesos hijos terminen
    for (int i = 0; i < NUM_PROCESOS; i++) {
        wait(NULL);
    }

    if (*encontrado != -1) {
        printf("El objetivo %d fue encontrado en la posición %d.\n", objetivo, *encontrado);
    } else {
        printf("El objetivo %d no fue encontrado.\n", objetivo);
    }

    pthread_mutex_destroy(&lock);  // Destruir el mutex
    munmap(encontrado, sizeof(*encontrado));  // Liberar la memoria compartida

    return 0;
}