#pragma once

#include <algorithm>
#include <cblas.h>

#include <hicr/core/exceptions.hpp>
#include <hicr/core/executionState.hpp>
#include <hicr/core/executionUnit.hpp>
#include <hicr/core/processingUnit.hpp>
#include <hicr/core/computeManager.hpp>

#include "../../proto/onnx.proto3.pb.h"
#include "./factory/executionUnit/executionUnitFactory.hpp"
#include "./argumentsFactory.hpp"
#include "./tensor/tensor.hpp"
#include "./operation.hpp"

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
   * @param[in] computeManager HiCR compute manager
   * @param[in] computeResource HiCR compute resource 
   * @param[in] communication HiCR communication manager
   * @param[in] memoryManager HiCR memory manager
   * @param[in] memorySpace HiCR memory space 
  */
  NeuralNetwork(HiCR::ComputeManager                  &computeManager,
                std::unique_ptr<HiCR::ProcessingUnit> processingUnit,
                HiCR::CommunicationManager            &communicationManager,
                HiCR::MemoryManager                   &memoryManager,
                std::shared_ptr<HiCR::MemorySpace>    &memorySpace,
                factory::ExecutionUnitFactory             &executionUnitFactory,
                tensorFactoryFc_t                          tensorFactoryFunction,
                tensorCloneFc_t                            tensorCloneFunction)
    : _communicationManager(communicationManager),
      _computeManager(computeManager),
      _memoryManager(memoryManager),
      _processingUnit(std::move(processingUnit)),
      _memorySpace(memorySpace),
      _executionUnitFactory(executionUnitFactory),
      _tensorFactoryFunction(tensorFactoryFunction),
      _tensorCloneFunction(tensorCloneFunction)
  {}

  ~NeuralNetwork()
  {
    // Free memory slots containing the pre-trained data
    for (auto &[_, tensor] : _tensors) { _memoryManager.freeLocalMemorySlot(tensor->getData()); }
  }

  /**
   * Inference implementation
   * 
   * @param[in] input image provided as tensor
   * 
   * @return a tensor containing the result of the inference process
  */
  std::shared_ptr<tensor_t> forward(std::shared_ptr<tensor_t> input)
  {
    // /gemm1/Gemm
    auto output = gemm(input, fc1Weight, fc1Bias, gemm1Operation);

    // /Relu
    output = relu(output);

    // Clone the result to use it as input in the left and right branch
    auto rightClone  = _tensorCloneFunction(*output, _memoryManager, _memorySpace, _communicationManager);
    auto leftClone   = _tensorCloneFunction(*output, _memoryManager, _memorySpace, _communicationManager);
    auto leftBranch  = leftClone;
    auto rightBranch = rightClone;

    ///// left branch
    // /left_branch_gemm1/Gemm
    leftBranch = gemm(leftBranch, fc2LeftWeight, fc2LeftBias, gemm2LeftOperation);

    // Free original clone of output variable
    _memoryManager.freeLocalMemorySlot(leftClone->getData());

    // /Relu_1
    leftBranch = relu(leftBranch);

    // /left_branch_gemm2/Gemm
    leftBranch = gemm(leftBranch, fc2Left2Weight, fc2Left2Bias, gemm2Left2Operation);

    // /Relu_2
    leftBranch = relu(leftBranch);

    ///// right branch
    // /right_branch_gemm1/Gemm
    rightBranch = gemm(rightBranch, fc2RightWeight, fc2RightBias, gemm2RightOperation);

    // Free original clone of output variable
    _memoryManager.freeLocalMemorySlot(rightClone->getData());

    // /Relu_3
    rightBranch = relu(rightBranch);

    // /VectorAdd
    leftBranch = vectorAdd(leftBranch, rightBranch);

    // /gemm2/Gemm
    leftBranch = gemm(leftBranch, fc3Weight, fc3Bias, gemm3Operation);

    return leftBranch;
  }

  /**
 * Load from a pre-trained ONNX model the relevant informations for the network
 * 
 * @param[in] model the ONNX model
 * @param[in] hostMemorySpace The host memory space in which data should be temporarily copied from the file 
 * 
*/
  void loadPreTrainedData(const onnx::ModelProto &model, const std::shared_ptr<HiCR::MemorySpace> &hostMemorySpace)
  {
    // Get the graph from the model
    const auto &graph = model.graph();

    // Iterate through each node in the graph
    for (const auto &node : graph.node())
    {
      operation::attributes_t attributes;

      // Get the node attributes
      for (const auto &attribute : node.attribute())
      {
        if (attribute.type() == onnx::AttributeProto_AttributeType_FLOAT) { attributes[attribute.name()] = attribute.f(); }
        else if (attribute.type() == onnx::AttributeProto_AttributeType_INT) { attributes[attribute.name()] = attribute.i(); }
        else { HICR_THROW_RUNTIME("Unsupported attribute"); }
      }

      _operations.emplace(node.name(), operation_t(attributes));

      // Check if the node has any initializers (i.e., weights or biases)
      for (const auto &initializer : graph.initializer())
      {
        const auto initializerName = initializer.name();

        // Look into the inputs to retrieve any pre-trained data
        if (std::find(node.input().begin(), node.input().end(), initializerName) != node.input().end())
        {
          // Get the shape
          std::vector<uint64_t> shape;
          for (const auto dim : initializer.dims()) { shape.emplace_back(dim); }

          // Extract the data in a float vector
          size_t tensorSize    = (std::accumulate(shape.begin(), shape.end(), 1, std::multiplies<>())) * sizeof(float);
          auto   srcMemorySlot = _memoryManager.registerLocalMemorySlot(hostMemorySpace, (void *)initializer.raw_data().data(), tensorSize);
          auto   dstMemorySlot = _memoryManager.allocateLocalMemorySlot(_memorySpace, tensorSize);
          _communicationManager.memcpy(dstMemorySlot, 0, srcMemorySlot, 0, tensorSize);
          _memoryManager.deregisterLocalMemorySlot(srcMemorySlot);

          _tensors.emplace(initializerName, _tensorFactoryFunction(shape, dstMemorySlot));
        }
      }
    }
  }

  /**
   * @return index of the max element in the tensor
  */
  uint64_t getPrediction(const std::shared_ptr<HiCR::LocalMemorySlot> predictionMemSLot, uint64_t numberOfElements)
  {
    const auto data = (const float *)predictionMemSLot->getPointer();
    return std::distance(data, std::max_element(data, data + numberOfElements));
  }

  std::unique_ptr<HiCR::ProcessingUnit> releaseProcessingUnit() { return std::move(_processingUnit); }

  private:

  /**
   * Collection of pre-trained tensors
  */
  tensorsMap_t _tensors;

  /**
   * Collection of the neural network operations
  */
  operationsMap_t _operations;

  /**
   * Communication manager
  */
  HiCR::CommunicationManager &_communicationManager;

  /**
   * Compute manager
  */
  HiCR::ComputeManager &_computeManager;

  /**
   * Memory manager
  */
  HiCR::MemoryManager &_memoryManager;

  /**
   * Compute resource
  */
  std::unique_ptr<HiCR::ProcessingUnit> _processingUnit;

  /**
   * Memory space
  */
  std::shared_ptr<HiCR::MemorySpace> _memorySpace;

  /**
  * Collection of exection units for the neural network operations 
  */
  factory::ExecutionUnitFactory &_executionUnitFactory;

  /**
   * Function for creating a new Tensor
   */
  tensorFactoryFc_t _tensorFactoryFunction;

  /**
   * Function for cloning a new Tensor
   */
  tensorCloneFc_t _tensorCloneFunction;

  /**
   * Factory to create arguments for each operations
  */
  ArgumentsFactory _argumentsFactory;

  /**
   * GEMM execution
   * 
   * @param[in] input the input tensor (A)
   * @param[in] weigtName the name of the weight tensor (B)
   * @param[in] biasName the name of the bias tensor (C)
   * @param[in] operationName the name of the gemm operation to retrieve the attributes (alpha, beta, etc.
   * 
   * @return the tensor containing the GEMM result
   */
  std::shared_ptr<tensor_t> gemm(const std::shared_ptr<tensor_t> input, const std::string &weightName, const std::string &biasName, const std::string &operationName)
  {
    // Retrieve the weigt and bias tensors
    auto &weight = _tensors.at(weightName);
    auto &bias   = _tensors.at(biasName);

    // Retrieve the GEMM attributes
    auto &operation = _operations.at(operationName);

    // Construct the arguments to pass to the GEMM
    const auto args = _argumentsFactory.gemm(input, weight, bias, operation);

    // Execute the GEMM
    executeOperation(_executionUnitFactory.gemm(args));

    // The result is stored in bias (C)
    return bias;
  }

  /**
   * ReLU execution
   * 
   * @param[in] input the input tensor. It stores the result of the operation
   * 
   * @return the tensor containing the ReLU result
   */
  std::shared_ptr<tensor_t> relu(std::shared_ptr<tensor_t> input)
  {
    // Construct the ReLU arguments
    const auto args = _argumentsFactory.relu(input);

    // Execute the ReLU
    executeOperation(_executionUnitFactory.relu(args));

    // The result is store in the input tensor
    return input;
  }

  /**
   * VectorAdd execution
   * 
   * @param[in] a the first tensor. It stores the result of the operation
   * @param[in] b the second tensor
   * 
   * @return the tensor containing the ReLU result
   */
  std::shared_ptr<tensor_t> vectorAdd(std::shared_ptr<tensor_t> a, const std::shared_ptr<tensor_t> b)
  {
    const auto args = _argumentsFactory.vectorAdd(a, b);
    executeOperation(_executionUnitFactory.vectorAdd(args));
    return a;
  }

  /**
   * Execute an operation wrapped in a HiCR execution unit
   * 
   * @param[in] executionUnit the execution unit to run
  */
  __INLINE__ void executeOperation(std::shared_ptr<HiCR::ExecutionUnit> executionUnit)
  {
    // Create an exeuction state
    auto executionState = _computeManager.createExecutionState(executionUnit);

    // Initialize the processing unit
    _computeManager.initialize(_processingUnit);

    // Start the execution state
    _computeManager.start(_processingUnit, executionState);

    // Send termination signal to the execution state
    _computeManager.terminate(_processingUnit);

    // Wait for completion
    _computeManager.await(_processingUnit);
  }
};