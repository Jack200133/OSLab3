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

#define SIZE 9

int sudoku[SIZE][SIZE];
bool acepted;

bool check_rows()
{
    int i, j, k;
    for (i = 0; i < SIZE; i++)
    {
        int nums[SIZE] = {0};
        for (j = 0; j < SIZE; j++)
        {
            int num = sudoku[i][j];

            if (nums[num - 1] == 1)
            {
                // printf("Error en fila %d\n", i);
                return false;
            }
            nums[num - 1] = 1;
        }
    }
    printf("Revisión de filas exitosa\n");
    return true;
}

void *check_columns(void *arg)
{
    int i, j, k;
    for (i = 0; i < SIZE; i++)
    {
        int nums[SIZE] = {0};
        int tID = syscall(SYS_gettid);
        printf("Columna %d revisanda por el Thread %d\n", i, tID);

        for (j = 0; j < SIZE; j++)
        {
            int num = sudoku[j][i];
            if (nums[num - 1] == 1)
            {
                // printf("Error en columna %d\n", i);
                acepted = false;
                pthread_exit(NULL);
            }
            nums[num - 1] = 1;
        }
    }
    // printf("Revisión de columnas exitosa\n");
    acepted = true;
    pthread_exit(NULL);
}

bool check_subgrid(void *arg)
{
    int row_start = ((int *)arg)[0];
    int col_start = ((int *)arg)[1];
    int nums[SIZE] = {0};
    int i, j;
    for (i = row_start; i < row_start + 3; i++)
    {
        for (j = col_start; j < col_start + 3; j++)
        {
            int num = sudoku[i][j];
            if (nums[num - 1] == 1)
            {
                // printf("Error en subarreglo (%d,%d)\n", row_start, col_start);
                return false;
            }
            nums[num - 1] = 1;
        }
    }
    return true;
}

int main(int argc, char *argv[])
{
    // char *path = argv[1];
    char *path = "sudoku";
    int sudoku_file = open(path, O_RDONLY);
    acepted = true;
    if (sudoku_file == -1)
    {
        printf("Error al abrir el archivo");
        return 1;
    }

    char *sudoku_str = mmap(NULL, SIZE * SIZE + 1, PROT_READ, MAP_PRIVATE, sudoku_file, 0);
    if (sudoku_str == MAP_FAILED)
    {
        printf("Error al mapear el archivo");
        return 1;
    }

    for (int i = 0; i < 9; i++)
    {
        for (int j = 0; j < 9; j++)
        {
            sudoku[i][j] = sudoku_str[i * 9 + j] - '0';
        }
    }
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

    pid_t pid = getpid();
    printf("PID: %d\n", pid);

    pthread_t threadColumn;
    int f = fork();
    char pidS[10];
    sprintf(pidS, "%d", pid);

    if (f == 0)
    {
        printf("ID del PAPA: %s\n", pidS);
        execlp("ps", "ps", "-p", pidS, "-lLf", NULL);
    }
    else
    {
        if (pthread_create(&threadColumn, NULL, check_columns, NULL) != 0)
        {
            printf("Error al crear el thread");
            return 1;
        }

        if (pthread_join(threadColumn, NULL) != 0)
        {
            printf("Error al unir el thread");
            return 1;
        }
        wait(NULL);

        niceRows = check_rows();
    }

    if (!nicesubway && !niceRows && !acepted)
    {
        printf("Sudoku rechazado\n");
        exit(0);
    }
    printf("Sudoku aceptado\n");
}