/* File:  
 *    etapa3.c
 *
 * Purpose:
 *    Implementação do modelo produtor consumidor
 *
 *
 *
 * Compile:  mpicc -g -Wall -o etapa3 etapa3.c -lpthread -lrt
 * Usage:    mpiexec -n 3 ./etapa3
 *
 *Charlie Rodrigues Fonseca
 *Elana Tanan Sande
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h> 
#include <unistd.h>
#include <semaphore.h>
#include <time.h>
#include <mpi.h>     

#define THREAD_NUM 9    // Tamanho do pool de threads
#define BUFFER_SIZE 256 // Númermo máximo de tarefas enfileiradas


typedef struct {
    int p[3];
    int flag;
} Clock;

pthread_mutex_t outMutex;
pthread_cond_t outCondEmpty;
pthread_cond_t outCondFull;
int outClockCount = 0;
Clock outClockQueue[3];

pthread_mutex_t inMutex;
pthread_cond_t inCondEmpty;
pthread_cond_t inCondFull;
int inClockCount = 0;
Clock inClockQueue[3];


void CompareClock(Clock* clock, Clock* clock1){
    if (clock->p[0] < clock1->p[0]) { clock->p[0] = clock1->p[0]; }
    if (clock->p[1] < clock1->p[1]) { clock->p[1] = clock1->p[1]; }
    if (clock->p[2] < clock1->p[2]) { clock->p[2] = clock1->p[2]; }
}

void Event(int pid, Clock *clock) {
    clock->p[pid]++;
    printf("Process: %d, Clock: (%d, %d, %d)\n", pid, clock->p[0], clock->p[1], clock->p[2]);
}

Clock GetClock(pthread_mutex_t *mutex, pthread_cond_t *condEmpty, pthread_cond_t *condFull, int *clockCount, Clock *clockQueue) {
    Clock clock;
    pthread_mutex_lock(mutex);
    
    while (*clockCount == 0) {
        pthread_cond_wait(condEmpty, mutex);
    }

    clock = clockQueue[0];

    for (int i = 0; i < *clockCount - 1; i++) {
        clockQueue[i] = clockQueue[i + 1];
    }

    (*clockCount)--;
    
    pthread_mutex_unlock(mutex);

    pthread_cond_signal(condFull);
    
    return clock;
}



void PutClock(pthread_mutex_t *mutex, pthread_cond_t *condEmpty, pthread_cond_t *condFull, int *clockCount, Clock clock, Clock *clockQueue) {
    pthread_mutex_lock(mutex);

    while (*clockCount == 3) {
        pthread_cond_wait(condFull, mutex);
    }
    
    Clock temp = clock;

    clockQueue[*clockCount] = temp;
    (*clockCount)++;
    

    pthread_mutex_unlock(mutex);
    pthread_cond_signal(condEmpty);
}

void SendControl(int id, pthread_mutex_t *mutex, pthread_cond_t *condEmpty, pthread_cond_t *condFull, int *clockCount, Clock *clock, Clock *clockQueue){
    Event(id, clock);
    PutClock(mutex, condEmpty, condFull, clockCount, *clock, clockQueue);
}

Clock* ReceiveControl(int id, pthread_mutex_t *mutex, pthread_cond_t *condEmpty, pthread_cond_t *condFull, int *clockCount, Clock *clockQueue, Clock *clock){
    Clock* temp = clock;
    Clock clock2 = GetClock(mutex, condEmpty, condFull, clockCount, clockQueue);
    CompareClock(temp, &clock2);
    temp->p[id]++;
    printf("Process: %d, Clock: (%d, %d, %d)\n", id, clock->p[0], clock->p[1], clock->p[2]);
    return temp;
}

void Send(int pid, Clock *clock){
   int mensagem[3];
   mensagem[0] = clock->p[0];
   mensagem[1] = clock->p[1];
   mensagem[2] = clock->p[2];
   //MPI SEND
   MPI_Send(&mensagem, 3, MPI_INT, clock->flag, 0, MPI_COMM_WORLD);
}

void SendClock(int pid){
  Clock clock = GetClock(&outMutex, &outCondEmpty, &outCondFull, &outClockCount, outClockQueue);
  Send(pid, &clock);
}

void Receive(int pid, Clock *clock){
    int mensagem[3];
    //MPI RECV
    MPI_Recv(&mensagem, 3, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    clock->p[0] = mensagem[0];
    clock->p[1] = mensagem[1];
    clock->p[2] = mensagem[2];
}

void *MainThread(void *args) {
    long id = (long) args;
    int pid = (int) id;
    Clock* clock = malloc(sizeof(Clock));
    memset(clock, 0, sizeof(Clock)); 
    
    switch (pid) {
        case 0:
            //a - Event
            Event(pid, clock);
            
            //b - Send
            clock->flag = 1;
            SendControl(pid, &outMutex, &outCondEmpty, &outCondFull, &outClockCount, clock, outClockQueue);
            
            //c - Receive
            clock = ReceiveControl(pid, &inMutex, &inCondEmpty, &inCondFull, &inClockCount, inClockQueue, clock);
            
            //d - Send
            clock->flag = 2;
            SendControl(pid, &outMutex, &outCondEmpty, &outCondFull, &outClockCount, clock, outClockQueue);
            
            //e - Receive
            clock = ReceiveControl(pid, &inMutex, &inCondEmpty, &inCondFull, &inClockCount, inClockQueue, clock);
            
            //f - Send
            clock->flag = 1;
            SendControl(pid, &outMutex, &outCondEmpty, &outCondFull, &outClockCount, clock, outClockQueue);

            //g - Event
            Event(pid, clock);
            break;

        case 1:
            //h - Send
            clock->flag = 0;
            SendControl(pid, &outMutex, &outCondEmpty, &outCondFull, &outClockCount, clock, outClockQueue);
            
            //i - Receive
            clock = ReceiveControl(pid, &inMutex, &inCondEmpty, &inCondFull, &inClockCount, inClockQueue, clock);
            
            //j - Receive
            clock = ReceiveControl(pid, &inMutex, &inCondEmpty, &inCondFull, &inClockCount, inClockQueue, clock);
            break;

        case 2:
            //k - Event
            Event(pid, clock);
            
            //l - Send
            clock->flag = 0;
            SendControl(pid, &outMutex, &outCondEmpty, &outCondFull, &outClockCount, clock, outClockQueue);
            
            //m - Receive
            clock = ReceiveControl(pid, &inMutex, &inCondEmpty, &inCondFull, &inClockCount, inClockQueue, clock);
            break;

        default:
            break;
    }

    return NULL;
}

void *SendThread(void *args) {
    long pid = (long) args;
    Clock clock;
    
    while(1){
      clock = GetClock(&outMutex, &outCondEmpty, &outCondFull, &outClockCount, outClockQueue);
      Send(pid, &clock);
    }

    return NULL;
}



void *ReceiveThread(void *args) {
    long pid = (long) args;
    Clock clock;

    while(1){
      Receive(pid, &clock);
      PutClock(&inMutex, &inCondEmpty, &inCondFull, &inClockCount, clock, inClockQueue);
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


/*--------------------------------------------------------------------*/
int main(int argc, char* argv[]) {
   int my_rank;
   
      pthread_mutex_init(&inMutex, NULL);
      pthread_mutex_init(&outMutex, NULL);
      pthread_cond_init(&inCondEmpty, NULL);
      pthread_cond_init(&outCondEmpty, NULL);
      pthread_cond_init(&inCondFull, NULL);
      pthread_cond_init(&outCondFull, NULL);
  
   
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
   
   
      pthread_mutex_destroy(&inMutex);
      pthread_mutex_destroy(&outMutex);
      pthread_cond_destroy(&inCondEmpty);
      pthread_cond_destroy(&outCondEmpty);
      pthread_cond_destroy(&inCondFull);
      pthread_cond_destroy(&outCondFull);
   
   MPI_Finalize();

   return 0;
}  /* main */

/*-------------------------------------------------------------------*/