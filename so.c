#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pthread.h>

#define N 6
#define N_THREADS 3

typedef struct {
    pthread_t thread;
    int start;
    int end;
    int id_thread;
}t_args;

int** matriz1;
int** matriz2;
int** matrizRes;

//calcula o intervalo de tempo em milisegundos
unsigned int calculaTempo(struct timeval startTime, struct timeval endTime) {
    unsigned int totalSecs;

    totalSecs = (unsigned long long) (endTime.tv_sec - startTime.tv_sec) * 1000000 +
               (unsigned long long) (endTime.tv_usec - startTime.tv_usec);

    return totalSecs/1000;
}

int** alocarMatriz()
{
    int i;
    int** matriz = (int**)malloc(N*sizeof(int*));

    for(i=0; i<N; i++)
    {
        matriz[i] = (int*)malloc(N*sizeof(int));
    }

    return matriz;
}

void** gerarMatriz(int** matriz)
{
    int i, j;

    for(i=0; i<N; i++)
        for(j=0; j<N; j++)
            matriz[i][j] = rand() % 50;
}

void* multiplicacao(void *arg)
{
    t_args* indices = (t_args*) arg;

    printf("Thread %d está executando......\nThread %d calculará os valores das linhas %d a %d da matriz resultado\n", indices->id_thread, indices->id_thread, indices->start, indices->end -1);

    int i,j,k;

    for(i = indices->start; i < indices->end; i++)
    {
        for(j = 0; j < N; j++)
        {
            matrizRes[i][j]=0;
            for(k = 0; k < N; k++)
            {
                matrizRes[i][j] += matriz1[i][k] * matriz2[k][j];
            }
        }
    }

    pthread_exit(NULL);
}

void imprimeMatriz(int** matriz)
{
    int i, j;

    for(i=0; i<N; i++)
    {
        for(j=0; j<N; j++)
        {
            printf("%d ", matriz[i][j]);
        }

        printf("\n");
    }

    printf("\n");
}

void liberaMatriz(int** matriz)
{
    int i;

    for(i=0; i < N; i++)
    {
        free(matriz[i]);
    }

    free(matriz);
}

int main(int argc, char *argv[])
{
    if(N_THREADS > N){
        printf("Não foi possível calcular, pois a quantidade de threads é maior que a dimensão da matriz\n");
        return 0;
    }

    matriz1 = alocarMatriz();
    gerarMatriz(matriz1);

    printf("\nMATRIZ 1\n");
    imprimeMatriz(matriz1);

    matriz2 = alocarMatriz();
    gerarMatriz(matriz2);

    printf("MATRIZ 2\n");
    imprimeMatriz(matriz2);

    matrizRes = alocarMatriz();

    struct timeval startTime, endTime;
    unsigned int tempo_execucao;

    //-------- início do cálculo do processTime
    gettimeofday(&startTime, NULL);

    int intervalo = N/N_THREADS;
    int i, a, b;    

    t_args *arg = (t_args*)malloc(N_THREADS*sizeof(t_args));

    for(i = 0; i < N_THREADS; i++)
    {        
        if(i == 0){
            a = 0;
            b = intervalo;
        }
        else{
            a = b;

            if(i == N_THREADS-1){
                b = N;
            }
            else{
                b += intervalo;
            }
        }

        arg[i].start = a;
        arg[i].end = b;
        arg[i].id_thread = i;
    }
    
    for(i = 0; i < N_THREADS; i++)
    {   
        //cria as threads
        if (pthread_create(&arg[i].thread, NULL, multiplicacao, &arg[i]))
        {
            printf("--ERRO: pthread_create()\n");
            exit(-1);
        }
    }

    for (i = 0; i < N_THREADS; i++)
    {
        if (pthread_join(arg[i].thread, NULL))
        {
            printf("--ERRO: pthread_join() \n");
            exit(-1); 
        }
    }

    gettimeofday(&endTime, NULL);
    tempo_execucao = calculaTempo(startTime, endTime);
    //--------fim do cálculo do processTime

    printf("\nMATRIZ RESULTADO\n");
    imprimeMatriz(matrizRes);

    printf("Tempo de execução: %d ms\n", tempo_execucao);

    liberaMatriz(matriz1);
    liberaMatriz(matriz2);
    liberaMatriz(matrizRes);

    return 0;
}