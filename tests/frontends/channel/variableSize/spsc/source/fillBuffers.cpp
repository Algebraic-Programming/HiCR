#include <fstream>
#include <mpi.h>
#include <nlohmann_json/json.hpp>

#include <hicr/backends/mpi/communicationManager.hpp>
#include <hicr/backends/mpi/memoryManager.hpp>
#include <hicr/backends/mpi/instanceManager.hpp>
#include <hicr/backends/hwloc/topologyManager.hpp>

#include "channelFixture.hpp"
#include "producer.hpp"
#include "consumer.hpp"

TEST_F(ChannelFixture, fillBufferCounter)
{
  // Rank 0 is producer, Rank 1 is consumer
  if (_instanceManager->getCurrentInstance()->isRootInstance()) { producerFc(*_memoryManager, *_communicationManager, *_communicationManager, _memorySpace, *_producer); }
  else { consumerFc(*_communicationManager, *_communicationManager, *_consumer); }
}
