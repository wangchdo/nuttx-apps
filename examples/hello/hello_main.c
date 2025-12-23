#include <nuttx/config.h>
#include <stdio.h>

#include <nuttx/hrtimer.h>

#define HRTIMER_TEST(expr, value)                                   \
  do                                                                \
    {                                                               \
      ret = (expr);                                                 \
      if (ret != (value))                                           \
        {                                                           \
          printf("ERROR: HRTimer test failed, line=%d ret=%d\n",   \
                 __LINE__, ret);                                    \
        }                                                           \
    }                                                               \
  while (0)

#define HRTIMER_TEST_THREAD_NR (4)

/****************************************************************************
 * Public Functions
 ****************************************************************************/

static volatile int count = 0;
static hrtimer_t    timer;
static spinlock_t   lock = SP_UNLOCKED;

static uint32_t test_callback(FAR struct hrtimer_s *hrtimer)
{
  irqstate_t flags;
  flags = spin_lock_irqsave(&lock);
  up_ndelay(10 * NSEC_PER_USEC);
  count++;
  spin_unlock_irqrestore(&lock, flags);
  return 0;
}

static uint32_t test_callback_back(FAR struct hrtimer_s *hrtimer)
{
  return 0;
}

static void* test_thread(void *arg)
{
  irqstate_t  flags;
  int         tid   = (int)arg;
  int         ret;

  if (tid == 0)
    {
      hrtimer_init(&timer, test_callback, NULL);
    }

  while (1)
    {
      if (tid == 0)
        {
          int tmp_count;
          flags = spin_lock_irqsave(&lock);
          ret = hrtimer_cancel(&timer);
          ret = hrtimer_start(&timer, 0, HRTIMER_MODE_REL);
          spin_unlock_irqrestore(&lock, flags);

          up_ndelay(1000);

          spin_lock_irqsave(&lock);
          ret = hrtimer_cancel(&timer);
          ret = hrtimer_start(&timer, 0, HRTIMER_MODE_REL);
          spin_unlock_irqrestore(&lock, flags);

          ret = hrtimer_cancel_sync(&timer); // timer should be fully-cancelled here.
          tmp_count = count; // Read the count
          printf("tmp_count: %d\n",tmp_count);
          up_ndelay(10 * NSEC_PER_USEC);
          HRTIMER_TEST(tmp_count == count, true);
          ASSERT(tmp_count == count);
        }
      else
        {
          flags = up_irq_save();
          up_irq_restore(flags);
        }
    }

  UNUSED(ret);

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