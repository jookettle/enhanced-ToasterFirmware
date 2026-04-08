#include "hal/display/display.h"

namespace toaster {

class SimulatedDisplay : public Display {
public:
    bool begin(int width, int height) override {
        bool result = Display::begin(width, height);
        simulator_log_append("SimulatedDisplay initialized\n");
        return result;
    }

    void endDraw() override {
        // Here we would ideally render to a window.
        // For now, let's just print a small notification every 100 frames to show it's working.
        static int frames = 0;
        if (++frames % 100 == 0) {
            simulator_log_append("SimulatedDisplay frame tick\n");
        }
    }
};

}
