#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define N_PAGINAS 5
#define N_THREADS 2
#define N_FRAMES 64

#define WSL 4 //working set limit
#define VAZIO -1 //representa posição vazia no vetor
#define MAX 20 //MAX-1 será o maior número de página gerado aleatoriamente

int* mp; //memória principal
int next_index = 0; //índice da memória que será alocado
pthread_mutex_t mutex; //exclusão mútua

//imprime as páginas contidas em cada frame da memória principal
void printMP()
{
    int i;

    for(i = 0; i < next_index; i++)
    {
        printf("%d ", mp[i]);
    }

    printf("\n");
}

void printIndicesMP(int* indices)
{
    int i;

    printf("[");
    for(i = 0; i < WSL; i++)
    {
        if(i < WSL-1)
            printf("%d, ", indices[i]);
        else
            printf("%d]\n", indices[i]);
    }

    printf("\n");
}


//função responsável por alocar as páginas na memória
//será chamada por cada thread
void* aloca_paginas(void *threadid)
{
    int *id = (int*)threadid;
    int i;
    int qtd_paginas_mp = 0;
    int* indiceMP = (int*)malloc(WSL*sizeof(int)); //vetor com os índices

    //colocar -1 no vetor de índices, para indicar que as posições estão vazias
    for (i = 0; i < WSL; i++)
    {
        indiceMP[i] = VAZIO;
    }

    srand((unsigned)time(NULL));

    //for que controla as tentativas de alocação de página, com delay de 3 segundos
    for (i = 0; i < N_PAGINAS; i++)
    {
        pthread_mutex_lock(&mutex);

        printf("Processo %d\n", *id);
        printf("Qtd paginas %d\n", qtd_paginas_mp);

        if(qtd_paginas_mp < WSL)
        {
            mp[next_index] = rand() % MAX;

            printf("Valor aleatorio %d\n", mp[next_index]);
            printf("Indice na MP %d\n\n", next_index);

            indiceMP[qtd_paginas_mp] = next_index;
            next_index++;
            qtd_paginas_mp++;
        }
        printf("Vetor de indices na MP: ");
        printIndicesMP(indiceMP);
        pthread_mutex_unlock(&mutex);

        sleep(3);
    }

    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    int i;
    int *id;
    pthread_t thread[N_THREADS];
    pthread_mutex_init(&mutex, NULL);

    mp = (int*)malloc(N_FRAMES*sizeof(int));

    //criação das threads, com delay de 3 segundos
    for(i = 0; i < N_THREADS; i++)
    {
        //aloca espaço para o identificar da thread
        if ((id = malloc(sizeof(int))) == NULL)
        {
            pthread_exit(NULL);
            return 1;
        }
        *id=i;

        if (pthread_create(&thread[i], NULL, aloca_paginas, (void *)id))
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

    printMP();

    free(mp);

    //desaloca o lock de exclusao mútua
    pthread_mutex_destroy(&mutex);

    //libera as threads
    pthread_exit(NULL);

    return 0;
}
