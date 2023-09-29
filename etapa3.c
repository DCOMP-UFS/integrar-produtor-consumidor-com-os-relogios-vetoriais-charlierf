/* File:  
 *    etapa3.c
 *
 * Purpose:
 *    Implementação do modelo produtor consumidor
 *
 *
 * Compile:  gcc -g -Wall -o etapa2 etapa2.c -lpthread -lrt
 * Usage:    ./etapa2
 *
 * Compile:  mpicc -g -Wall -o etapa3 etapa3.c -lpthread -lrt
 * Usage:    mpiexec -n 3 ./etapa3
 *
 *Charlie Rodrigues Fonseca
 *Elana Tanan Sande
 */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h> 
#include <unistd.h>
#include <semaphore.h>
#include <time.h>
#include <mpi.h>     

#define THREAD_NUM 9    // Tamanho do pool de threads
#define BUFFER_SIZE 256 // Númermo máximo de tarefas enfileiradas


typedef struct {
    int p[3];
} Clock;

pthread_mutex_t outMutex[3];
pthread_cond_t outCondEmpty[3];
pthread_cond_t outCondFull[3];
int outClockCount[3];
Clock outClockQueue[3][3];

pthread_mutex_t inMutex[3];
pthread_cond_t inCondEmpty[3];
pthread_cond_t inCondFull[3];
int inClockCount[3];
Clock inClockQueue[3][3];

void printClock(Clock* clock, int id){
   printf("Teste - Process: %d, Clock: (%d, %d, %d)\n", id, clock->p[0], clock->p[1], clock->p[2]);
}

Clock* CompareClock(Clock* clock, Clock* clock1){
    printf("Comparing (%d, %d, %d) and (%d, %d, %d)\n", clock->p[0], clock->p[1],clock->p[2], clock1->p[0],clock1->p[1], clock1->p[2]);
    if (clock->p[0] < clock1->p[0]) { clock->p[0] = clock1->p[0]; }
    if (clock->p[1] < clock1->p[1]) { clock->p[1] = clock1->p[1]; }
    if (clock->p[2] < clock1->p[2]) { clock->p[2] = clock1->p[2]; }
    printf("Return (%d, %d, %d)\n", clock->p[0], clock->p[1], clock->p[2]);
    return clock;
}

void Event(int pid, Clock *clock) {
    clock->p[pid]++;
    printf("Process: %d, Clock: (%d, %d, %d)\n", pid, clock->p[0], clock->p[1], clock->p[2]);
}

void GetClock(pthread_mutex_t *mutex, pthread_cond_t *condEmpty, pthread_cond_t *condFull, int *clockCount, Clock *clockQueue, Clock *clock) {
    pthread_mutex_lock(mutex);

    while (*clockCount == 0) {
        pthread_cond_wait(condEmpty, mutex);
        //printf("Wait get.\n");
    }
    //printf("Pass get\n");

    *clock = clockQueue[0];
    
    //printClock(clock, 5);

    for (int i = 0; i < *clockCount - 1; i++) {
        clockQueue[i] = clockQueue[i + 1];
    }

    (*clockCount)--;
    
    pthread_mutex_unlock(mutex);

    pthread_cond_signal(condFull);
}

void PutClock(pthread_mutex_t *mutex, pthread_cond_t *condEmpty, pthread_cond_t *condFull, int *clockCount, Clock *clock, Clock *clockQueue) {
    pthread_mutex_lock(mutex);

    while (*clockCount == 3) {
        pthread_cond_wait(condFull, mutex);
        //printf("Wait. put\n");
    }
    //printf("Pass put\n");

    clockQueue[*clockCount] = *clock;
    (*clockCount)++;

    pthread_mutex_unlock(mutex);
    pthread_cond_signal(condEmpty);
}

void SendControl(int id, pthread_mutex_t *mutex, pthread_cond_t *condEmpty, pthread_cond_t *condFull, int *clockCount, Clock *clock, Clock *clockQueue){
    Event(id, clock);
    //printf("Process: %d, Clock: (%d, %d, %d)\n", id, clock->p[0], clock->p[1], clock->p[2]);
    PutClock(mutex, condEmpty, condFull, clockCount, clock, clockQueue);
}

void ReceiveControl(int id, pthread_mutex_t *mutex, pthread_cond_t *condEmpty, pthread_cond_t *condFull, int *clockCount, Clock *clock, Clock *clockQueue){
    Clock *clock2;
    clock->p[id]++;
    printf("Before Rget\n");
    GetClock(mutex, condEmpty, condFull, clockCount, clockQueue, clock2);
    printf("After Rget\n");
    clock = CompareClock(clock, clock2);
    printf("Process: %d, Clock: (%d, %d, %d)\n", id, clock->p[0], clock->p[1], clock->p[2]);
}

void Send(int pid, int destino, Clock *clock){
   int mensagem[3];
   MPI_Status status;
   mensagem[0] = clock->p[0];
   mensagem[1] = clock->p[1];
   mensagem[2] = clock->p[2];
   //MPI SEND
   MPI_Send(&mensagem, 3, MPI_INT, destino, pid, MPI_COMM_WORLD);
   printf("Sent message with tag %d: %d %d %d\n", pid, mensagem[0], mensagem[1], mensagem[2]);
}

void Receive(int pid, int origem, Clock *clock){
   int mensagem[3];
   MPI_Status status;
    //MPI RECV
   MPI_Recv(&mensagem, 3, MPI_INT, origem, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
   printf("Process %d Received message with tag %d: %d %d %d\n", pid, status.MPI_TAG, mensagem[0], mensagem[1], mensagem[2]);
   /*if (clock->p[0] < mensagem[0]) { clock->p[0] = mensagem[0]; }
   if (clock->p[1] < mensagem[1]) { clock->p[1] = mensagem[1]; }
   if (clock->p[2] < mensagem[2]) { clock->p[2] = mensagem[2]; }*/
}

void *MainThread(void *args) {
    long pid = (long) args;
    Clock clock = {{0, 0, 0}};
    Clock clock2 = {{0, 0, 0}};
    
    //int mensagem[3];

    switch (pid) {
        case 0:
            //a - Event
            printf("a) ");
            Event(pid, &clock);
            
            
            //b - Send
            printf("b) ");
            SendControl(pid, &outMutex[0], &outCondEmpty[0], &outCondFull[0], &outClockCount[0], &clock, outClockQueue[0]);
            
            //c - Receive
            printf("c) ");
            ReceiveControl(pid, &inMutex[0], &inCondEmpty[0], &inCondFull[0], &inClockCount[0], inClockQueue[0], &clock2);
            
            //d - Send
            printf("d) ");
            SendControl(pid, &outMutex[0], &outCondEmpty[0], &outCondFull[0], &outClockCount[0], &clock, outClockQueue[0]);
            
            //e - Receive
            printf("e) ");
            ReceiveControl(pid, &inMutex[0], &inCondEmpty[0], &inCondFull[0], &inClockCount[0], inClockQueue[0], &clock2);
            //clock = CompareClock(clock, clock2);
            
            //f - Send
            printf("f) ");
            SendControl(pid, &outMutex[0], &outCondEmpty[0], &outCondFull[0], &outClockCount[0], &clock, outClockQueue[0]);

            //g - Event
            printf("g) ");
            Event(pid, &clock);
            break;

        case 1:
            printf("h) ");
            SendControl(pid, &outMutex[1], &outCondEmpty[1], &outCondFull[1], &outClockCount[1], &clock, outClockQueue[1]);
            
            printf("i) ");
            ReceiveControl(pid, &inMutex[1], &inCondEmpty[1], &inCondFull[1], &inClockCount[1], inClockQueue[1], &clock);
            
            printf("j) ");
            ReceiveControl(pid, &inMutex[1], &inCondEmpty[1], &inCondFull[1], &inClockCount[1], inClockQueue[1], &clock);
            break;

        case 2:
            printf("k) ");
            Event(pid, &clock);
            
            printf("l) ");
            SendControl(pid, &outMutex[2], &outCondEmpty[2], &outCondFull[2], &outClockCount[2], &clock, outClockQueue[2]);
            
            printf("m) ");
            ReceiveControl(pid, &inMutex[2], &inCondEmpty[2], &inCondFull[2], &inClockCount[2], inClockQueue[2], &clock);
            break;

        default:
            break;
    }

    return NULL;
}

void *SendThread(void *args) {
    long pid = (long) args;
    Clock clock;
    //int mensagem[3];
    //MPI_Status status;
    //int tag;

    switch (pid) {
        case 0:
            GetClock(&outMutex[0], &outCondEmpty[0], &outCondFull[0], &outClockCount[0], outClockQueue[0], &clock);
            printf("Processo %ld tirou da fila %d %d %d\n", pid, clock.p[0], clock.p[1], clock.p[2]);
            //mensagem[0] = clock.p[0];
            //mensagem[1] = clock.p[1];
            //mensagem[2] = clock.p[2];
            Send(pid, 1, &clock);
            
            GetClock(&outMutex[0], &outCondEmpty[0], &outCondFull[0], &outClockCount[0], outClockQueue[0], &clock);
            printf("Processo %ld tirou da fila %d %d %d\n", pid, clock.p[0], clock.p[1], clock.p[2]);
            //mensagem[0] = clock.p[0];
            //mensagem[1] = clock.p[1];
            //mensagem[2] = clock.p[2];
            Send(pid, 2, &clock);
            
            
            GetClock(&outMutex[0], &outCondEmpty[0], &outCondFull[0], &outClockCount[0], outClockQueue[0], &clock);
            printf("Processo %ld tirou da fila %d %d %d\n", pid, clock.p[0], clock.p[1], clock.p[2]);
            //mensagem[0] = clock.p[0];
            //mensagem[1] = clock.p[1];
            //mensagem[2] = clock.p[2];
            Send(pid, 1, &clock);
            break;

        case 1:
            GetClock(&outMutex[1], &outCondEmpty[1], &outCondFull[1], &outClockCount[1], outClockQueue[1], &clock);
            printf("Processo %ld tirou da fila %d %d %d\n", pid, clock.p[0], clock.p[1], clock.p[2]);
            //mensagem[0] = clock.p[0];
            //mensagem[1] = clock.p[1];
            //mensagem[2] = clock.p[2];
            Send(pid, 0, &clock);
            break;

        case 2:
            GetClock(&outMutex[2], &outCondEmpty[2], &outCondFull[2], &outClockCount[2], outClockQueue[2], &clock);
            printf("Processo %ld tirou da fila %d %d %d\n", pid, clock.p[0], clock.p[1], clock.p[2]);
            //mensagem[0] = clock.p[0];
            //mensagem[1] = clock.p[1];
            //mensagem[2] = clock.p[2];
            Send(pid, 0, &clock);
            break;

        default:
            break;
    }

    return NULL;
}


void *ReceiveThread(void *args) {
    long pid = (long) args;
    Clock clock;
    //MPI_Status status;
    //int tag;
    //int mensagem[3] = {0,0,0};
    switch (pid) {
        case 0:
            printf("Case 0r\n");
            
            Receive(pid, 1, &clock);
            //printClock(&clock, pid);
            PutClock(&inMutex[0], &inCondEmpty[0], &inCondFull[0], &inClockCount[0], &clock, inClockQueue[0]);
            
            Receive(pid, 2, &clock);
            //printClock(&clock, pid);
            PutClock(&inMutex[0], &inCondEmpty[0], &inCondFull[0], &inClockCount[0], &clock, inClockQueue[0]);
            break;

        case 1:
            printf("Case 1r\n");
            Receive(pid, 0, &clock);
            PutClock(&inMutex[1], &inCondEmpty[1], &inCondFull[1], &inClockCount[1], &clock, inClockQueue[1]);
            
            Receive(pid, 0, &clock);
            PutClock(&inMutex[1], &inCondEmpty[1], &inCondFull[1], &inClockCount[1], &clock, inClockQueue[1]);
            break;

        case 2:
            printf("Case 2r\n");
            Receive(pid, 0, &clock);
            PutClock(&inMutex[2], &inCondEmpty[2], &inCondFull[2], &inClockCount[2], &clock, inClockQueue[2]);
            break;

        default:
            break;
    }

    return NULL;
}



// Representa o processo de rank 0
void process0(){
   pthread_t thread[3];
   
   pthread_create(&thread[0], NULL, &MainThread, (void*) 0);
   pthread_create(&thread[1], NULL, &SendThread, (void*) 0);
   pthread_create(&thread[2], NULL, &ReceiveThread, (void*) 0);

   for (int i = 0; i < 3; i++){  
      if (pthread_join(thread[i], NULL) != 0) {
         perror("Failed to join the thread");
      }  
   }
   
}

// Representa o processo de rank 1
void process1(){
   pthread_t thread[3];
   
   pthread_create(&thread[0], NULL, &MainThread, (void*) 1);
   pthread_create(&thread[1], NULL, &SendThread, (void*) 1);
   pthread_create(&thread[2], NULL, &ReceiveThread, (void*) 1);
   
   for (int i = 0; i < THREAD_NUM; i++){  
      if (pthread_join(thread[i], NULL) != 0) {
         perror("Failed to join the thread");
      }  
   }
   
}

// Representa o processo de rank 2
void process2(){
   pthread_t thread[3];
   
   pthread_create(&thread[0], NULL, &MainThread, (void*) 2);
   pthread_create(&thread[1], NULL, &SendThread, (void*) 2);
   pthread_create(&thread[2], NULL, &ReceiveThread, (void*) 2);
   
   for (int i = 0; i < 3; i++){  
      if (pthread_join(thread[i], NULL) != 0) {
         perror("Failed to join the thread");
      }  
   }
   
}

//Clock globalClock;


/*--------------------------------------------------------------------*/
int main(int argc, char* argv[]) {
   int my_rank;
   
   for (int i = 0; i < 3; i++){
      pthread_mutex_init(&inMutex[i], NULL);
      pthread_mutex_init(&outMutex[i], NULL);
      pthread_cond_init(&inCondEmpty[i], NULL);
      pthread_cond_init(&outCondEmpty[i], NULL);
      pthread_cond_init(&inCondFull[i], NULL);
      pthread_cond_init(&outCondFull[i], NULL);
   }
  
   
   MPI_Init(NULL, NULL); 
   MPI_Comm_rank(MPI_COMM_WORLD, &my_rank); 

   if (my_rank == 0) { 
      process0();
   } else if (my_rank == 1) {  
      process1();
   } else if (my_rank == 2) {  
      process2();
   }

   /* Finaliza MPI */
   
   
   for (int i = 0; i < 3; i++){
      pthread_mutex_destroy(&inMutex[i]);
      pthread_mutex_destroy(&outMutex[i]);
      pthread_cond_destroy(&inCondEmpty[i]);
      pthread_cond_destroy(&outCondEmpty[i]);
      pthread_cond_destroy(&inCondFull[i]);
      pthread_cond_destroy(&outCondFull[i]);
   }
   MPI_Finalize();

   return 0;
}  /* main */

/*-------------------------------------------------------------------*/

