#include "ValveController.h"
#include <chrono>
#include <iostream>
#include <thread>
/**
 * @file example_forceController.cpp
 * @brief This program shows an example usage of the ForceController.
 */
void wait() { std::this_thread::sleep_for(std::chrono::milliseconds(2000)); }
int main() {
    ValveController forceController(16, 1000);
    for (int i = 0; i < 20; i++) {
        forceController.setSinglePressure(i % (N_SEGMENTS*N_CHAMBERS)+4, 300);
        wait();
        forceController.setSinglePressure(i % (N_SEGMENTS*N_CHAMBERS)+4, 0);
        //forceController.setSinglePressure(i % 12, 0);
        //forceController.setSinglePressure((i + 2) % 12, 0);
    }

    wait();
    forceController.disconnect();

    return 1;
}