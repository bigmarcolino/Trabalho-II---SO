#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define N_PAGINAS 10
#define N_THREADS 1
#define N_FRAMES 10
//////////////////
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

    for(i = 0; i < N_FRAMES; i++)
    {
        printf("%d ", mp[i]);
    }

    printf("\n");
}

//imprime o vetor de índices de um processo
void printIndiceMP(int* indiceMP)
{
    int i;

    for(i = 0; i < WSL; i++)
    {
        printf("%d ", indiceMP[i]);
    }

    printf("\n");
}

//print do igor
void printIndicesMP(int* indices)
{
    int i;

    printf("{[PV, indice na MP]}\n");
    printf("{");
    for(i = 0; i < WSL; i++)
    {
        if(i < WSL-1){
            printf("[%d, ", mp[indices[i]]);
            printf("%d]\n", indices[i] );
        }
        else{
            printf("[%d, ", mp[indices[i]]);
            printf("%d]}\n", indices[i]);
        }
    }

    printf("\n");
}

//função responsável por alocar as páginas na memória
//será chamada por cada thread
void* aloca_paginas(void *threadid)
{
    int *id = (int*)threadid;
    int i, j, k;
    int qtd_paginas_mp = 0;    
    int numero_pagina;
    int contem = 0;

    //vetor com os índices da MP onde estão as páginas do processo
    //o valor mais a esquerda desse vetor representa o índice da MP que foi alocado a mais tempo
    //portanto, indiceMP[0] contém a posição da MP que guarda a página mais antiga
    int* indiceMP = (int*)malloc(WSL*sizeof(int));

    //inicializa indiceMP com -1, para indicar as posições que estão vazias
    for (i = 0; i < WSL; i++)
    {
        indiceMP[i] = VAZIO;
    }

    srand((unsigned)time(NULL));

    //for que controla as tentativas de alocação de página, com delay de 3 segundos
    //a política de alocação é a seguinte:
    //1--- um número é gerado aleatoriamente
    //2--- é verificado se esse número (página) já existe na MP. Caso exista, a MP não é alterada e o vetor de índices é atualizado para indicar qual é o índice da página mais antiga
    //3--- se o número (página) não existir na MP, haverão duas possibilidades: WSL atingido ou não
    //4--- se o WSL for atingido, a página mais antiga na MP será substituída pela página nova e o vetor de índices atualizado
    //5--- caso contrário, a página é colaca na MP, pois há espaço
    for (i = 0; i < N_PAGINAS; i++)
    {   
        pthread_mutex_lock(&mutex);

        //número de página gerado aleatoriamente
        numero_pagina = rand() % MAX;

        printf("\nProcesso %d\n", *id);

        printf("Número da página: %d\n", numero_pagina);

        //verifica se o número gerado está na memória
        for (int i = 0; i < qtd_paginas_mp; i++)
        {
            int indice_mp = indiceMP[i];

            //se estiver na mp, o vetor de índices é atualizado
            if(mp[indice_mp] == numero_pagina)
            {
                contem = 1;

                k = 0;

                for (j = 0; j < qtd_paginas_mp; j++)
                { 
                    if(indiceMP[j] != indice_mp)
                    {
                        indiceMP[k] = indiceMP[j];
                        k++;   
                    }
                    else
                    {
                        j++;
                        indiceMP[k] = indiceMP[j];
                        k++;
                    }
                }

                indiceMP[qtd_paginas_mp-1] = indice_mp;

                break;
            }
        }

        //se não está na memória, colocar na memória
        if(contem == 0)
        {
            //reorganiza o vetor indiceMP, quando a quantidade de páginas do processo é igual ao WSL
            if(qtd_paginas_mp == WSL)
            {
                int aux = indiceMP[0];

                k = 0;

                for (j = 1; j < qtd_paginas_mp; j++)
                { 
                    indiceMP[k] = indiceMP[j];
                    k++;   
                }

                indiceMP[qtd_paginas_mp-1] = aux;
                mp[aux] = numero_pagina;    
            }
            else
            {
                //verificar aqui a necessidade de swap out
                //outra alteração que deve ser feita é, antes de tentar alocar uma página, verificar se está no swap

                mp[next_index] = numero_pagina;
                indiceMP[qtd_paginas_mp] = next_index;
                next_index++;
                qtd_paginas_mp++;
            }
        }

        contem = 0;

        printf("Vetor de índices: ");
        printIndiceMP(indiceMP);

        printf("Memória principal: ");
        printMP();

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

    //inicializa a MP com -1, para indicar as posições que estão vazias
    for (i = 0; i < N_FRAMES; i++)
    {
        mp[i] = VAZIO;
    }

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

    free(mp);

    //desaloca o lock de exclusao mútua
    pthread_mutex_destroy(&mutex);

    //libera as threads
    pthread_exit(NULL);

    return 0;
}