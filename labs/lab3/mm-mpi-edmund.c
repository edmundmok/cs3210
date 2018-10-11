/**
 * CS3210 - Matrix Multiplication in MPI
 **/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <mpi.h>

int size;
int slaves;
int myid;

long long comm_time = 0;
long long comp_time = 0;

#define MASTER_ID slaves

typedef struct
{
	float ** element;
} matrix;

void slave_compute(matrix *	b, float a[], float result[]);

/**
 * Determines the current time
 *
 **/
long long wall_clock_time()
{
#ifdef __linux__
	struct timespec tp;
	clock_gettime(CLOCK_REALTIME, &tp);
	return (long long)(tp.tv_nsec + (long long)tp.tv_sec * 1000000000ll);
#else
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (long long)(tv.tv_usec * 1000 + (long long)tv.tv_sec * 1000000000ll);
#endif
}

/**
 * Allocates memory for a matrix of size SIZE
 * The memory is allocated row-major order, i.e.
 *  elements from the same row are allocated at contiguous
 *  memory addresses.
 **/
void allocate_matrix(matrix* m)
{
	int i, j;

	// allocate array for all the rows
	m->element = (float**)malloc(sizeof(float*) * size);
	if (m->element == NULL)
	{
		fprintf(stderr, "Out of memory\n");
		exit(1);
	}

	// allocate an array for each row of the matrix
	for	(i = 0; i < size; i++)
	{
		m->element[i] = (float*)malloc(sizeof(float) * size);
		if (m->element[i] == NULL)
		{
			fprintf(stderr, "Out of memory\n");
			exit(1);
		}
	}
}

/**
 * Initializes the elements of the matrix with
 * random values between 0 and 9
 **/
void init_matrix(matrix m)
{
	int i, j;

	for (i = 0; i < size; i++)
		for (j = 0; j < size; j++)
		{
			m.element[i][j] = rand() % 10;
		}
}

/**
 * Initializes the elements of the matrix with
 * element 0.
 **/
void init_matrix_zero(matrix m)
{
	int i, j;

	for (i = 0; i < size; i++)
		for (j = 0; j < size; j++)
		{
			m.element[i][j] = 0.0;
		}
}

/**
 * Prints the matrix
 *
 **/
void print_matrix(matrix m)
{
	int i, j;

	for (i = 0; i < size; i++)
	{
		printf("row =%4d: ", i);
		for (j = 0; j < size; j++)
			printf("%6.2f  ", m.element[i][j]);
		printf("\n");
	}
}


/*************************************************************************************************************************************/

/**
 * Function used by the slaves to receive data from the master
 * Each slave receives the entire B matrix
 * and a number of rows from the A matrix
 **/
void slave_receive_data(matrix* b, float a[size])
{
	int i, row_id;
	MPI_Status status;
	long long before, after;

	// Getting the entire B matrix from the master
	before = wall_clock_time();
	allocate_matrix(b);
	after = wall_clock_time();
	comp_time += after - before;

	before = wall_clock_time();
	for (i = 0; i < size; i++)
	{
		MPI_Recv(b->element[i], size, MPI_FLOAT, MASTER_ID, i, MPI_COMM_WORLD, &status);
	}
	fprintf(stderr," --- SLAVE %d: Received matrix B\n", myid);
	after = wall_clock_time();
	comm_time += after - before;


	// Getting a few rows of matrix A from the master
	float result[size];
	before = wall_clock_time();
	fprintf(stderr," --- SLAVE %d: Sending ready to MASTER\n", myid);
	MPI_Send(result, size, MPI_FLOAT, MASTER_ID, size, MPI_COMM_WORLD);
	while (1) {
		MPI_Recv(a, size, MPI_FLOAT, MASTER_ID, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		after = wall_clock_time();
		comm_time += after - before;
		if (status.MPI_TAG == size) break;
		row_id = status.MPI_TAG;
		slave_compute(b, a, result);
		//fprintf(stderr," --- SLAVE %d: Send computed result row %d\n", myid, row_id);
		before = wall_clock_time();
		MPI_Send(result, size, MPI_FLOAT, MASTER_ID, row_id, MPI_COMM_WORLD);
	}
	fprintf(stderr," --- SLAVE %d: Finished all work\n", myid);
}


/**
 * Function used by the slaves to compute the product
 * result = A x B
 **/
void slave_compute(matrix *b, float a[], float result[])
{
	int j, k;
	long long before, after;
	before = wall_clock_time();
	for (j=0; j<size; j++) {
		result[j] = 0;
		for (k=0; k<size; k++) {
			result[j] += a[k] * b->element[k][j];
		}
	}

	after = wall_clock_time();
	comp_time += after - before;
	//fprintf(stderr," --- SLAVE %d: Finished the computations\n", myid);
}


/**
 * Main function called by slaves
 *
 **/
void slave()
{
	int rows_per_slave = size / slaves ;

	float row_a_buffer[size];

	matrix b;

	float result[size];

	// Receive data
	slave_receive_data(&b, row_a_buffer);

	fprintf(stderr, " --- SLAVE %d: communication_time=%6.2f seconds; computation_time=%6.2f seconds\n", myid, comm_time / 1000000000.0, comp_time / 1000000000.0);
}


/*************************************************************************************************************************************/

/**
 * Function called by the master to distribute
 * rows from matrix A among different slaves
 * and the entire matrix B to each slave
 **/
void master_distribute(matrix a, matrix b, matrix result)
{
	int i, j, k;
	int slave_id = 1;
	MPI_Status status;

	// Send the entire B matrix to all slaves
	fprintf(stderr," +++ MASTER : Sending matrix B to all slaves\n");
	for (i = 0; i < size; i++)
	{
		float buffer[size];
		for (j = 0; j < size; j++)
			buffer[j] = b.element[i][j];

		for (slave_id = 0; slave_id < slaves; slave_id++)
		{
			MPI_Send(buffer, size, MPI_FLOAT, slave_id, i, MPI_COMM_WORLD);
		}
	}
	fprintf(stderr," +++ MASTER : Finished sending matrix B to all slaves\n");

	fprintf(stderr," +++ MASTER : Distributing matrix A to slaves\n");
	float row_a_buffer[size], result_buffer[size];
	int row_id;
	for (i=0; i<size; i++) {
		// Distribute each row at a time
		//fprintf(stderr, "MASTER row %d: ", i);
		for (j=0; j<size; j++) {
			row_a_buffer[j] = a.element[i][j];
			//fprintf(stderr, "%f ", row_a_buffer[j]);
		}
		//fprintf(stderr, "\n");
		// Wait for ready signal or possibly a result row
		MPI_Recv(result_buffer, size, MPI_FLOAT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		row_id = status.MPI_TAG;
		if (row_id != size) {
			for (int j=0; j<size; j++) {
				result.element[row_id][j] = result_buffer[j];
			}
		}
		slave_id = status.MPI_SOURCE;
		//fprintf(stderr," +++ MASTER : Sending row %d to slave %d\n", i, slave_id);
		MPI_Send(row_a_buffer, size, MPI_FLOAT, slave_id, i, MPI_COMM_WORLD);
	}

	// Ran out of rows to distribute
	// Inform everyone (tag = -1)
	for (i=0; i<slaves; i++) {
		MPI_Recv(result_buffer, size, MPI_FLOAT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		row_id = status.MPI_TAG;
		if (row_id != size) {
			fprintf(stderr," +++ MASTER : Received row %d\n", row_id);
			for (int j=0; j<size; j++) {
				result.element[row_id][j] = result_buffer[j];
			}
		}
		MPI_Send(NULL, 0, MPI_FLOAT, i, size, MPI_COMM_WORLD);
	}

}

/**
 * Main function called by the master process
 *
 **/
void master()
{
	matrix a, b, result;


	// Allocate memory for matrices
	allocate_matrix(&a);
	allocate_matrix(&b);
	allocate_matrix(&result);

	// Initialize matrix elements
	init_matrix(a);
	init_matrix(b);

	// Distribute data to slaves
	master_distribute(a, b, result);

	// Print the result matrix
	//print_matrix(a);
//	print_matrix(b);
	print_matrix(result);
}



/*************************************************************************************************************************************/

/**
 * Matrix multiplication using master-slave paradigm
 * The master initializes and sends the data to the
 * slaves. The slaves do all the computation and send
 * the product back to the master
 *
 * Matrix size must by a power of 2
 * Number of slaves must be a power of 2 and less than matrix size
 *
 * Total number of processes is 1 + number of slaves
 *
 **/
int main(int argc, char ** argv)
{
	srand(1);
	int nprocs;
	MPI_Init(&argc,&argv);
	MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
	MPI_Comm_rank(MPI_COMM_WORLD, &myid);

	size = 2048;

	// One master and nprocs-1 slaves
	slaves = nprocs - 1;

	if (myid == MASTER_ID)
	{
		fprintf(stderr, " +++ Process %d is master\n", myid);
		master();
	}
	else
	{
		fprintf(stderr, " --- Process %d is slave\n", myid);
		slave();
	}
	MPI_Finalize();
	return 0;
}