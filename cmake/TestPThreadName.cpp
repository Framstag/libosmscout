#include <type_traits>
#include <thread>

#include <pthread.h>

int main()
{
  static_assert(std::is_same<std::thread::native_handle_type, pthread_t>::value, "std::thread::native_handle_type have to be pthread_t");

  return pthread_setname_np(pthread_self(), "Test") == 0;
}
