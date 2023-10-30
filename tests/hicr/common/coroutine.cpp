/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file coroutine.cpp
 * @brief Unit tests for the coroutine class
 * @author S. M. Martin
 * @date 7/9/2023
 */

#include "gtest/gtest.h"
#include <hicr/common/coroutine.hpp>
#include <pthread.h>
#include <unistd.h>

TEST(Coroutine, Construction)
{
  auto c = new HiCR::common::Coroutine();
  EXPECT_FALSE(c == nullptr);
}

// Defines the number of coroutine to use in the test
#define COROUTINE_COUNT 8

// Defines the number of times a coroutine will be resumed by each thread
#define RESUME_COUNT 100

// Defines the number of threads to use in the test
#define THREAD_COUNT 16

// Storage for thread-local identification of running thread
static pthread_key_t key;
static pthread_once_t key_once = PTHREAD_ONCE_INIT;

// Declaring a barrier. It is important to make sure the two threads are alive while the coroutine is being used
pthread_barrier_t _barrier;

// Mutex to ensure the threads do not execute the same coroutine at the same time
std::vector<std::mutex *> _mutexes;

// Flag to store whether the execution failed or not
bool falseRead = false;

// Storage for the coroutine array
HiCR::common::Coroutine *coroutines[COROUTINE_COUNT];

static void make_key() { (void) pthread_key_create(&key, NULL); }

/**
 * Sets up new affinity for the thread. The thread needs to yield or be preempted for the new affinity to work.
 *
 * \param[in] affinity New affinity to use
 */
void updateAffinity(const std::set<int> &affinity)
{
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  for (const auto c : affinity) CPU_SET(c, &cpuset);
  if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) != 0) HICR_THROW_RUNTIME("Problem assigning affinity.");
}

void *threadFc(void *arg)
{
 void *ptr;

 pthread_once(&key_once, make_key);

 if ((ptr = pthread_getspecific(key)) == NULL)
 {
     ptr = malloc(sizeof(pthread_t));
     *((pthread_t*)ptr) = pthread_self();
     (void) pthread_setspecific(key, ptr);
 }

  // Getting thread id
  size_t threadId = (size_t)arg;

  // Setting initial thread affinity
  updateAffinity(std::set<int>({(int)threadId}));

  // Yielding execution to allow affinity to refresh
  sched_yield();

  // Storing thread-local value
  threadId = pthread_self();

  // Waiting for all threads to have started
  pthread_barrier_wait(&_barrier);

  // Resuming coroutines many times
  for (size_t i = 0; i < RESUME_COUNT; i++)
    for (size_t c = 0; c < COROUTINE_COUNT; c++)
    {
      _mutexes[c]->lock();
      coroutines[c]->resume();
      _mutexes[c]->unlock();
    }

  // Waiting for all other threads to finish
  pthread_barrier_wait(&_barrier);

  return NULL;
}

/*
 *  This is a stress test that combines coroutines, thread-level storage and pthreads to make sure TLS does never get corrupted when
 *  a coroutine is started and resumed by multiple different pthreads.
 */
TEST(Coroutine, TLS)
{
  // Creating new HiCR coroutine
  for (size_t i = 0; i < COROUTINE_COUNT; i++) coroutines[i] = new HiCR::common::Coroutine();

  // Creating per-coroutine mutexes
  _mutexes.resize(COROUTINE_COUNT);
  for (size_t i = 0; i < COROUTINE_COUNT; i++) _mutexes[i] = new std::mutex;

  // Creating coroutine function
  auto fc = [](void *arg)
  {
    // Recovering a pointer to the coroutine
    auto coroutine = (HiCR::common::Coroutine *)arg;

    // Executing coroutine yield cycle as many times as necessary
    while (true)
    {
      // Yielding
      coroutine->yield();

      // Getting self reference
      pthread_t* selfReference = (pthread_t*)pthread_getspecific(key);

      // Making sure the TLS registers the correct thread as the one reported by the OS
      if (*selfReference != pthread_self()) falseRead = true;
    }
  };

  // Starting coroutines
  for (size_t i = 0; i < COROUTINE_COUNT; i++) coroutines[i]->start([i, fc]()
                                                                    { fc(coroutines[i]); });

  // Initializing barrier
  pthread_barrier_init(&_barrier, NULL, THREAD_COUNT);

  // Storage for thread ids
  pthread_t threadIds[THREAD_COUNT];

  // Creating threads
  for (size_t i = 0; i < THREAD_COUNT; i++) pthread_create(&threadIds[i], NULL, threadFc, (void*)i);

  // Waiting for threads to finish
  for (size_t i = 0; i < THREAD_COUNT; i++) pthread_join(threadIds[i], NULL);

// Since coverage inteferes with this test on Ubuntu 20.04 / gcc 12, we bypass this check
#if !(defined __GNUC__ && defined ENABLE_COVERAGE)
    // Asserting whether there was any false reads
    //ASSERT_FALSE(falseRead);
#endif
}
