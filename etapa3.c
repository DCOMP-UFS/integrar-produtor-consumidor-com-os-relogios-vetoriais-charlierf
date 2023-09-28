/* File:  
 *    etapa2.c
 *
 * Purpose:
 *    Implementação do modelo produtor consumidor
 *
 *
 * Compile:  gcc -g -Wall -o etapa2 etapa2.c -lpthread -lrt
 * Usage:    ./etapa2
 *
 * Compile:  mpicc -g -Wall -o etapa3 etapa3.c -lpthread -lrt
 * Usage:    mpiexec -n 3 ./rvet
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

typedef struct Clock { 
   int p[3];
} Clock;

//Mutexes para as filas de cada processo
pthread_mutex_t inMutex0, outMutex0, inMutex1, outMutex1, inMutex2, outMutex2;

//Condições para as filas de cada processo
pthread_cond_t outCondFull0, inCondFull0, outCondFull1, inCondFull1, outCondFull2, inCondFull2;
pthread_cond_t outCondEmpty0, inCondEmpty0, outCondEmpty1, inCondEmpty1, outCondEmpty2, inCondEmpty2;

//Filas de cada processo
Clock outClockQueue0[3], inClockQueue0[3], outClockQueue1[3], inClockQueue1[3], outClockQueue2[3], inClockQueue2[3];

int inClockCount0 = 0;
int inClockCount1 = 0;
int inClockCount2 = 0;

int outClockCount0 = 0;
int outClockCount1 = 0;
int outClockCount2 = 0;


void *MainThread(void* args){
   long pid = (long) args;
   Clock clock = {{0,0,0}};
   
   switch (pid)
   {
      case 0:
      //a
      Event(pid, clock);
      
      //b
      Event(pid, clock);
      PutClock(&outMutex0, &outCondEmpty0, &outCondFull0, &outClockCount0, &clock, &outClockQueue0);
      
      //c
      GetClock(inMutex0, inCondEmpty0, inCondFull0, inClockCount0, inClockQueue0);
      Event(pid, clock);
      
      //d
      Event(pid, clock);
      PutClock(&outMutex0, &outCondEmpty0, &outCondFull0, &outClockCount0, &clock, &outClockQueue0);
      
      //e
      GetClock(inMutex0, inCondEmpty0, inCondFull0, inClockCount0, inClockQueue0);
      Event(pid, clock);
      
      //f
      Event(pid, clock);
      PutClock(&outMutex0, &outCondEmpty0, &outCondFull0, &outClockCount0, &clock, &outClockQueue0);
      
      //g
      Event(pid, clock);
      break;
   
      case 1:
         //h
         Event(pid, clock);
         PutClock(&outMutex1, &outCondEmpty1, &outCondFull1, &outClockCount1, &clock, &outClockQueue1);
         //i
         GetClock(inMutex1, inCondEmpty1, inCondFull1, inClockCount1, inClockQueue1);
         Event(pid, clock);
         //j
         GetClock(inMutex1, inCondEmpty1, inCondFull1, inClockCount1, inClockQueue1);
         Event(pid, clock);
      break;
      
      case 2:
         //k
         Event(pid, clock);
         //l
         Event(pid, clock);
         PutClock(&outMutex2, &outCondEmpty2, &outCondFull2, &outClockCount2, &clock, &outClockQueue2);
         //m
         GetClock(inMutex2, inCondEmpty2, inCondFull2, inClockCount2, inClockQueue2);
         Event(pid, clock);
      break;
   
      default:
      break;
   }
   
   return NULL;
   
}


void Event(int pid, Clock *clock){
   clock->p[pid]++;
}


void GetClock(pthread_mutex_t *mutex, pthread_cond_t *condEmpty, pthread_cond_t *condFull, int *clockCount, Clock *clockQueue[3]){
      pthread_mutex_lock(&mutex);
        
      while (clockCount == 0){
         pthread_cond_wait(&condEmpty, &mutex);
         printf("Out Empty.\n");
      }
        
      Clock clock = clockQueue[0];
        
      for (int i = 0; i < clockCount - 1; i++){
        clockQueue[i] = clockQueue[i+1];
      }
        
      clockCount--;
        
      pthread_mutex_unlock(&mutex);
        
      pthread_cond_signal(&condFull);
}


void *SendThread(void* args){
   long pid = (long) args;
   Clock *clock = {{0,0,0}};
   int mensagem[3]; 
   
   
   switch (pid)
   {
      case 0:
         //b   
         GetClock(&outMutex0, &outCondEmpty0, &outCondFull0, &outClockCount0, &outClockCount0);
         mensagem[0] = clock->p[0];
         mensagem[1] = clock->p[1];
         mensagem[2] = clock->p[2];
         MPI_Send(&mensagem, 3, MPI_INT, 1, 0, MPI_COMM_WORLD);
         //d
         GetClock(&outMutex0, &outCondEmpty0, &outCondFull0, &outClockCount0, &outClockCount0);
         mensagem[0] = clock->p[0];
         mensagem[1] = clock->p[1];
         mensagem[2] = clock->p[2];
         MPI_Send(&mensagem, 3, MPI_INT, 2, 0, MPI_COMM_WORLD);
         //f
         GetClock(&outMutex0, &outCondEmpty0, &outCondFull0, &outClockCount0, &outClockCount0);
         mensagem[0] = clock->p[0];
         mensagem[1] = clock->p[1];
         mensagem[2] = clock->p[2];
         MPI_Send(&mensagem, 3, MPI_INT, 1, 0, MPI_COMM_WORLD);
      break;
   
      case 1:
         //h
         GetClock(&outMutex1, &outCondEmpty1, &outCondFull1, &outClockCount1, &outClockCount1);
         mensagem[0] = clock->p[0];
         mensagem[1] = clock->p[1];
         mensagem[2] = clock->p[2];
         MPI_Send(&mensagem, 3, MPI_INT, 0, 1, MPI_COMM_WORLD);
      break;
      
      case 2:
         //h
         GetClock(&outMutex2, &outCondEmpty2, &outCondFull2, &outClockCount2, &outClockCount2);
         mensagem[0] = clock->p[0];
         mensagem[1] = clock->p[1];
         mensagem[2] = clock->p[2];
         MPI_Send(&mensagem, 3, MPI_INT, 0, 2, MPI_COMM_WORLD);
      break;
   
      default:
      break;
   }
   
   return NULL;
}


void PutClock(pthread_mutex_t *mutex, pthread_cond_t *condEmpty, pthread_cond_t *condFull, int clockCount, Clock *clock, Clock *clockQueue[3]){
        pthread_mutex_lock(&mutex);

         while (clockCount == 3){
            pthread_cond_wait(&condFull, &mutex);
            printf("In Full.\n");
         }
      
         clockQueue[clockCount] = clock;
         clockCount++;
      
         pthread_mutex_unlock(&mutex);
         pthread_cond_signal(&condEmpty);
}


void *ReceiveThread(void* args){
   long pid = (long) args;
   Clock *clock = {{0,0,0}};
   int mensagem[3];
   switch (pid)
   {
      case 0:
        //c
         MPI_Recv(&mensagem, 3, MPI_INT, 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
         if (clock->p[0] < mensagem[0]) { clock->p[0] = mensagem[0]; }
         if (clock->p[1] < mensagem[1]) { clock->p[1] = mensagem[1]; }
         if (clock->p[2] < mensagem[2]) { clock->p[2] = mensagem[2]; }
         PutClock(&inMutex0, &inCondEmpty0, &inCondFull0, inClockCount0, &clock, &inClockQueue0);
         //e
         MPI_Recv(&mensagem, 3, MPI_INT, 2, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
         if (clock->p[0] < mensagem[0]) { clock->p[0] = mensagem[0]; }
         if (clock->p[1] < mensagem[1]) { clock->p[1] = mensagem[1]; }
         if (clock->p[2] < mensagem[2]) { clock->p[2] = mensagem[2]; }
         PutClock(&inMutex0, &inCondEmpty0, &inCondFull0, inClockCount0, &clock, &inClockQueue0);
      break;
   
      case 1:
        //i
         MPI_Recv(&mensagem, 3, MPI_INT, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
         if (clock->p[0] < mensagem[0]) { clock->p[0] = mensagem[0]; }
         if (clock->p[1] < mensagem[1]) { clock->p[1] = mensagem[1]; }
         if (clock->p[2] < mensagem[2]) { clock->p[2] = mensagem[2]; }
         PutClock(&inMutex1, &inCondEmpty1, &inCondFull1, inClockCount1, &clock, &inClockQueue1);
         //j
         MPI_Recv(&mensagem, 3, MPI_INT, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
         if (clock->p[0] < mensagem[0]) { clock->p[0] = mensagem[0]; }
         if (clock->p[1] < mensagem[1]) { clock->p[1] = mensagem[1]; }
         if (clock->p[2] < mensagem[2]) { clock->p[2] = mensagem[2]; }
         PutClock(&inMutex1, &inCondEmpty1, &inCondFull1, inClockCount1, &clock, &inClockQueue1);
      break;
      
      case 2:
        //m
         MPI_Recv(&mensagem, 3, MPI_INT, 0, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
         if (clock->p[0] < mensagem[0]) { clock->p[0] = mensagem[0]; }
         if (clock->p[1] < mensagem[1]) { clock->p[1] = mensagem[1]; }
         if (clock->p[2] < mensagem[2]) { clock->p[2] = mensagem[2]; }
         PutClock(&inMutex2, &inCondEmpty2, &inCondFull2, inClockCount2, &clock, &inClockQueue2);
      break;
   
      default:
        break;
   }
   
   return NULL;
}
/*
void GetClock(Clock *clockQueue, pthread_mutex_t *mutex, pthread_cond_t *condEmpty, pthread_cond_t *condFull){
   
}
*/

// Representa o processo de rank 0
void process0(){
   Clock clock = {{0,0,0}};
   Clock inClockQueue[3];
   Clock outClockQueue[3];
   pthread_t thread[3];
   
   pthread_create(&thread[0], NULL, &MainThread, (void*) 0);
   pthread_create(&thread[1], NULL, &SendThread, (void*) 0);
   pthread_create(&thread[2], NULL, &ReceiveThread, (void*) 0);

   //a
   Event(0, &clock);
   printf("a)Process: %d, Clock: (%d, %d, %d)\n", 0, clock.p[0], clock.p[1], clock.p[2]);
   
   //g
   Event(0, &clock);
   printf("g)Process: %d, Clock: (%d, %d, %d)\n", 0, clock.p[0], clock.p[1], clock.p[2]);
   
   for (int i = 0; i < 3; i++){  
      if (pthread_join(thread[i], NULL) != 0) {
         perror("Failed to join the thread");
      }  
   }
   
}

// Representa o processo de rank 1
void process1(){
   Clock clock = {{0,0,0}};
   Clock inClockQueue[3];
   Clock outClockQueue[3];
   
   pthread_t thread[3];
   
   pthread_create(&thread[0], NULL, &MainThread, (void*) 0);
   pthread_create(&thread[1], NULL, &SendThread, (void*) 0);
   pthread_create(&thread[2], NULL, &ReceiveThread, (void*) 0);
   
   //h
   Send(1, 0, &clock);
   printf("h)Process: %d, Clock: (%d, %d, %d)\n", 1, clock.p[0], clock.p[1], clock.p[2]);
   //i
   Receive(1, 0, &clock);
   printf("i)Process: %d, Clock: (%d, %d, %d)\n", 1, clock.p[0], clock.p[1], clock.p[2]);
   //j
   Receive(1, 0, &clock);
   printf("j)Process: %d, Clock: (%d, %d, %d)\n", 1, clock.p[0], clock.p[1], clock.p[2]);
   
   for (int i = 0; i < THREAD_NUM; i++){  
      if (pthread_join(thread[i], NULL) != 0) {
         perror("Failed to join the thread");
      }  
   }
   
}

// Representa o processo de rank 2
void process2(){
   Clock clock = {{0,0,0}};
   Clock inClockQueue[3];
   Clock outClockQueue[3];
   
   pthread_t thread[3];
   
   pthread_create(&thread[0], NULL, &MainThread, (void*) 0);
   pthread_create(&thread[1], NULL, &SendThread, (void*) 0);
   pthread_create(&thread[2], NULL, &ReceiveThread, (void*) 0);
   
   //k
   Event(2, &clock);
   printf("k)Process: %d, Clock: (%d, %d, %d)\n", 2, clock.p[0], clock.p[1], clock.p[2]);
   //l
   Send(2, 0, &clock);
   printf("l)Process: %d, Clock: (%d, %d, %d)\n", 2, clock.p[0], clock.p[1], clock.p[2]);
   //m
   Receive(2, 0, &clock);
   printf("m)Process: %d, Clock: (%d, %d, %d)\n", 2, clock.p[0], clock.p[1], clock.p[2]);
   
   for (int i = 0; i < 3; i++){  
      if (pthread_join(thread[i], NULL) != 0) {
         perror("Failed to join the thread");
      }  
   }
   
}

//Clock globalClock;


void printClock(Clock* clock, int id){
   printf("Consumidor %d consumiu o Relógio (%d, %d, %d)\n", id, clock->p[0], clock->p[1], clock->p[2]);
}


void *startThread(void* args);  

/*--------------------------------------------------------------------*/
int main(int argc, char* argv[]) {
   int my_rank;
   pthread_mutex_init(&inMutex0, NULL);
   pthread_mutex_init(&inMutex1, NULL);
   pthread_mutex_init(&inMutex2, NULL);
   pthread_mutex_init(&outMutex0, NULL);
   pthread_mutex_init(&outMutex1, NULL);
   pthread_mutex_init(&outMutex2, NULL);
   
   pthread_cond_init(&inCondEmpty0, NULL);
   pthread_cond_init(&inCondEmpty1, NULL);
   pthread_cond_init(&inCondEmpty2, NULL);
   pthread_cond_init(&outCondEmpty0, NULL);
   pthread_cond_init(&outCondEmpty1, NULL);
   pthread_cond_init(&outCondEmpty2, NULL);
   
   pthread_cond_init(&inCondFull0, NULL);
   pthread_cond_init(&inCondFull1, NULL);
   pthread_cond_init(&inCondFull2, NULL);
   pthread_cond_init(&outCondFull0, NULL);
   pthread_cond_init(&outCondFull1, NULL);
   pthread_cond_init(&outCondFull2, NULL);
   
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
   MPI_Finalize();


   pthread_mutex_destroy(&inMutex0);
   pthread_mutex_destroy(&inMutex1);
   pthread_mutex_destroy(&inMutex2);
   pthread_mutex_destroy(&outMutex0);
   pthread_mutex_destroy(&outMutex1);
   pthread_mutex_destroy(&outMutex2);
   
   pthread_cond_destroy(&inCondEmpty0);
   pthread_cond_destroy(&inCondEmpty1);
   pthread_cond_destroy(&inCondEmpty2);
   pthread_cond_destroy(&outCondEmpty0);
   pthread_cond_destroy(&outCondEmpty1);
   pthread_cond_destroy(&outCondEmpty2);
   
   pthread_cond_destroy(&inCondFull0);
   pthread_cond_destroy(&inCondFull1);
   pthread_cond_destroy(&inCondFull2);
   pthread_cond_destroy(&outCondFull0);
   pthread_cond_destroy(&outCondFull1);
   pthread_cond_destroy(&outCondFull2);
   return 0;
}  /* main */

/*-------------------------------------------------------------------*/

