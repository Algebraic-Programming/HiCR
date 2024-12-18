/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file common.hpp
 * @brief Provides utilities commonly used across the LPF backend
 * @author K. Dichev, O. Korakitis
 * @date 18/12/2024
 */
#pragma once

/**
 * #CHECK(f...) Checks if an LPF function returns LPF_SUCCESS, else
 * it prints an error message
 */
#define CHECK(f...)                                                                                                                                                                \
  if (f != LPF_SUCCESS) { HICR_THROW_RUNTIME("LPF Backend Error: '%s'", #f); }
