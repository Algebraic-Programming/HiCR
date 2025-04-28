#pragma once

#include <string>
#include <vector>

#include <hicr/core/exceptions.hpp>

#include "../../proto/onnx.proto3.pb.h"
#include "tensor.hpp"
#include "operation.hpp"

/**
 * Load from a pre-trained ONNX model the relevant informations for the network
 * 
 * @param[in] model the ONNX model
 * 
 * @return a pair containing a collection of pre-trained tensors (e.g., weights, biases), 
 * and a collection of operation that store the operation attributes (e.g., for GEMM: alpha, beta, etc.)
*/
std::pair<tensorsMap_t, operationsMap_t> extractNetworkInformations(const onnx::ModelProto &model)
{
  // Map to store node details
  tensorsMap_t tensors;

  // Map to store graph operations
  operationsMap_t operations;

  // Get the graph from the model
  const auto &graph = model.graph();

  // Iterate through each node in the graph
  for (const auto &node : graph.node())
  {
    attributes_t attributes;

    // Get the node attributes
    for (const auto &attribute : node.attribute())
    {
      if (attribute.type() == onnx::AttributeProto_AttributeType_FLOAT) { attributes[attribute.name()] = attribute.f(); }
      else if (attribute.type() == onnx::AttributeProto_AttributeType_INT) { attributes[attribute.name()] = attribute.i(); }
      else { HICR_THROW_RUNTIME("Unsupported attribute"); }
    }

    operations.emplace(node.name(), operation_t(attributes));

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
        float *rawTensorValues = (float *)initializer.raw_data().data();
        size_t tensorSize      = std::accumulate(shape.begin(), shape.end(), 1, std::multiplies<>());
        auto   tensorValues    = std::vector<float>(rawTensorValues, rawTensorValues + tensorSize);

        tensors.emplace(initializerName, tensor_t(shape, tensorValues));
      }
    }
  }

  return std::make_pair<>(tensors, operations);
}