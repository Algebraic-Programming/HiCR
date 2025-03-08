__kernel void gemm_kernel(
    __global unsigned int *Mptr,     // Number of rows of matrix A and C
    __global unsigned int *Nptr,     // Number of columns of matrix B and C
    __global unsigned int *Kptr,     // Number of columns of matrix A and rows of matrix B
    __global const float *A,  // Matrix A (M x K)
    __global const float *B,  // Matrix B (K x N)
    __global float *C        // Matrix C (M x N)
) {
    const unsigned int M = *Mptr ;
    const unsigned int N = *Nptr ;    
    const unsigned int K = *Kptr ;

    // Get the row and column indices for the work item (thread)
  int row = get_global_id(1); // Row index for matrix C
  int col = get_global_id(0); // Column index for matrix C

  // Initialize the sum for the C matrix entry
  float sum = 0.0f;

  // Perform the matrix multiplication: C[row, col] = sum(A[row, k] * B[k, col])
  if (row < M && col < N)
  {
    for (int k = 0; k < K; k++) { sum += A[row * K + k] * B[k * N + col]; }

    // Write the result to the C matrix
    C[row * N + col] += sum;
  }
}
