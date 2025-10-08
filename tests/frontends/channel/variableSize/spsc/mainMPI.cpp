#include <gtest/gtest.h>
#include <mpi.h>

#include <thread>
#include <chrono>

class MPITestEnvironment : public ::testing::Environment
{
  void TearDown() override
  {
    int result = ::testing::UnitTest::GetInstance()->failed_test_count();
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (result > 0)
    {
      std::this_thread::sleep_for(std::chrono::seconds(1700));
      fprintf(stderr, "[Rank %d] Test failed, aborting MPI.\n", rank);
      MPI_Abort(MPI_COMM_WORLD, 1);
    }
  }
};

int main(int argc, char **argv)
{
  MPI_Init(&argc, &argv);
  ::testing::InitGoogleTest(&argc, argv);
  ::testing::AddGlobalTestEnvironment(new MPITestEnvironment);
  int result = RUN_ALL_TESTS();
  MPI_Finalize();
  return result;
}
