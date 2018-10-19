/**
 * CS3210 - Blocking communication in MPI.
 */

#include <mpi.h>
#include <stdio.h>

int main(int argc,char *argv[])
{
	int numtasks, rank, dest, source, rc, count, tag=1;  
	char inmsg, outmsg='x';
	MPI_Status Stat;

	MPI_Init(&argc,&argv);
	MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	if (rank == 0)	{
		dest = 1;
		source = 1;
		rc = MPI_Send(&outmsg, 1, MPI_CHAR, dest, tag, MPI_COMM_WORLD);
    // Process 0 prepares to receive 10 floating point values
    float buffer[10];
		rc = MPI_Recv(&buffer, 10, MPI_FLOAT, source, tag, MPI_COMM_WORLD, &Stat);
	  rc = MPI_Get_count(&Stat, MPI_FLOAT, &count);
	  printf("Task %d: Received %d float(s) from task %d with tag %d \n",
       		rank, count, Stat.MPI_SOURCE, Stat.MPI_TAG);
    printf("Task %d: The floats are [", rank);
    for (int i=0; i<count; i++) {
      if (i>0) printf(",");
      printf("%d", i);
    }
    printf("]\n");
		
	} 	else if (rank == 1)	{
		dest = 0;
		source = 0;
		rc = MPI_Recv(&inmsg, 1, MPI_CHAR, source, tag, MPI_COMM_WORLD, &Stat);
    // Process 1 prepares to send 10 floating point values
    float buffer[10] = {1,2,3,4,5,6,7,8,9,10};
		rc = MPI_Send(&buffer, 10, MPI_FLOAT, dest, tag, MPI_COMM_WORLD);
	  rc = MPI_Get_count(&Stat, MPI_CHAR, &count);
	  printf("Task %d: Received %d char(s) from task %d with tag %d \n",
       		rank, count, Stat.MPI_SOURCE, Stat.MPI_TAG);
    printf("Task %d: The char is %c\n", rank, inmsg);
	}


	MPI_Finalize();
	
	return 0;
}

