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
#include <unistd.h>

#include <nuttx/hrtimer.h>

#define HRTIMER_TEST_THREAD_NR (1)
#define HRTIMER_TEST_NR        (1000000)

/****************************************************************************
 * Public Functions
 ****************************************************************************/

static int volatile tcount = 0;
static volatile uint32_t next = 0;

static uint32_t test_callback(FAR struct hrtimer_s *hrtimer)
{

  tcount++;
  up_ndelay(hrtimer->expired % (10 * NSEC_PER_USEC));

  return 0;
}

static uint32_t test_callback_background(FAR struct hrtimer_s *hrtimer)
{
  up_ndelay(hrtimer->expired % NSEC_PER_USEC);
  return 0;
}


static void test1(int tid)
{
  hrtimer_t   timer;
  int         count = 0;
  irqstate_t flags;
  spinlock_t lock;

  if (tid == 0)
    {
      hrtimer_init(&timer, test_callback, NULL);
    }
  else
    {
      hrtimer_init(&timer, test_callback_background, NULL);
    }

  while (count++ < HRTIMER_TEST_NR)
    {
      int ret;
      if (tid == 0)
        {
          uint64_t delay = rand() % (10 * NSEC_PER_MSEC);

          /* Simulate the periodical hrtimer.. */

          flags = spin_lock_irqsave(&lock);

          /* Use as periodical timer */

          ret = hrtimer_cancel(&timer);
          ret = hrtimer_start(&timer, 1000, HRTIMER_MODE_REL);

          spin_unlock_irqrestore(&lock, flags);

          up_ndelay(NSEC_PER_MSEC);

          flags = spin_lock_irqsave(&lock);

          ret = hrtimer_cancel_sync(&timer);
          ret = hrtimer_start(&timer, 1000, HRTIMER_MODE_REL);
          spin_unlock_irqrestore(&lock, flags);

          up_ndelay(NSEC_PER_MSEC);

          hrtimer_cancel_sync(&timer); // stucked here????
          printf("???\n");
        }
      else
        {
          /* Simulate the background hrtimer.. */

          uint64_t delay = rand() % (10 * NSEC_PER_MSEC);

          ret = hrtimer_cancel(&timer);
          ret = hrtimer_start(&timer, delay, HRTIMER_MODE_REL);
        }

      UNUSED(ret);
    }
}

static void* test_thread(void *arg)
{
  while (1)
    {
      test1((int)arg);
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
                            test_thread, (void *)thread_id) == 0);
    }

  for (thread_id = 0; thread_id < HRTIMER_TEST_THREAD_NR; thread_id++)
    {
      pthread_join(pthreads[thread_id], NULL);
    }

  ASSERT(pthread_attr_destroy(&attr) == 0);

  printf("hrtimer_test end...\n");
  return 0;
}