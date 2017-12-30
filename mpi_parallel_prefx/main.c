#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <mpi.h>
#include <assert.h>
#include <time.h>
#include <math.h>
#include <stdint.h>
#include <fcntl.h>

int rnk;
int p;

int a=3;
int b=4;
int P=65071; // large prime
long n;
long x0;
long prefixValue;



typedef struct _PrefixItem{
  unsigned long local[4];
  unsigned long global[4];
  unsigned long myVal;
}PrefixItem;

PrefixItem * localList;

unsigned long findLowerLeft(long i, unsigned long LLval);
void computeLocalPrefix();

int findPrefix();
int LeftSend(int dist);
int RightSend(int dist);
void computeResults();
void incrementLocalPrefix(unsigned long matrix[4]);
int multMatrix(unsigned long rhs[4], unsigned long lhs[4]){
  rhs[0]=rhs[0]*lhs[0]+rhs[1]*lhs[2];
  rhs[1]=rhs[0]*lhs[1]+rhs[1]*lhs[3];
  rhs[2]=rhs[2]*lhs[0]+rhs[3]*lhs[2];
  rhs[3]=rhs[2]*lhs[1]+rhs[3]*lhs[3];
}
unsigned long findLowerLeft(long i, unsigned long LLval){
  if (i==1)
    return b*a+b;
  LLval = findLowerLeft(i-1, LLval)*a+b;
  return LLval;
}
int main(int argc, char *argv[]){

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rnk);
  MPI_Comm_size(MPI_COMM_WORLD, &p);
  clock_t start;
  clock_t diff;

  if (argc < 3){
    printf("Please provide a value for n followed by a seed\n");
    MPI_Finalize();
    exit(-1);
  }
  /* Obtain n */
  n = atol(argv[1]);
  int temp = n;
  n = n/p;
  //  if (rnk==0)
    //    printf("n=%d\n",temp);
  if (rnk==p-1)
    n+=temp%p;
  x0 = atol(argv[2]);
  
  if (argc == 4 && strcmp(argv[3],"serial")==0 && rnk==0){
    int i = 0;
    int items[n];
    start = clock();
    for (i=0; i < temp; ++i){
      if (i==0){
	items[i]=x0;
	continue;
      }
      items[i]=(a*items[i-1]+b)%P;
    }
    diff = clock()-start;
    printf("%s,%.5f\n","serial",(double)diff*1000/CLOCKS_PER_SEC);// .csv format

    // run serial algorithm over input
    MPI_Finalize();
    return 0;
  }
  else if(argc == 4 && strcmp(argv[3],"serial")==0 && rnk!=0){
    MPI_Finalize();
    return 0;
  }
 


  /* init prefix item list... */
  localList = (PrefixItem *)malloc(sizeof(PrefixItem)*n);
  
  if(rnk==0){
    //    printf("Start the clock...\n");
    start = clock();}
  computeLocalPrefix();
  findPrefix();
  int i;
  computeResults();
  /*  for (i=0;i<n;++i){
    if(rnk==0)
      printf("%4lu ", localList[i].myVal);
    }
  */

  //MPI_Barrier(MPI_COMM_WORLD);
  if (rnk==0){
  diff = clock()-start;
  printf("%d,%.5f\n",p,(double)diff*1000/CLOCKS_PER_SEC);}// .csv format
  MPI_Finalize();
  return 0;
}

void computeLocalPrefix(){
  int i=0,j;
  for (i=0; i < n; ++i){
    if (i!=0){
    localList[i].local[0]=a;
    localList[i].local[1]=0;
    localList[i].local[2]=b;
    localList[i].local[3]=1;
    for(j=0;j<4;++j)
      localList[i].global[j]=localList[i].local[j];
    }
    else {
      // identity matrix
      localList[i].local[0]=1;
      localList[i].local[1]=0;
      localList[i].local[2]=0;
      localList[i].local[3]=1;
      for(j=0;j<4;++j)
	localList[i].global[j]=localList[i].local[j];
    }

    if(i!=0){
      multMatrix(localList[i].global, localList[i-1].global);
      for(j=0;j<4;++j)
	localList[i].local[j] = localList[i].global[j];     
    }
  }
}

void incrementLocalPrefix(unsigned long matrix[4]){
  int i = 0;
  for (i=0; i < n; ++i){
    multMatrix(localList[i].local,matrix);
  }
}

void computeResults(){
  int i = 0;
  
  for (i=0; i < n; ++i){
    localList[i].myVal = (localList[i].local[0]*x0 + localList[i].local[2])%P;
    //localList[i].myVal=localList[i].local[2];
  }
}
void incrementGlobalPrefix(unsigned long matrix[4]){
  int i = 0;
  for (i=0; i < n; ++i){
    multMatrix(localList[i].global,matrix);
  }
}
int findPrefix(){
  int dist = 1;
  int i, dest;
  MPI_Status status;
  unsigned long globalOffset[4];

  for (i=0;;++i){
    dist= 1 << i;
    if (dist >= p)
      break;
    // send 1, 2, 4...
    if(dist + rnk < p && ((rnk)/dist)%2 == 0){ 
      // send right, recv right
      //      fprintf(stderr,"rnk=%d Send dist=right %d\n",rnk,dist);
      MPI_Sendrecv(&localList[n-1].global, 4, MPI_INT, rnk+dist, 0, &globalOffset, 4, MPI_INT, rnk+dist, 0, MPI_COMM_WORLD, &status);
      //fprintf(stderr,"rnk=%d recv dist=right %d\n",rnk,dist);
      incrementGlobalPrefix(globalOffset);
    }
  else if (rnk - dist >= 0 && ((rnk)/dist)%2 == 1){ // send left, recv left
    //fprintf(stderr,"rnk=%d Send dist=left %d\n",rnk,dist);
      MPI_Sendrecv(&localList[n-1].global, 4, MPI_INT, rnk-dist, 0, &globalOffset, 4, MPI_INT, rnk - dist, 0, MPI_COMM_WORLD, &status);
      //fprintf(stderr,"rnk=%d recv dist=left %d\n",rnk,dist);
      incrementGlobalPrefix(globalOffset);
      incrementLocalPrefix(globalOffset);
    }
  }
}


