#pragma once

#include <cstddef>
#include <cstdint>

namespace toaster {

struct NativeMemoryStats {
  uintmax_t working_set{0};
  uintmax_t private_bytes{0};
  bool valid{false};
};

NativeMemoryStats native_query_memory_stats();

}
