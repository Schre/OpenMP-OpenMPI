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
void HyperCubeSum(int,int*);

int main(int argc, char *argv[])
{
  int startIndex,endIndex;

  struct timeval t1,t2;
  int n;
  int * array;
  int procSum = 0;
  int i = 0,remainder=0;
  MPI_Status status;
  srand(time(NULL));
  
  MPI_Init(&argc,&argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &p);
  
  //assert(p > 1);
  n = atoi(argv[1]);
  // Get array count

  // Proc 0 will alloc
  if (rank == 0){// Partition and distribute
  array = (int *)malloc(sizeof(int)*n);
  generateRandomArray(array,n);
  for (i = 1; i < p; ++i){
    //fprintf(stderr, "Sending partition to %d...\n",i);
    if (i==p-1)
      remainder = n%p;
    MPI_Send(&array[i*(n/p)], n/p+remainder, MPI_INT, i, 1, MPI_COMM_WORLD);
  }
  remainder=0;
  }
  else{// Receive partition
    if (rank == p-1)
      remainder = n%p;
    array = (int*)malloc(sizeof(int)*(n/p+remainder));
    MPI_Recv(array,(n/p)+remainder,MPI_INT,0,1,MPI_COMM_WORLD,&status);
    
  }

  procSum = computeSum(array, n/p+remainder);
  clock_t start = clock(),diff;
  //  MPI_Barrier(MPI_COMM_WORLD);
  for(i=0;i<1000;++i)
    {
  HyperCubeSum(rank,&procSum);
    }
  MPI_Barrier(MPI_COMM_WORLD);
  diff=clock()-start;
  double msec=(double)diff*1000/CLOCKS_PER_SEC;
  //  fprintf(stderr,"rnk=%d, sum=%d\n",rank,procSum);
  if(rank==0)
    printf("%.3f,%d\n",msec,p);
  //  outputArray(array,n/p+remainder);
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
  for (i=0; i<end; ++i)
    {
      sum+=array[i];
    }
  return sum;
}

void outputArray(int * array, int size)
{
  int i = 0;
  //  fprintf(stderr, "Partition:");
  for (i=0; i<size; ++i){
    //fprintf(stderr,"%d ",array[i]);
  }
  // fprintf(stderr,"\n");
}


// returns 0 if data sent, 1 if data did not send.
void HyperCubeSum(int rank, int *data)
{
  int lsb = 1; // least significant bit
  int recvSum,sent=0,i,dest;
  MPI_Status status;

  for (i=0;;++i){
    dest=rank^lsb;
    if (dest < p){
      MPI_Sendrecv(data,1,MPI_INT,dest,0,&recvSum,1,MPI_INT,dest,0,MPI_COMM_WORLD,&status);
      *data+=recvSum;
    }
    lsb=lsb<<1; // left shift
    if(lsb/2 > p)
      break;
    
  } 
}

