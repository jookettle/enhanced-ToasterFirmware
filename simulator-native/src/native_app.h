#pragma once

#include <cstdint>

namespace toaster {

void native_enqueue_key(int key, uint8_t mode);
void native_start_simulation();
void native_stop_simulation();
void native_wait_for_simulation();
bool native_is_running();
void native_request_gui_close();

}
