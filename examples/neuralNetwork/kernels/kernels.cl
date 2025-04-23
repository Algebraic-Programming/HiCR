__kernel void gemm_kernel(__global const float *A,     // Matrix A (M x K)
                          __global const float *B,     // Matrix B (K x N)
                          __global float       *C,     // Matrix C (M x N)
                          const unsigned int    M,     // Number of rows of matrix A and C
                          const unsigned int    N,     // Number of columns of matrix B and C
                          const unsigned int    K,     // Number of columns of matrix A and rows of matrix B
                          const float           alpha, // Scaling factor for matrix product
                          const float           beta,  // Scaling factor for matrix C
                          const int             transB // Whether to transpose matrix B (0 for no transpose, 1 for transpose)
)
{
  // Get the row and column indices for the work item (thread)
  int globalRow = get_global_id(0); // Row index for matrix C
  int globalCol = get_global_id(1); // Column index for matrix C

  // Compute a single element (loop over K)
  float acc = 0.0f;
  for (int k = 0; k < K; k++)
  {
    if (transB == true) { acc += A[globalRow * K + k] * B[globalCol * K + k]; }
    else { acc += A[globalRow * K + k] * B[k * N + globalCol]; }
  }

  // Store the result
  C[globalRow * N + globalCol] = alpha * acc + beta * C[globalRow * N + globalCol];
}

__kernel void vector_add_kernel(__global const float *a,      // First input vector
                                __global const float *b,      // Second input vector
                                __global float       *output, // Output vector
                                const int             size    // Size of the input vectors
)
{
  int idx = get_global_id(0); // Get the global index
  if (idx < size)
  {
    output[idx] = a[idx] + b[idx]; // Perform element-wise addition
  }
}

__kernel void relu_kernel(__global const float *input,  // Input vector
                          __global float       *output, // Output vector
                          const int             size    // Size of the input vector
)
{
  int idx = get_global_id(0); // Get the global index
  if (idx < size)
  {
    output[idx] = max(input[idx], 0.0f); // Apply ReLU activation function
  }
}