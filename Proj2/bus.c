#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <mpi.h>
#include <math.h>
#include <assert.h>
#include <time.h>

int p=1,rank=0;
void generateRandomArray(int*,int);
int  computeSum(int *,int end);
void outputArray(int *,int);
void shiftRight(int,int*);
void shiftLeft(int,int*);

int main(int argc, char *argv[])
{
  int startIndex,endIndex;
  struct timeval t1,t2;
  int n;
  int * array;
  int procSum = 0;
  int i = 0;
  MPI_Status status;

  srand(time(NULL));
  
  MPI_Init(&argc,&argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &p);
  
  //  assert(p > 1);
  n = atoi(argv[1]);
  // Get array count

  // Proc 0 will alloc
  if (rank == 0){// Partition and distribute
  array = (int *)malloc(sizeof(int)*n);
  generateRandomArray(array,n);
  int sum = 0;
  /* for (i=0;i<n;++i)
     sum+=array[i];*/
  //fprintf(stderr,"Actual sum:%d\n",sum);
  for (i = 1; i < p; ++i){
    //    fprintf(stderr, "Sending partition to %d...\n",i);
    MPI_Send(&array[i*(n/p)], n/p, MPI_INT, i, 1, MPI_COMM_WORLD);
  }
  }
  else{// Receive partition
    array = (int*)malloc(sizeof(int)*(n/p));
    MPI_Recv(array,n/p,MPI_INT,0,1,MPI_COMM_WORLD,&status);
    
  }
  procSum = computeSum(array, n/p);
  clock_t start = clock(),diff;

  //  MPI_Barrier(MPI_COMM_WORLD);
  //all sums computed. Shift val to right proc. then shift to left
  for (i = 0; i < 1000;++i)
    {
  shiftRight(rank,&procSum);
  shiftLeft(rank,&procSum);
    }
  MPI_Barrier(MPI_COMM_WORLD);
  diff = clock()-start;
  double msec=(double)diff*1000/CLOCKS_PER_SEC;
  if (rank==0)
    printf("%.3f,%d\n",msec,p);
  //  fprintf(stderr,"Done",procSum);
  //outputArray(array,n/p);
  MPI_Finalize();
}

void generateRandomArray(int * array, int size)
{
  int i;
  int r;

  for(i=0; i<size; ++i){
    array[i] = rand()%10;
  }
}

int computeSum(int *array, int end)
{
  int i,sum=0;
  // fprintf(stderr,"Printing from mem adderess %p\n", array);
  for (i=0; i<end; ++i)
    {
      //fprintf(stderr,"%d ", i);
      sum+=array[i];
    }
  return sum;
}

void outputArray(int * array, int size)
{
  int i = 0;
  fprintf(stderr, "Partition:");
  for (i=0; i<size; ++i){
    fprintf(stderr,"%d ",array[i]);
  }
  fprintf(stderr,"\n");
}


// returns 0 if data sent, 1 if data did not send.
void shiftRight(int rank, int *data)
{
  int recvSum,sent=0;
  MPI_Status status;
  if (rank+1 < p && rank > 0){
    //MPI_Sendrecv(data,1, MPI_INT, rank+1, 0, &recvSum, 1, MPI_INT, rank-1,0,MPI_COMM_WORLD,&status);
    MPI_Recv(&recvSum,1,MPI_INT,rank-1,0,MPI_COMM_WORLD,&status);
    *data+=recvSum;//add and send
    MPI_Send(data,1,MPI_INT,rank+1,0,MPI_COMM_WORLD);
  }
  else if (rank+1 == p){
    MPI_Recv(&recvSum,1,MPI_INT,rank-1,0,MPI_COMM_WORLD,&status);
    *data+=recvSum;
  }
  else if (rank == 0)
    MPI_Send(data,1,MPI_INT,rank+1,0,MPI_COMM_WORLD);
}

void shiftLeft(int rank, int *data)
{
  int finalSum;
  MPI_Status status;

  if (rank+1 < p && rank > 0){
    MPI_Recv(&finalSum,1,MPI_INT,rank+1,0,MPI_COMM_WORLD,&status);
    *data=finalSum;
    MPI_Send(data,1,MPI_INT,rank-1,0,MPI_COMM_WORLD);
  }
  else if (rank == 0){
    MPI_Recv(&finalSum,1,MPI_INT,rank+1,0,MPI_COMM_WORLD,&status);
    *data=finalSum;
    }
  else if (rank+1==p){
    MPI_Send(data,1,MPI_INT,rank-1,0,MPI_COMM_WORLD);
  }
}
