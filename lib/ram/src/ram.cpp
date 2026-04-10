#include "ram.h"


namespace toaster {


void *malloc_auto(size_t size) {
#if CONFIG_SPIRAM_SUPPORT || CONFIG_SPIRAM
  void *ptr = ps_malloc(size);
  return (ptr != nullptr) ? ptr : malloc(size);
#else
  return malloc(size);
#endif
}


void free_auto(void *ptr) {
  free(ptr);
}


};
