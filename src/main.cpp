#include <TeensyThreads.h>
#include <atomic>
#include <cstdint>
#include <cstdio>

#include "Arduino.h"

// Define struct to store each sample saved in thread 1
// to be read by thread 2.
struct Sample {
    uint32_t x;
    uint32_t y;
};

static const int DEFAULT_STACK_SIZE = 128;

static Sample sample;
static std::atomic<bool> sampleReady;
static Threads::Mutex mutex;

// Prototypes for thread body functions
static void t1_thread_func(void* arg);
static void t2_thread_func(void* arg);
   
// For reference, API for Mutex class
/*
    int getState(); // get the lock state; 1=locked; 0=unlocked
    int lock(unsigned int timeout_ms = 0); // lock, optionally waiting up to timeout_ms milliseconds
    int try_lock(); // if lock available, get it and return 1; otherwise return 0
    int unlock();   // unlock if locked
*/
void setup()
{
    const int t1_id = threads.addThread(t1_thread_func, nullptr, DEFAULT_STACK_SIZE);
    if (t1_id == -1) {
        Serial.println("error creating thread t1");
    }

    const int t2_id = threads.addThread(t2_thread_func, nullptr, DEFAULT_STACK_SIZE);
    if (t2_id == -1) {
        Serial.println("error creating thread t2");
    }
}

void loop()
{
    // Nothing to do in standard Ardunio loop function now
    yield();
}

static void t1_thread_func(void* arg) {
    static int c;
    while (true) {
        while (mutex.try_lock() == 0) {
            threads.yield();
        }
        sample.x = c;
        sample.y = ++c / 2;
        mutex.unlock();
        sampleReady = true; 
        threads.delay(500);
    }
}

static void t2_thread_func(void* arg) {
    char buf[64];
    uint32_t x;
    uint32_t y;

    while (true) {
        Serial.println("Waiting for sample to be ready");
        while (!sampleReady) {
            threads.yield();
        }
        sampleReady = false;
  
        // Don't read from shared variable until the lock in the producer
        // has been released.
        while (mutex.try_lock() == 0) {
            threads.yield();
        }
        x = sample.x;
        y = sample.y;
        mutex.unlock();
        snprintf(buf, sizeof buf, "Sample is ready, x = %ld, y = %ld", x, y);
        Serial.println(buf);
    }
}
