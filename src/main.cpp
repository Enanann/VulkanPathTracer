#include "application.hpp"
#include "log.hpp"

#include <exception>

int main() {
    try {
        Application app;
        app.run();
    } catch (const std::exception& e) {
        LOGE("{}", e.what());
        return 1;
    }

    LOGI("Program exited successfully");

    return 0;
}
