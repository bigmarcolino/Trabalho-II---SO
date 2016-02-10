#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define N_PAGINAS 50
#define N_THREADS 20
#define N_FRAMES 64
#define WSL 4 //working set limit

#define VAZIO -1

int* mp; //memória principal

//imprime as páginas contidas em cada frame da memória principal
void printMP(int* mp)
{
    int i;

    for(i = 0; i < N_FRAMES; i++)
    {
        printf("%d ", mp[i]);
    }

    printf("\n");
}

//função responsável por alocar as páginas na memória
//será chamada por cada thread
void* aloca_paginas(void *arg)
{
    int i;
    int* indiceMP = (int*)malloc(WSL*sizeof(int)); //vetor com os índices

    //colocar -1 no vetor de índices, para indicar que as posições estão vazias
    for (i = 0; i < WSL; i++)
    {
        indiceMP[i] = VAZIO;
    }

    //for que controla as tentativas de alocação de página, com delay de 3 segundos
    for (i = 0; i < N_PAGINAS; i++)
    {
        
        
        sleep(3);
    }

    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    int i;
    pthread_t thread[N_THREADS];

    mp = (int*)malloc(N_FRAMES*sizeof(int));

    //criação das threads, com delay de 3 segundos
    for(i = 0; i < N_THREADS; i++)
    {
        if (pthread_create(&thread[i], NULL, aloca_paginas, NULL))
        {
            printf("--ERRO: pthread_create()\n");
            exit(-1);
        }

        sleep(3);
    }

    //aguarda todas as threads terminarem
    for (i = 0; i < N_THREADS; i++)
    {
        if (pthread_join(thread[i], NULL))
        {
            printf("--ERRO: pthread_join() \n");
            exit(-1);
        }
    }

    free(mp);

    return 0;
}