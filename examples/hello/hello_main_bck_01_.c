/****************************************************************************
 * apps/examples/hello/hello_main.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdio.h>

#include <nuttx/hrtimer.h>

#define HRTIMER_TEST_THREAD_NR (CONFIG_SMP_NCPUS * 8)

/****************************************************************************
 * Public Functions
 ****************************************************************************/

static uint32_t test_callback(FAR struct hrtimer_s *hrtimer)
{
  printf("test callback ...\n");
  return 0;
}

static void* test_thread(void *arg)
{
  irqstate_t  flags;
  hrtimer_t   timer;
  spinlock_t  lock = SP_UNLOCKED;
  hrtimer_init(&timer, test_callback, NULL);
  while (1)
    {
      uint64_t delay = rand() % NSEC_PER_MSEC;
      int ret;

      /* Simulate the usage of driver->wait_dog. */

      flags = spin_lock_irqsave(&lock);

      /* The driver lock acquired */

      /* First try, failed. Because hrtimer_start can not ensure the timer being started. */

      ret = hrtimer_cancel(&timer);
      // ret = hrtimer_start(&timer, 10 * NSEC_PER_USEC, HRTIMER_MODE_REL); /* May fail */

      /* This try-loop start should be OK. But it failed again.
       * Besides, we can not sleep or spin in the critical sections.
       */

      while (hrtimer_start(&timer, 10 * NSEC_PER_USEC, HRTIMER_MODE_REL) != OK);
      ret = OK;

      /* Second try, Success, but we can not sleep or spin in the critical section. */

      // ret = hrtimer_cancel_sync(&timer); /* Sleep in critical sections */
      // ret = hrtimer_start(&timer, delay, HRTIMER_MODE_REL);


      spin_unlock_irqrestore(&lock, flags);

      if (ret != OK)
        {
          printf("hrtimer_start failed\n");
        }
      up_ndelay(delay);
    }
  return NULL;
}

/****************************************************************************
 * hello_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  unsigned int   thread_id;
  pthread_attr_t attr;
  pthread_t      pthreads[HRTIMER_TEST_THREAD_NR];

  printf("hrtimer_test start...\n");

  ASSERT(pthread_attr_init(&attr) == 0);

  /* Create wdog test thread */

  for (thread_id = 0; thread_id < HRTIMER_TEST_THREAD_NR; thread_id++)
    {
      ASSERT(pthread_create(&pthreads[thread_id], &attr,
                            test_thread, NULL) == 0);
    }

  for (thread_id = 0; thread_id < HRTIMER_TEST_THREAD_NR; thread_id++)
    {
      pthread_join(pthreads[thread_id], NULL);
    }

  ASSERT(pthread_attr_destroy(&attr) == 0);

  printf("hrtimer_test end...\n");
  return 0;
}
