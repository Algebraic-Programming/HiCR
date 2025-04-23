#pragma once

#include <memory>

#include "./arguments.hpp"
#include "./operation.hpp"

// Axis attribute of flatten operation
const std::string axisAttribute = "axis";
// Alpha attribute of GEMM operation
const std::string alphaAttribute = "alpha";
// Beta attribute of GEMM operation
const std::string betaAttribute = "beta";
// Transpose B matrix attribute of GEMM operation
const std::string transBAttribute = "transB";

/**
 * Factory class to create arguments for each operation in the network 
*/
class ArgumentsFactory
{
  public:

  /**
   * 
   * Create arguments for gemm
   * 
   * @param[in] A Tensor A
   * @param[in] B Tensor B
   * @param[in] C Tensor C
   * @param[in] operation description of the gemm  operation
   *   
   * @return struct with the gemm arguments
*/
  const gemmArgs_t gemm(const std::shared_ptr<tensor_t> &A, const std::shared_ptr<tensor_t> &B, std::shared_ptr<tensor_t> &C, const operation_t &operation)
  {
    // Get attributes
    auto alpha      = operation.getAttribute<float>(alphaAttribute);
    auto beta       = operation.getAttribute<float>(betaAttribute);
    auto transposeB = static_cast<bool>(operation.getAttribute<int64_t>(transBAttribute));

    return gemmArgs_t{.transposeB = transposeB, .alpha = alpha, .beta = beta, .A = A, .B = B, .C = C};
  }

  /**
   * Creates arguments for ReLU
   * 
   * @param[in] input tensor
   * 
   * @return struct with the ReLU arguments
  */
  const reluArgs_t relu(std::shared_ptr<tensor_t> &t) { return reluArgs_t{.t = t}; }

  /**
   * 
   * Create arguments for vector add
   * 
   * @param[in] A Tensor A
   * @param[in] B Tensor B
   *   
   * @return struct with the vector add arguments
*/
  const vectorAddArgs_t vectorAdd(std::shared_ptr<tensor_t> &a, const std::shared_ptr<tensor_t> &b) { return vectorAddArgs_t{.a = a, .b = b}; }
};