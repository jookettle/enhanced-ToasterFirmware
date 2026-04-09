#include "native_memory.h"

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#endif

namespace toaster {

NativeMemoryStats native_query_memory_stats() {
  NativeMemoryStats stats{};

#ifdef _WIN32
  PROCESS_MEMORY_COUNTERS_EX counters{};
  counters.cb = sizeof(counters);
  if (GetProcessMemoryInfo(
      GetCurrentProcess(),
      reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counters),
      sizeof(counters))) {
    stats.working_set = static_cast<uintmax_t>(counters.WorkingSetSize);
    stats.private_bytes = static_cast<uintmax_t>(counters.PrivateUsage);
    stats.valid = true;
  }
#endif

  return stats;
}

}
