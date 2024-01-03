/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file dispatcher.cpp
 * @brief Unit tests for the dispatcher class
 * @author S. M. Martin
 * @date 11/9/2023
 */

#include "gtest/gtest.h"
#include <frontends/taskr/dispatcher.hpp>
#include <frontends/taskr/task.hpp>

TEST(Dispatcher, Construction)
{
  // Creating by context
  EXPECT_NO_THROW(taskr::Dispatcher([]()
                                    { return (taskr::Task *)NULL; }));

  // Creating by new
  taskr::Dispatcher *d = NULL;
  EXPECT_NO_THROW(d = new taskr::Dispatcher([]()
                                            { return (taskr::Task *)NULL; }));
  delete d;
}

TEST(Dispatcher, Pull)
{
  taskr::Dispatcher d([]()
                      { return (taskr::Task *)NULL; });
  taskr::Task *t = NULL;
  EXPECT_NO_THROW(t = d.pull());
  EXPECT_EQ(t, (taskr::Task *)NULL);
}
