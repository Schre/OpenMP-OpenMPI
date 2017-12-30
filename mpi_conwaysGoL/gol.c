#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <mpi.h>
#include <assert.h>
#include <time.h>
#include <math.h>
#include <stdint.h>

typedef struct COORDINATES
{
  int row;
  int col;
}Coordinates;

int p=0,rank=0,rows=0,cols=0,totalRows=0,totalCols=0,dim;
int ** procBoard, *ghostTop, *ghostBottom, *ghostLeft, *ghostRight;
int ** procIDs, ULCorner=0, URCorner=0, DLCorner=0, DRCorner=0; // corners. 
int fCount=0;
Coordinates globalCoor; // Upper left-most position relative to global board
void OutputBoard();
void OutputRow(FILE* fp,int rowIndx); // utility function for displaying the global board
void GetXYLocation();
void ComputePhase();
int * twoToOne(int ** array);
int doEdgeCase(Coordinates rc);
int exchUp(int col); // return up value
void SendRecvGhost();
int * colToArray(int col)
{
  int i = 0;
  int * array = (int *)malloc(sizeof(int)*totalRows);
  for (i=0;i<totalRows;++i)
    array[i] = procBoard[i][col];
  return array;
}
void initializeProcIDArray()
{
  int row = 0, col = 0, i = 0;
  int width = cols;
  procIDs = (int **)malloc(sizeof(int *)*rows);
  for (row = 0; row < rows; ++row){
    procIDs[row] = (int *)malloc(sizeof(int *)*cols);
    for (col = 0; col < cols; ++col){
      procIDs[row][col] = i++;
      }
  }
}
int LinearCoordinates(int rowIndx, int colIndx,int width)
{
  return rowIndx*width + colIndx;
}
int getColIndx(int indx,int width)
{
  return indx % width; 
}
int getRowIndx(int indx, int width)
{
  return indx / width;
}
void GenerateInitialGoL()
{
  int r=0,c=0;
  for(r=0;r<totalRows;++r){
    procBoard[r] = (int *)malloc(sizeof(int*)*(totalCols));
    for(c=0;c<totalCols;++c)
      {
	procBoard[r][c]= 0;
       
      }
  }
  procBoard[0][0]=procBoard[0][1]=procBoard[0][2] = 1;

  GetXYLocation();
  initializeProcIDArray();
  ghostTop = (int *)malloc(sizeof(int)*(totalCols));
  ghostBottom = (int *)malloc(sizeof(int)*(totalCols));
  ghostRight = (int *)malloc(sizeof(int)*(totalRows));
  ghostLeft = (int *)malloc(sizeof(int)*(totalRows));

}
// METHOD: DIVIDE NxN MATRIX INTO N^2/p RECTANGLES
int main(int argc, char *argv[])
{
  int r,c;
  srand(time(NULL));

  // PHASE 1: INITIALIZATION
  // STEP 1: GET RANK AND #P
  MPI_Init(&argc,&argv);
  MPI_Comm_rank(MPI_COMM_WORLD,&rank);
  MPI_Comm_size(MPI_COMM_WORLD,&p);

  if(argc < 4){
    printf("Usage: a.out <#rows> <#cols>\n");
    return 1;
  }
  // STEP 2: GET DIMENSIONS NxN
  dim=atoi(argv[1]);
  // STEP 3: GENERATE GAME BOARD (chunk)
  (p>dim)?MPI_Finalize(),exit(-1):dim;
  (dim%p!=0)?dim+=p-dim%p:dim;
  if(p>1){
  cols=2;rows=p/2;
  totalCols=dim/cols;
  totalRows=dim/rows;
  }
  else{
    rows=cols=1;
    totalCols=totalRows=dim;
  }

  
  procBoard = (int **)malloc(sizeof(int**)*(totalRows));
  GenerateInitialGoL(); 
  int g = 0,i;
  int it = atoi(argv[3]);
  double timeSpent;
  clock_t diff;
  clock_t start;
  clock_t comm = 0,temp,comp;
  if(rank==0){
    start=clock();
    printf("Start clock..\n");
  }
  for (g=0;g < it; ++g)
    {
      if (rank==0)
	temp=clock();
      SendRecvGhost();
      if(rank==0){
	temp=clock()-temp;
	comm+=temp; // keep track of total comm time
      }
      ComputePhase();
        // if(g%5==0)
      OutputBoard();
    }
  
  MPI_Barrier(MPI_COMM_WORLD);
  
  if(rank==0){
    diff=(clock()-start);
    comp=((clock()-start)-comm); // runtime - comm time
    fprintf(stderr,"Average time taken with p=%d and sqrt(elements)=%d : %.5fs with comm time=%.5fs, comp time=%.5fs\n",p,dim,(double)(((double)diff*1000/CLOCKS_PER_SEC)/it),(double)((double)comm*1000/CLOCKS_PER_SEC)/it,(double)(((double)comp*1000/CLOCKS_PER_SEC)/it));
  }
  MPI_Finalize();
  return 0;
}

void OutputRow(FILE* fp, int rowIndx)// for testing purposes
{
  int i,j,r; // print in order
  for (i=0;i<totalCols;++i)
    fprintf(fp,"%d ", rank);//procBoard[rowIndx][i]);
}
void OutputBoard()
{
  int i,j, go=1,r=0,k=0;
  int board[dim][dim];
  int * buffer =(int*)malloc(sizeof(int)*totalRows*totalCols);
  MPI_Status status;
  char fname[100];
  char temp[100];
  strcpy(fname,"file");
  sprintf(temp,"%d",fCount);
  strcat(fname,temp);
  if (rank==0)
    ++fCount;
  FILE* fp;
  // SEND IT ALL TO 0, 0 ASSEMBLES, 0 PRINTS
  if (rank != 0)
    MPI_Send(twoToOne(procBoard), totalRows*totalCols,MPI_INT,0,rank,MPI_COMM_WORLD);
  else{ // get and assemble pieces
     fp=fopen(fname, "w+");
    // copy rank 0's board
    /************************************/
    for (r=0;r<totalRows;++r)
      for(j=0;j<totalCols;++j)
	board[r][j] = procBoard[r][j];
    /************************************/
    for(i=1;i<p;++i){
      MPI_Recv(buffer,totalRows*totalCols,MPI_INT,i,i,MPI_COMM_WORLD,&status);
    // put this piece in board array...
      for(r=0;r<totalRows;++r){
	for(j=0;j<totalCols;++j){
	  board[getRowIndx(i,cols)*totalRows+r][getColIndx(i,cols)*totalCols+j] = buffer[r*totalCols + j];
	}
      }
    }
    for (i=0;i<dim;++i){
      for(j=0;j<dim;++j)
	fprintf(fp,"%d ",board[i][j]);
      fprintf(fp,"\n");
    }
  }
  if (rank==0)
    fclose(fp);
}
void GetXYLocation()
{
  //globalCoor.row = totalRows/p * rank;
  //globalCoor.col = totalCols/p * rank;
}
void ComputePhase()
{
  Coordinates curCoordinates;
  int i,j, neighborCount,width=totalCols,height=totalRows;
  for(i=0;i<totalRows;++i){
    curCoordinates.row = i;
    for(j=0;j<totalCols;++j){
      neighborCount = 0;
      // Check for edge case:
      curCoordinates.col = j;
      //check up
      if (curCoordinates.row != 0)
	neighborCount+=procBoard[curCoordinates.row - 1][curCoordinates.col];
      else
	neighborCount+=ghostTop[curCoordinates.col];
      // check down
      if (curCoordinates.row != height-1)
	neighborCount+=procBoard[curCoordinates.row + 1][curCoordinates.col];
      else
	neighborCount+=ghostBottom[curCoordinates.col];
      // check left							
      if(curCoordinates.col != 0)
	neighborCount+=procBoard[curCoordinates.row][curCoordinates.col-1];
      else
	neighborCount+=ghostLeft[curCoordinates.row];
      // check right
      if(curCoordinates.col != width - 1)
	neighborCount+=procBoard[curCoordinates.row][curCoordinates.col+1];
      else
	neighborCount+=ghostRight[curCoordinates.row];
            //check up left
      if(curCoordinates.col != 0 && curCoordinates.row != 0)
	neighborCount+=procBoard[curCoordinates.row - 1][curCoordinates.col - 1];
      else if (curCoordinates.col != 0)
	neighborCount+=ghostTop[curCoordinates.col - 1];
      else if (curCoordinates.row != 0)
	neighborCount+=ghostLeft[curCoordinates.row - 1];
      else
	neighborCount+=ULCorner;
      // check down left
      if(curCoordinates.col != 0 && curCoordinates.row != height-1)
	neighborCount+=procBoard[curCoordinates.row+1][curCoordinates.col -1];
      else if(curCoordinates.col != 0)
	neighborCount+=ghostBottom[curCoordinates.col-1];
      else if (curCoordinates.row != height-1)
	neighborCount+=ghostLeft[curCoordinates.row+1];
      else
	neighborCount+=DLCorner;
      // check up right
      if(curCoordinates.col != width - 1 && curCoordinates.row != 0)
	neighborCount+=procBoard[curCoordinates.row-1][curCoordinates.col+1];
      else if(curCoordinates.col != width - 1)
	neighborCount+=ghostTop[curCoordinates.col+1];
      else if (curCoordinates.row != 0)
	neighborCount+=ghostRight[curCoordinates.row-1];
      else
	neighborCount+=URCorner;
      // check down right
      if(curCoordinates.col != width - 1 && curCoordinates.row != height - 1)
	neighborCount+=procBoard[curCoordinates.row+1][curCoordinates.col+1];
      else if (curCoordinates.col != width - 1)
	neighborCount+=ghostBottom[curCoordinates.col+1];
      else if (curCoordinates.row != height - 1)
	neighborCount+=ghostRight[curCoordinates.row+1];
      
      if (neighborCount < 2 || neighborCount > 3) // dead
	procBoard[curCoordinates.row][curCoordinates.col] = 0;
      else if (procBoard[curCoordinates.row][curCoordinates.col]) // if alive and 2 or 3
	procBoard[curCoordinates.row][curCoordinates.col] = 1;
      else if (neighborCount == 3) // if dead but count == 3
      procBoard[curCoordinates.row][curCoordinates.col]=1;
    }
  }
}
void SendRecvGhost()
{
  MPI_Status stat;
  int width = cols,height=rows;//sqrt(p);
  int rowIndx = getRowIndx(rank,width);
  int colIndx = getColIndx(rank,width);
  // send up get up down
  // fprintf(stderr,"Send UP:rank=%d at[%d][%d] => %d\n",rank,rowIndx,colIndx,LinearCoordinates((rowIndx-1+height)%height,colIndx,width));
    MPI_Sendrecv(procBoard[0], totalCols, MPI_INT,LinearCoordinates((rowIndx-1+height)%height,colIndx,width),0,ghostBottom,totalCols,MPI_INT,LinearCoordinates((rowIndx+1+height)%height,colIndx,width),0,MPI_COMM_WORLD,&stat);
    // send down get up
    //fprintf(stderr,"Send DOWN:rank=%d at[%d][%d] => %d\n",rank,rowIndx,colIndx,LinearCoordinates((rowIndx+1+height)%height,colIndx,width));
    MPI_Sendrecv(procBoard[totalRows-1],totalCols,MPI_INT,LinearCoordinates((rowIndx+1+height)%height,colIndx,width),0,ghostTop,totalCols,MPI_INT,LinearCoordinates((rowIndx-1+height)%height,colIndx,width),0,MPI_COMM_WORLD,&stat);
    
    //send left, recv right
    //    fprintf(stderr,"Send LEFT:rank=%d at[%d][%d] => %d\n",rank,rowIndx,colIndx,LinearCoordinates(rowIndx,(colIndx-1+width)%width,width));
    MPI_Sendrecv(colToArray(0),totalRows,MPI_INT,LinearCoordinates(rowIndx,(colIndx-1+width)%width,width),0,ghostRight,totalRows,MPI_INT,LinearCoordinates(rowIndx,(colIndx+1+width)%width,width),0,MPI_COMM_WORLD,&stat);
    
    // send right, recv left
    //    fprintf(stderr,"Send RIGHT:rank=%d at[%d][%d] => %d\n",rank,rowIndx,colIndx,LinearCoordinates(rowIndx,(colIndx+1+width)%width,width));
    MPI_Sendrecv(colToArray(totalCols-1),totalRows,MPI_INT,LinearCoordinates(rowIndx,(colIndx+1+width)%width,width),0,ghostLeft,totalRows,MPI_INT,LinearCoordinates(rowIndx,(colIndx-1+width)%width,width),0,MPI_COMM_WORLD,&stat);
    
  // GET 4 CORENERS

  // Send upper left, recv lower right
    //    fprintf(stderr,"Send UL:rank=%d at[%d][%d] => %d\n",rank,rowIndx,colIndx,LinearCoordinates((rowIndx-1+height)%height,(colIndx-1+width)%width,width));
    MPI_Sendrecv(procBoard[0],1,MPI_INT,LinearCoordinates((rowIndx-1+height)%height,(colIndx-1+width)%width,width),0,&DRCorner,1,MPI_INT,LinearCoordinates((rowIndx+1+height)%height,(colIndx+1+width)%width,width),0,MPI_COMM_WORLD,&stat);
    // send upper right, recv lower left
    
    //    fprintf(stderr,"Send UR:rank=%d at[%d][%d] => %d\n",rank,rowIndx,colIndx,LinearCoordinates((rowIndx-1+height)%height,(colIndx+1+width)%width,width));
    MPI_Sendrecv(&procBoard[0][totalCols-1],1,MPI_INT,LinearCoordinates((rowIndx-1+height)%height, (colIndx+1+width)%width,width),0,&DLCorner,1,MPI_INT,LinearCoordinates((rowIndx+1+height)%height,(colIndx-1+width)%width,width),0,MPI_COMM_WORLD,&stat);
    
  // send lower right, recv upper left
    //    fprintf(stderr,"Send DR:rank=%d at[%d][%d] => %d\n",rank,rowIndx,colIndx,LinearCoordinates((rowIndx+1+height)%height,(colIndx+1+width)%width,width));
    MPI_Sendrecv(&procBoard[totalRows-1][totalCols-1],1,MPI_INT,LinearCoordinates((rowIndx+1+height)%height,(colIndx+1+width)%width,width),0,&ULCorner,1,MPI_INT,LinearCoordinates((rowIndx-1+height)%height,(colIndx-1+width)%width,width),0,MPI_COMM_WORLD,&stat);
    
    // send lower left, recv upper right
    //    fprintf(stderr,"Send DL:rank=%d at[%d][%d] => %d\n",rank,rowIndx,colIndx,LinearCoordinates((rowIndx+1+height)%height,(colIndx-1+width)%width,width));
    MPI_Sendrecv(&procBoard[totalRows-1][0],1,MPI_INT,LinearCoordinates((rowIndx+1+height)%height,(colIndx-1+width)%width,width),0,&URCorner,1,MPI_INT,LinearCoordinates((rowIndx-1+height)%height,(colIndx+1+width)%width,width),0,MPI_COMM_WORLD,&stat);
}
int isEdgeCase(Coordinates rc)
{
  return 0;
}
int doEdgeCase(Coordinates rc)// return 1 if edge case is done, 0 if no work is done
{
  // IMPLEMENT
  if (rc.row == 0){// top
    if(rc.col == 0){// upper left

    }
    else if (rc.col == cols){// top left
   
    }
    else{ // top somewhere in middle
   
    }
    return 1;
  }
 else if (rc.row == rows){// bottom
   if(rc.col == 0){ // bottom left

   }
   else if (rc.col == cols){ // bottom left
   }
   else{ // bottom somewhere in middle
     
   }
   return 1;
 }
  return 0;
}

int * twoToOne(int ** array)
{
  int * ret = (int *)malloc(sizeof(int)*totalRows*totalCols),i=0,j=0;
  for(i=0;i<totalRows;++i){
    for(j=0;j<totalCols;++j){
      ret[i*totalCols+j]=array[i][j];
      
    }
  }
  return ret;
}
