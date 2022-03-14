#include "tbb/task_scheduler_init.h"

int main()
{
  [[maybe_unused]] tbb::task_scheduler_init task_scheduler;
  return 0;
}
