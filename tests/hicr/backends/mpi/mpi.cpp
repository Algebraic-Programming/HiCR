/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file mpi.cpp
 * @brief Unit tests for the MPI backend class
 * @author S. M. Martin
 * @date 11/9/2023
 */

#include "gtest/gtest.h"
#include <hicr/backends/mpi/mpi.hpp>
#include <limits>

namespace backend = HiCR::backend::mpi;

TEST(MPI, Construction)
{
  int argc = 0;
  char **argv = NULL;

  MPI_Init(&argc, &argv);
  backend::MPI *b = NULL;

  EXPECT_NO_THROW(b = new backend::MPI(MPI_COMM_WORLD));
  EXPECT_FALSE(b == nullptr);
  delete b;
}
