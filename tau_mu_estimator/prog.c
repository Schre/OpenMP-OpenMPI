#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <assert.h>
#include <sys/time.h>

float runIteration(int argc, char *argv[], int size)
{
  char * sendData = (char *)malloc(sizeof(char)*size);
  struct timeval t1,t2;
  int rank, p,i;
 
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &p);

  assert(p>=2);
  if (rank==1){
    int dest = 0;
    
    for (i = 0; i < 10; ++i) // Send 10x
      {
    MPI_Send(sendData, size, MPI_CHAR, dest, 0, MPI_COMM_WORLD);
      }
  }else
    if (rank == 0){
      char *y = (char*)malloc(sizeof(char)*size);
      MPI_Status status;
      float tRecv = 0.0;

      for (i = 0; i < 10; ++i) // Recv 10x
	{
	  gettimeofday(&t1,NULL);
	  MPI_Recv(y,size,MPI_CHAR,MPI_ANY_SOURCE,MPI_ANY_TAG,MPI_COMM_WORLD,&status);
	  gettimeofday(&t2,NULL);
	  tRecv += (t2.tv_sec-t1.tv_sec)*1000 + (t2.tv_usec-t1.tv_usec)/1000.0;
	}

      tRecv/=10;
      printf("Rank=%d: received message of size %d from rank %d; Recv time %.3f millisec\n",rank,size,status.MPI_SOURCE,tRecv);
    }
}
int main(int argc, char *argv[])
{
  int size = 1,i=0,j,rank;
  float avg = 0.0;
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  for ( i= 0; i < 30; ++i)
  {
    runIteration(argc, argv, size);
    size*=2;
  }



    MPI_Finalize();
}
