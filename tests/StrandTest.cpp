#include <random>
#include <stdlib.h>
#include <string>
#include <atomic>
#include <thread>
#include <gtest/gtest.h>

#include "Strand.h"
#include "WorkQueue.h"

// http://stackoverflow.com/questions/7560114/random-number-c-in-some-range
int randInRange(int min, int max) {
    std::random_device rd;   // obtain a random number from hardware
    std::mt19937 eng(rd());  // seed the generator
    std::uniform_int_distribution<> distr(min, max);  // define the range
    return distr(eng);
}

struct Obj {
    explicit Obj(int n, WorkQueue& wp) : strand(wp) {
        name = "Obj " + std::to_string(n);
    }

    void doSomething(int val) {
        printf("%s : doing %d\n", name.c_str(), val);
    }
    
    std::string name;
    Strand<WorkQueue> strand;
};

// Demonstrate some basic usage.
TEST(StrandTest, BasicAssertions) {
    WorkQueue workQueue;
    // Start a couple of worker threads
    std::vector<std::thread> workerThreads;
    for (int i = 0; i < 4; i++) {
        workerThreads.push_back(std::thread([&workQueue] { workQueue.run(); }));
    }

    // Create a couple of objects that need strands
    std::vector<std::unique_ptr<Obj>> objs;
    for (int i = 0; i < 8; i++) {
        objs.push_back(std::make_unique<Obj>(i, workQueue));
    }

    // Counter used by all strands, so we can check if all work was done
    std::atomic<int> doneCount(0);

    // Add work to random objects
    const int todo = 20;
    for (int i = 0; i < todo; i++) {
        auto&& obj = objs[randInRange(0, objs.size() - 1)];
        obj->strand.post([&obj, i, &doneCount] {
            obj->doSomething(i);
            ++doneCount;
        });
    }

    workQueue.stop();
    for (auto&& t : workerThreads) {
        t.join();
    }

    EXPECT_EQ(doneCount, todo);
}