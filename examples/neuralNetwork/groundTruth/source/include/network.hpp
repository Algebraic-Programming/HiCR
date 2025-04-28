#pragma once

#include <algorithm>
#include <cblas.h>
#include <hicr/core/exceptions.hpp>

#include "tensor.hpp"
#include "operation.hpp"

////// Attributes to retrieve the operation attributes
// Axis attribute of flatten operation
const std::string axis = "axis";
// Alpha attribute of GEMM operation
const std::string alphaAttribute = "alpha";
// Beta attribute of GEMM operation
const std::string betaAttribute = "beta";
// Transpose B matrix attribute of GEMM operation
const std::string transBAttribute = "transB";

// Tensors name to retrieve pre-trained weights
const std::string fc1Weight      = "gemm1.weight";
const std::string fc2RightWeight = "right_branch_gemm1.weight";
const std::string fc2LeftWeight  = "left_branch_gemm1.weight";
const std::string fc2Left2Weight = "left_branch_gemm2.weight";
const std::string fc3Weight      = "gemm2.weight";

// Tensors name to retrieve pre-trained biases
const std::string fc1Bias      = "gemm1.bias";
const std::string fc2RightBias = "right_branch_gemm1.bias";
const std::string fc2LeftBias  = "left_branch_gemm1.bias";
const std::string fc2Left2Bias = "left_branch_gemm2.bias";
const std::string fc3Bias      = "gemm2.bias";

// Operations name
const std::string flattenOperation    = "/Flatten";
const std::string gemm1Operation      = "/gemm1/Gemm";
const std::string gemm2RightOperation = "/right_branch_gemm1/Gemm";
const std::string gemm2LeftOperation  = "/left_branch_gemm1/Gemm";
const std::string gemm2Left2Operation = "/left_branch_gemm2/Gemm";
const std::string gemm3Operation      = "/gemm2/Gemm";

/**
 * Class representing a neural network deployed in inference
*/
class NeuralNetwork
{
  public:

  /**
   * NeuralNetwork constructor
   * 
   * @param[in] tensors collection of pre-trained tensors
   * @param[in] operations collection of operations attributes
  */
  NeuralNetwork(tensorsMap_t &tensors, operationsMap_t &operations)
    : _tensors(tensors),
      _operations(operations)
  {}

  ~NeuralNetwork() = default;

  /**
   * Inference implementation
   * 
   * @param[in] input image provided as tensor
   * 
   * @return a tensor containing the result of the inference process
  */
  tensor_t forward(tensor_t &input)
  {
    // /Flatten
    const auto flattenAxis = _operations.at(flattenOperation).getIntAttribute(axis);
    flatten(input, flattenAxis);

    // /gemm1/Gemm
    auto &weight = _tensors.at(fc1Weight);
    auto &bias   = _tensors.at(fc1Bias);
    auto &gemm1  = _operations.at(gemm1Operation);
    gemm(input, weight, bias, gemm1);

    // /Relu
    relu(bias);

    // Duplicate the tensor
    // TODO: Avoid one copy
    auto  output0_left  = bias;
    auto  output0_right = output0_left;
    auto &result_left   = output0_left;
    auto &result_right  = output0_right;

    ///// left branch
    // /left_branch_gemm1/Gemm
    auto &weight_left = _tensors.at(fc2LeftWeight);
    auto &bias_left   = _tensors.at(fc2LeftBias);
    auto &gemm_left   = _operations.at(gemm2LeftOperation);
    gemm(result_left, weight_left, bias_left, gemm_left);
    result_left = bias_left;

    // /Relu_1
    relu(result_left);

    // /left_branch_gemm2/Gemm
    weight_left = _tensors.at(fc2Left2Weight);
    bias_left   = _tensors.at(fc2Left2Bias);
    gemm_left   = _operations.at(gemm2Left2Operation);
    gemm(result_left, weight_left, bias_left, gemm_left);
    result_left = bias_left;

    // /Relu_2
    relu(result_left);

    ///// right branch
    // /right_branch_gemm1/Gemm
    auto &weight_right = _tensors.at(fc2RightWeight);
    auto &bias_right   = _tensors.at(fc2RightBias);
    auto &gemm_right   = _operations.at(gemm2RightOperation);
    gemm(result_right, weight_right, bias_right, gemm_right);
    result_right = bias_right;

    // /Relu_1
    relu(result_right);

    ///// final steps
    // /Add
    add(result_left, result_right);

    // /gemm2/Gemm
    weight = _tensors.at(fc3Weight);
    bias   = _tensors.at(fc3Bias);
    gemm1  = _operations.at(gemm3Operation);
    gemm(result_left, weight, bias, gemm1);
    result_left = bias;

    return result_left;
  }

  private:

  /**
   * Collection of pre-trained tensors
  */
  tensorsMap_t &_tensors;

  /**
   * Collection of the neural network operations
  */
  operationsMap_t &_operations;

  /**
   * Flatten operation
   * 
   * @param[inout] input tensor to flatten
   * @param[in] axis dimension on which the flatten should be performed
  */
  void flatten(tensor_t &input, uint64_t axis)
  {
    // Throw exception if the shape is not 2D
    auto &tensorShape = input.getShape();
    if (axis >= tensorShape.size()) { HICR_THROW_RUNTIME("Axis out of range. Axis %d shape size: %d", axis, tensorShape.size()); }

    // Get number of tensor elements
    const uint64_t resultingDimension = input.size();

    // Perform the flatten
    if (axis == 0) { tensorShape = std::vector<uint64_t>({resultingDimension, 1}); }
    if (axis == 1) { tensorShape = std::vector<uint64_t>({1, resultingDimension}); }
  }

  /**
   * GEMM operation. The result is accumulated in parameter C
   * 
   * @param[in] A input tensor
   * @param[in] B operation weights
   * @param[inout] C operation bias
   * @param[in] operation collection of operation attributes
  */
  void gemm(const tensor_t &A, const tensor_t &B, tensor_t &C, const operation_t &operation)
  {
    // Get attributes
    auto alpha  = operation.getFloatAttribute(alphaAttribute);
    auto beta   = operation.getFloatAttribute(betaAttribute);
    auto transB = operation.getIntAttribute(transBAttribute);

    // Define M N K
    blasint M = A.rows();
    blasint N = B.rows();
    blasint K = B.columns();

    // Define leading dimension and whether B should be transposed
    blasint lda = K;
    blasint ldc = N;

    blasint         ldb;
    CBLAS_TRANSPOSE transposeB;
    if (transB == 1)
    {
      transposeB = CblasTrans;
      ldb        = K;
    }
    else
    {
      transposeB = CblasNoTrans;
      ldb        = N;
    }

    // Perform the GEMM
    cblas_sgemm(CblasRowMajor, CblasNoTrans, transposeB, M, N, K, alpha, A.toCFloat(), lda, B.toCFloat(), ldb, beta, C.toFloat(), ldc);
  }

  /**
   * ReLU operation
   * 
   * @param[inout] input
  */
  void relu(tensor_t &input)
  {
    const uint64_t resultingDimension = input.size();
    auto           data               = input.toFloat();
    for (uint64_t i = 0; i < resultingDimension; i++) { data[i] = std::max(0.0f, data[i]); }
  }

  /**
   * Add operation. The result is accumulated in parameter a
   * 
   * @param[inout] a
   * @param[in] b
  */
  void add(tensor_t &a, tensor_t &b) { std::transform(a.begin(), a.end(), b.begin(), a.begin(), std::plus<float>()); }
};