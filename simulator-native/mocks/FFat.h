#pragma once
#include "SD.h"

// FFat mock - extends FS just like SD
namespace fs {
class FFatClass : public FS {
public:
  bool begin(bool formatOnFail = false) { (void)formatOnFail; return true; }
};
}
extern fs::FFatClass FFat;
