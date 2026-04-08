#include "native_app.h"
#include "protogen.h"

#include <atomic>
#include <chrono>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

using namespace toaster;

namespace toaster {
void native_request_gui_close();
}

namespace {

struct NativeKeyEvent {
    int key{0};
    uint8_t mode{0};
};

std::mutex g_input_mutex;
std::deque<NativeKeyEvent> g_input_queue;
std::atomic<bool> g_running{false};
std::thread g_simulation_thread;

void dispatch_key(int ch) {
    switch (ch) {
    case 'q':
    case 'Q':
    case 27:
        break;
    case 13:
        Protogen._hud.pressKey(VK_RIGHT, KM_KEYCODE);
        break;
    case '0':
        Protogen.setEmotion("blank");
        break;
    case '1':
        Protogen.setEmotion("normal");
        break;
    case '2':
        Protogen.setEmotion("badass");
        break;
    case '3':
        Protogen.setEmotion("grin");
        break;
    case '4':
        Protogen.setEmotion("worry");
        break;
    case '5':
        Protogen.setEmotion("love");
        break;
    case '6':
        Protogen.setEmotion("confusion");
        break;
    case '7':
        Protogen.setEmotion("qmark");
        break;
    case '8':
        Protogen.setEmotion("unyuu");
        break;
    case '9':
        Protogen.setEmotion("bean");
        break;
    case 'P':
    case 'p':
        Protogen.setEmotion("bsod");
        break;
    case 'W':
    case 'w':
        Protogen.setEmotion("white");
        break;
    case 'O':
    case 'o':
        Protogen.setEmotion("festive");
        break;
    case '\'':
        Protogen.setEmotion("loading");
        break;
    case 'Z':
    case 'z':
        Protogen.setColorMode(PCM_ORIGINAL);
        break;
    case 'X':
    case 'x':
        Protogen.setColorMode(PCM_PERSONAL);
        break;
    case 'C':
    case 'c':
        Protogen.setColorMode(PCM_RAINBOW_SINGLE);
        break;
    case 'V':
    case 'v':
        Protogen.setColorMode(PCM_RAINBOW);
        break;
    case 'B':
    case 'b':
        Protogen.setColorMode(PCM_GRADATION);
        break;
    case 'E':
    case 'e':
        Protogen.setStaticMode(!Protogen.getStaticMode());
        break;
    case 'M':
    case 'm':
        Protogen._boopsensor.setDebug(!Protogen._boopsensor.getDebug());
        break;
    case '\t':
        Protogen.setEmotionNext();
        break;
    case '\\':
        Protogen._hud.setDithering(!Protogen._hud.getDithering());
        break;
    case 'A':
    case 'a':
    case 'S':
    case 's':
    case 'D':
    case 'd':
    case 'F':
    case 'f':
    case 'J':
    case 'j':
    case 'K':
    case 'k':
    case 'L':
    case 'l':
    case ';':
    case ':':
        Protogen._hud.pressKey(ch, KM_ASCII);
        break;
    default:
        if (ch == 0 || ch == 224) {
            break;
        }
        break;
    }
}

void drain_input_queue() {
    std::deque<NativeKeyEvent> queue;
    {
        std::lock_guard<std::mutex> lock(g_input_mutex);
        queue.swap(g_input_queue);
    }

    for (const auto& event : queue) {
        if (event.mode == KM_KEYCODE) {
            Protogen._hud.pressKey(event.key, KM_KEYCODE);
        }
        else {
            dispatch_key(event.key);
        }
    }
}

void simulation_loop() {
    toaster::simulator_log_append("Starting Protogen Native GUI Simulator...\n");
    toaster::simulator_log_append("Press q or Esc to quit.\n");
    toaster::simulator_log_append("Controls: 1-9 emotions, F/D/S/J/K/L menu, arrows in menus, TAB next emotion.\n");

    if (!Protogen.begin()) {
        toaster::simulator_log_append("Failed to initialize Protogen!\n");
        g_running = false;
        native_request_gui_close();
        return;
    }

    toaster::simulator_log_append("Protogen initialized successfully.\n");

    while (g_running.load()) {
        drain_input_queue();
        Protogen.loop();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

}

namespace toaster {

void native_enqueue_key(int key, uint8_t mode) {
    std::lock_guard<std::mutex> lock(g_input_mutex);
    g_input_queue.push_back({key, mode});
}

void native_start_simulation() {
    bool expected = false;
    if (!g_running.compare_exchange_strong(expected, true)) {
        return;
    }

    if (g_simulation_thread.joinable()) {
        g_simulation_thread.join();
    }

    g_simulation_thread = std::thread(simulation_loop);
}

void native_stop_simulation() {
    g_running = false;
}

void native_wait_for_simulation() {
    if (g_simulation_thread.joinable()) {
        g_simulation_thread.join();
    }
}

bool native_is_running() {
    return g_running.load();
}

}
