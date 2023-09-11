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
#include <hicr/task.hpp>
#include <hicr/dispatcher.hpp>

TEST(Dispatcher, Construction)
{
 // Creating by context
 EXPECT_NO_THROW(HiCR::Dispatcher([]() { return (HiCR::Task*)NULL; }));

 // Creating by new
 HiCR::Dispatcher* d;
 EXPECT_NO_THROW(d = new HiCR::Dispatcher([]() { return (HiCR::Task*)NULL; }));
 delete d;
}

TEST(Dispatcher, Pull)
{
 HiCR::Dispatcher d([]() { return (HiCR::Task*)NULL; });

 HiCR::Task* t;
 EXPECT_NO_THROW(t = d.pull());
 EXPECT_EQ(t, (HiCR::Task*)NULL);
}
