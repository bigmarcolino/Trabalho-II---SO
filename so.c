#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define N_PAGINAS 6
#define N_THREADS 4
#define N_FRAMES 10

#define WSL 4 //working set limit
#define VAZIO -1 //representa posição vazia no vetor
#define MAX 20 //MAX-1 será o maior número de página gerado aleatoriamente

int* mp; //memória principal
int* vetorProcessos; //vetor que indica a ordem de execução dos processos

//matriz de swap
int** swap; 

//matriz com os índices da MP onde estão as páginas dos processos
//a linha i da matriz representa o processo i
//o valor mais a esquerda de cada linha representa o índice da MP que foi alocado a mais tempo do processo i
//portanto, indiceMP[i][0] contém a posição da MP que guarda a página mais antiga do processo i
int** indiceMP;

int numero_execucao = 1; //será impresso para saber qual é a execução atual, sabendo que o maior número de execução é N_PAGINAS*N_THREADS
int next_index_MP = 0; //índice da memória que será alocado
int qtd_processos_mp = 0; //
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

//imprime o vetor de processos
void printVetorProcessos()
{
    int i;

    printf("\nVetor de processos: ");

    for(i = 0; i < N_THREADS; i++)
    {
        printf("%d ", vetorProcessos[i]);
    }

    printf("\n");
}

//imprime a matriz de swap
void printSwap()
{
    int i, j;

    printf("\nMatriz de swap\n");

    for(i = 0; i < N_THREADS; i++)
    {
        for (j = 0; j < WSL; j++)
        {
            printf("%d ", swap[i][j]);
        }

        printf("\n");        
    }
}

//imprime a matriz de índices de todos os processo
void printIndiceMP()
{
    int i, j;

    printf("\nMatriz de índices\n");

    for(i = 0; i < N_THREADS; i++)
    {
        for (j = 0; j < WSL; j++)
        {
            printf("%d ", indiceMP[i][j]);
        }

        printf("\n");
    }
}

//imprime a linha id da matriz de índices
void printIndiceMPLinha(int id)
{
    int i;

    for (i = 0; i < WSL; i++)
    {
        printf("%d ", indiceMP[id][i]);
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
    int i, j, k, w, z;
    int qtd_paginas_mp = 0;    
    int numero_pagina;
    int contem = 0;

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
        //***antes de tentar inserir uma página na MP, primeiramente devemos saber se o processo em execução está na matriz de swap
        //***caso negativo, a execução pode continuar normalmente
        //***caso positivo, devemos retirar o processo mais antigo da MP (vetorProcessos[0]) e colocá-lo na matriz de swap (swap out)
        //***depois do passo acima, mover o processo em execução do swap para a memória principal (swap in)
        //***pode haver o caso que o processo que vai sair da memória tem menos páginas que o processo que vai entrar
        //***portanto, concluímos que pode haver mais de um swap in
        //***por isso, devemos analisar se após um swap in vai haver espaço suficiente na MP para alocar todas as páginas do processo que vai entrar
        //***caso não haja, fazer mais um swap in e verificar se há espaço suficiente, e assim sucessivamente
        //***no pior dos casos, haverão 4 swap in, que é o caso que o processo que vai entrar na MP tem 4 páginas e os 4 mais antigos na MP que vão sair têm uma página cada
        //***após os swaps, ter cuidado para manter o vetor de índices atualizados, pois isso afeta a execução correta do LRU

        pthread_mutex_lock(&mutex);

        //verifica se o processo atual está na matriz de swap
        //caso esteja, o if faz os swaps necessários
        if (swap[*id][0] != -1)
        {
            int n_frames_necessarios = 0;
            int n_frames_livres = 0;
            //verificar quantos frames o processo precisa
            for(z = 0; z < WSL; z++)
            {
                if(swap[*id][z] != -1)
                {
                    n_frames_necessarios++;
                }
            }

            //contar quantos espaços livres ha na mp
            for(z = 0; z < N_FRAMES; z++)
            {
                if(mp[z] == -1)
                {
                    n_frames_livres++;
                }
            }

            // enquanto nao houver espaço suficiente na memoria
            while(n_frames_livres < n_frames_necessarios)
            {
                //tirar outro processo da memoria
                int proc_mais_antigo = vetorProcessos[0];

                printf("Swap out. Processo %d tem que sair da MP\n", proc_mais_antigo);

                for(j = 0; j < WSL; j++)
                {
                    int indice = indiceMP[proc_mais_antigo][j];

                    if(indice == -1)
                    {
                        break;
                    }

                    swap[proc_mais_antigo][j] = mp[indice];
                    mp[indice] = -1;
                }

                int aux = 1;

                //shift
                for (j = 0; j < N_THREADS; j++)
                {
                    if (vetorProcessos[aux] == -1)
                    {
                        break;
                    }

                    vetorProcessos[j] = vetorProcessos[aux];
                    aux++;
                }

                vetorProcessos[aux-1] = -1;

                printMP();
                printVetorProcessos();
                printSwap();   

                n_frames_livres = 0;

                //contar quantos espaços livres ha na mp
                for(z = 0; z < N_FRAMES; z++)
                {
                    if(mp[z] == -1)
                    {
                        n_frames_livres++;
                    }
                }
             }
             //no fim do while, count >= n_frames_necessarios ou seja, aqui o processo cabe no espaço livre
             //para cada pagina que vai ser colocada na mp, gravar as posicoes correspondentes no vtor mp e matriz indiceMP 
            int y = 0;

            for(z = 0; z < N_FRAMES; z++)
            {
                if(mp[z] == -1)
                {
                    if (swap[*id][y] == -1)
                    {
                        break;
                    }

                    mp[z] = swap[*id][y];
                    swap[*id][y] = -1;
                    indiceMP[*id][y] = z;
                    y++;
                }
            }

            //atualizar vetorProcessos
            for(z =0; z<N_THREADS;z++)
            {
                if(vetorProcessos[z] == -1)
                {
                    vetorProcessos[z] = *id;
                    break;
                }
            }

            //se couber, alocar as paginas do processo do swap para a mp 
            //atualizar indiceMP
            //zerar as posicoes que ficaram livres do swap
        }

        int count = 0;

        //verifica se o número do processo já está no vetor
        //alterar variável
        for (z = 0; z < N_THREADS; z++)
        {
            if (vetorProcessos[z] == *id)
            {
                count = 1;
                break;
            }
        }

        if (count == 0)
        {
            for (z = 0; z < N_THREADS; z++)
            {
                if (vetorProcessos[z] == -1)
                {
                    vetorProcessos[z] = *id;
                    break;
                }
            }
        }

        //número de página gerado aleatoriamente
        numero_pagina = rand() % MAX;

        printf("\nExecução %d\n", numero_execucao);
        numero_execucao++;

        printf("\nProcesso %d\n", *id);

        printf("Número da página: %d\n", numero_pagina);

        //verifica se o número gerado está na memória
        for (j = 0; j < qtd_paginas_mp; j++)
        {
            int indice_mp = indiceMP[*id][j];

            //se estiver na mp, o vetor de índices é atualizado
            if(mp[indice_mp] == numero_pagina)
            {
                contem = 1;

                k = 0;

                for (w = 0; w < qtd_paginas_mp; w++)
                { 
                    if(indiceMP[*id][w] != indice_mp)
                    {
                        indiceMP[*id][k] = indiceMP[*id][w];
                        k++;   
                    }
                    else
                    {
                        w++;
                        indiceMP[*id][k] = indiceMP[*id][w];
                        k++;
                    }
                }

                indiceMP[*id][qtd_paginas_mp-1] = indice_mp;

                break;
            }
        }

        //se não está na memória, colocar na memória
        if(contem == 0)
        {
            //reorganiza o vetor indiceMP, quando a quantidade de páginas do processo é igual ao WSL
            if(qtd_paginas_mp == WSL)
            {
                int aux = indiceMP[*id][0];

                k = 0;

                for (j = 1; j < qtd_paginas_mp; j++)
                { 
                    indiceMP[*id][k] = indiceMP[*id][j];
                    k++;   
                }

                indiceMP[*id][qtd_paginas_mp-1] = aux;
                mp[aux] = numero_pagina;    
            }
            else
            {
                //enquanto o next_index_MP é menor que o tamanho da MP, as inserções são sequenciais
                if(next_index_MP < N_FRAMES)
                {
                    mp[next_index_MP] = numero_pagina;
                    indiceMP[*id][qtd_paginas_mp] = next_index_MP;
                    next_index_MP++;
                    qtd_paginas_mp++;
                }
                else
                {   
                    int cheio = 1;

                    for (j = 0; j < N_FRAMES; j++)
                    {
                        if (mp[j] == -1)
                        {
                            cheio = 0;
                            mp[j] = numero_pagina;
                            indiceMP[*id][qtd_paginas_mp] = j;
                            qtd_paginas_mp++;

                            printf("Não foi necessário tirar um processo da memória, pois existe espaço para alocar a página\n");

                            break;
                        }
                    }

                    //swap out
                    if (cheio == 1)
                    {
                        int proc_mais_antigo = vetorProcessos[0];

                        printf("Processo %d tem que sair da MP\n", proc_mais_antigo);

                        for(j = 0; j < WSL; j++)
                        {
                            int indice = indiceMP[proc_mais_antigo][j];

                            if(indice == -1)
                            {
                                break;
                            }

                            swap[proc_mais_antigo][j] = mp[indice];
                            mp[indice] = -1;
                        }

                        int aux = 1;

                        //shift
                        for (j = 0; j < N_THREADS; j++)
                        {
                            if (vetorProcessos[aux] == -1)
                            {
                                break;
                            }

                            vetorProcessos[j] = vetorProcessos[aux];
                            aux++;
                        }

                        vetorProcessos[aux-1] = -1;

                        printMP();
                        printVetorProcessos();
                        printSwap();   
                    }

                    //***aqui a memória pode não estar cheia, verificar
                    //***retirar o processo mais antigo da memória (vetorProcessos[0]) e colocá-lo na matriz de swap (swap out)
                    //***denota-se retirar um processo da memória como mover suas respectivas páginas da memória princiapl para a matriz de swap
                    //***como uma página é adicionada na memória por vez, ao mover o processo mais antigo para o swap, é garantido que haverá espaço suficiente na MP para alocar a página
                }
            }
        }

        contem = 0;

        printf("Vetor de índices: ");
        printIndiceMPLinha(*id);

        printf("Memória principal: ");
        printMP();

        pthread_mutex_unlock(&mutex);

        sleep(3);
    }

    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    int i, j;
    int *id;
    pthread_t thread[N_THREADS];
    pthread_mutex_init(&mutex, NULL);

    //alocações de vetores e matrizes
    mp = (int*)malloc(N_FRAMES*sizeof(int));
    vetorProcessos = (int*)malloc(N_THREADS*sizeof(int));
    
    swap = (int**)malloc(N_THREADS*sizeof(int*));

    for(i = 0; i < N_THREADS; i++)
    {
        swap[i] = (int*)malloc(WSL*sizeof(int));
    }

    indiceMP = (int**)malloc(N_THREADS*sizeof(int*));

    for(i = 0; i < N_THREADS; i++)
    {
        indiceMP[i] = (int*)malloc(WSL*sizeof(int));
    }

    //inicializações dos vetores e matriz com -1, para indicar as posições que estão vazias
    for (i = 0; i < N_FRAMES; i++)
    {
        mp[i] = VAZIO;
    }

    for (i = 0; i < N_THREADS; i++)
    {
        vetorProcessos[i] = VAZIO;
    }

    for (i = 0; i < N_THREADS; i++)
    {
        for (j = 0; j < WSL; j++)
        {
            swap[i][j] = VAZIO;
        }
    }

    for (i = 0; i < N_THREADS; i++)
    {
        for (j = 0; j < WSL; j++)
        {
            indiceMP[i][j] = VAZIO;
        }
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

    printVetorProcessos();
    printSwap();
    printIndiceMP();

    //libera espaços de memória alocados
    free(mp);
    free(vetorProcessos);

    for(i = 0; i < N_THREADS; i++)
    {
        free(swap[i]);
    }

    free(swap);

    for(i = 0; i < N_THREADS; i++)
    {
        free(indiceMP[i]);
    }

    free(indiceMP);

    //desaloca o lock de exclusão mútua
    pthread_mutex_destroy(&mutex);

    //libera as threads
    pthread_exit(NULL);

    return 0;
}