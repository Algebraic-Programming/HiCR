#pragma once

#include <memory>

#include "./tensor/tensor.hpp"

using gemmArgs_t = struct GemmParameter;

/**
 * Parameters for gemm operation
*/
struct GemmParameter
{
  /**
   * Check whether B matrix needs to be transposed
  */
  const bool transposeB;
  /**
   * Alpha
  */
  const float alpha;
  /**
   * Beta
  */
  const float beta;
  /**
   * Tensor A
  */
  const std::shared_ptr<tensor_t> A;
  /**
   * Tensor B
  */
  const std::shared_ptr<tensor_t> B;
  /**
   * Tensor C
  */
  std::shared_ptr<tensor_t> C;
};

using reluArgs_t = struct ReluParameters;

/**
 * Parameters for ReLU operation
*/
struct ReluParameters
{
  /**
   * Tensor
  */
  std::shared_ptr<tensor_t> t;
};

using vectorAddArgs_t = struct VectorAddPrameters;

/**
 * Parameters for vector add operation
*/
struct VectorAddPrameters
{
  /**
   * Tensor A
  */
  std::shared_ptr<tensor_t> a;
  /**
   * Tensor B
  */
  const std::shared_ptr<tensor_t> b;
};