/*
 * Copyright Huawei Technologies Switzerland AG
 * All rights reserved.
 */

/**
 * @file common.hpp
 * @brief This file consists of the common nOS-V function used for all the backend implementations
 * @author N. Baumann and L. Terracciano
 * @date 9/1/2025
 */

 #pragma once

 #include <cstdio>
 #include <cstdlib>
 #include <pthread.h>
 #include <nosv.h>
 #include <nosv/hwinfo.h>
 
 #include <hicr/core/exceptions.hpp>
 
 /**
  * Check nOS-V error. Terminate the program if something goes bad
  * 
  * @param[in] error nOS-V error code
 */
 inline void check(int error)
 {
   if (error == NOSV_SUCCESS) { return; }
 
   HICR_THROW_RUNTIME("nOS-V Error: %s\n", nosv_get_error_string(error));
 }
 
 /**
  * Get nOS-V task's metadata
  * 
  * @param[in] task 
  * 
  * @return task's metadata
  *
 */
 inline void *getTaskMetadata(nosv_task_t task)
 {
   auto metadata = nosv_get_task_metadata(task);
   if (metadata == NULL) HICR_THROW_RUNTIME("nOS-V metadata task returned NULL:\n");
   return metadata;
 }
 
 /**
  * Get nOS-V task type's metadata
  * 
  * @param[in] task 
  * 
  * @return task's metadata
  *
 */
 inline void *getTaskTypeMetadata(nosv_task_t task)
 {
   auto metadata = nosv_get_task_type_metadata(nosv_get_task_type(task));
   if (metadata == NULL) HICR_THROW_RUNTIME("nOS-V metadata task type returned NULL:\n");
   return metadata;
 }
 
 /**
  * Print current CPU and Thread ID (For debugging)
  */
 inline void print_CPU_TID()
 {
   printf("[CPU: %d Thread: %ld] ", nosv_get_current_logical_cpu(), pthread_self());
 
   fflush(stdout);
 }
 
 /**
  * Print but with CPU and TID (For debugging)
  */
 inline void print(const char* message)
 {
   print_CPU_TID();
   printf("%s\n", message);
 
   fflush(stdout);
 }