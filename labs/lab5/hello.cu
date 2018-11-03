/*
 * Hello World in CUDA
 *
 * CS3210
 *
 * This program start from "hello world" string and should print "HELLO WORLD"
 *
 */

#include <stdio.h>

#define N       32 

// #define      DISCRETE

__global__ void hello(char *a, int len)
{
        //int tid = threadIdx.x;
	int block_index_in_grid = blockIdx.x * (gridDim.y * gridDim.z) + blockIdx.y * (gridDim.z) + blockIdx.z;
	int thread_index_in_block = threadIdx.x * (blockDim.y * blockDim.z) + threadIdx.y * (blockDim.z) + threadIdx.z;
	int tid = block_index_in_grid * (blockDim.x * blockDim.y * blockDim.z) + thread_index_in_block;
        if (tid >= len)
                return;
        a[tid] += 'A' - 'a';
}

int main()
{
        // original string
        char a[N] = "hello@world";
        // length
        int len = strlen(a);
        // pointer to the string on device
        char* ad;
        // pointer to the final string on host
        char* ah;
        // CUDA returned error code
        cudaError_t rc;


        //allocate space for the string on device (GPU) memory
        cudaMalloc((void**)&ad, N);
        cudaMemcpy(ad, a, N, cudaMemcpyHostToDevice);

        // launch the kernel
	dim3 gridDim(2, 2, 2);
	dim3 blockDim(2, 4);
        hello<<<gridDim, blockDim>>>(ad, len);
        cudaDeviceSynchronize();

	// for discrete GPUs, get the data from device memory to host memory
        cudaMemcpy(a, ad, N, cudaMemcpyDeviceToHost);
        ah = a;

        // was there any error?
        rc = cudaGetLastError();
        if (rc != cudaSuccess)
                printf("Last CUDA error %s\n", cudaGetErrorString(rc));

        // print final string
        printf("%s!\n", ah);

        // free memory
        cudaFree(ad);

        return 0;
}

