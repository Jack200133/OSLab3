/** ---------------------------------------------------------
 * UNIVERSIDAD DEL VALLE DE GUATEMALA
 * CC3064   -   Sistemas Operativos
 * Profesor:    Sebastian Galindo
 * Author:      Juan Angel Carrera
 * Date:        12 / 03 / 2023
 *
 * ---------------------------------------------------------
 * SudokuValidator.c
 * Laboratorio # 3
 * Realizar:
 * ---------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <stdbool.h>

#include <stdlib.h>
#include <sys/wait.h>
#include <omp.h>

// Tamaño del sudoku
#define SIZE 9

// Matriz que representa el sudoku
int sudoku[SIZE][SIZE];
bool acepted;

// Función que revisa las filas del sudoku
bool check_rows()
{
    int i, j;
    bool response;
    int nums[SIZE] = {0};
    response = true;
    // paralel para revisar las filas
    omp_set_num_threads(9);
    omp_set_nested(true);
#pragma omp parallel for private(i, nums) schedule(dynamic)
    for (i = 0; i < SIZE; i++)
    {
        // Hace un reset de los numeros
        int nums[SIZE] = {0};
#pragma omp parallel for private(j, nums)
        for (j = 0; j < SIZE; j++)
        {
            // Obtiene el numero de la fila
            int num = sudoku[i][j];
            // Si el numero ya existe en la fila, entonces el sudoku no es valido
            if (nums[num - 1] == 1)
            {
                response = false;
            }
            nums[num - 1] = 1;
        }
    }
    return response;
}

// Función que revisa las columnas del sudoku
void *check_columns(void *arg)
{
    int i, j;
    // paralel para revisar las columnas
    omp_set_num_threads(9);
    omp_set_nested(true);
    int nums[SIZE] = {0};
#pragma omp parallel for private(i, nums) schedule(dynamic)
    for (i = 0; i < SIZE; i++)
    {
        // Hace un reset de los numeros
        int nums[SIZE] = {0};
        // Obtiene el ID del thread y lo muestra
        int tID = syscall(SYS_gettid);
        printf("Columna %d revisanda por el Thread %d\n", i, tID);
#pragma omp parallel for private(j, nums)
        for (j = 0; j < SIZE; j++)
        {
            // Obtiene el numero de la columna
            int num = sudoku[j][i];
            // Si el numero ya existe en la columna, entonces el sudoku no es valido
            if (nums[num - 1] == 1)
            {
                acepted = false;
                pthread_exit(NULL);
            }
            nums[num - 1] = 1;
        }
    }
    acepted = true;
    pthread_exit(NULL);
}
// Función que revisa los subarreglos de 3x3
bool check_subgrid(void *arg)
{
    int row_start = ((int *)arg)[0];
    int col_start = ((int *)arg)[1];
    int nums[SIZE] = {0};
    int i, j;
    bool response = true;
    // paralel para revisar los subarreglos
    omp_set_num_threads(3);
    omp_set_nested(true);
#pragma omp parallel for private(j, nums) schedule(dynamic)
    for (i = row_start; i < row_start + 3; i++)
    {
        // Hace un reset de los numeros
        int nums[SIZE] = {0};
#pragma omp parallel for private(nums) schedule(dynamic)
        for (j = col_start; j < col_start + 3; j++)
        {
            // Obtiene el numero del subarreglo
            int num = sudoku[i][j];
            // Si el numero ya existe en el subarreglo, entonces el sudoku no es valido
            if (nums[num - 1] == 1)
            {
                response = false;
            }
            nums[num - 1] = 1;
        }
    }
    return response;
}
// Función principal
int main(int argc, char *argv[])
{
    char *path = argv[1];
    // Abrir el archivo del sudoku
    int sudoku_file = open(path, O_RDONLY);
    acepted = true;
    if (sudoku_file == -1)
    {
        printf("Error al abrir el archivo");
        return 1;
    }
    // Mapear el archivo del sudoku
    char *sudoku_str = mmap(NULL, SIZE * SIZE + 1, PROT_READ, MAP_PRIVATE, sudoku_file, 0);
    if (sudoku_str == MAP_FAILED)
    {
        printf("Error al mapear el archivo");
        return 1;
    }
    // meter los valores del sudoku en la matriz
    for (int i = 0; i < 9; i++)
    {
        for (int j = 0; j < 9; j++)
        {
            sudoku[i][j] = sudoku_str[i * 9 + j] - '0';
        }
    }
    // Variables para guardar el resultado de las funciones
    bool nicesubway;
    bool niceRows;
    int i, j, k;
    // Revisar subarreglos de 3x3
    for (int i = 0; i < 9; i += 3)
    {
        for (int j = 0; j < 9; j += 3)
        {
            int list[2];
            list[0] = i;
            list[1] = j;
            nicesubway = check_subgrid(list);
            // Si el subarreglo no es valido, entonces el sudoku no es valido
            if (!nicesubway)
            {
                break;
            }
        }
        if (!nicesubway)
        {
            break;
        }
    }
    // Se muestra el PID del proceso
    pid_t pid = getpid();
    printf("PID: %d\n", pid);

    // Se crea el thread para revisar las columnas
    pthread_t threadColumn;
    int f = fork();
    char pidS[10];
    sprintf(pidS, "%d", pid);

    if (f == 0)
    {
        // Se muestra el PID del proceso padre
        printf("ID del PAPA: %s\n", pidS);
        execlp("ps", "ps", "-p", pidS, "-lLf", NULL);
    }
    else
    {
        // Se crea el thread para revisar las columnas
        if (pthread_create(&threadColumn, NULL, check_columns, NULL) != 0)
        {
            printf("Error al crear el thread");
            return 1;
        }

        // Se hace el join del thread
        if (pthread_join(threadColumn, NULL) != 0)
        {
            printf("Error al unir el thread");
            return 1;
        }
        // Se espera a que termine el proceso hijo
        wait(NULL);

        niceRows = check_rows();
    }
    // Si el sudoku no es valido, entonces se rechaza
    if (!nicesubway && !niceRows && !acepted)
    {
        printf("Sudoku rechazado\n");
        exit(0);
    }
    printf("Sudoku aceptado\n");
    // Se muestra el estado del proceso y sus threads
    if (fork() == 0)
    {
        printf("Antes de terminar el estado de este proceso y sus threads es:\n");
        execlp("ps", "ps", "-p", pidS, "-lLf", NULL);
    }
    else
    {
        wait(NULL);
    }

    return 0;
}