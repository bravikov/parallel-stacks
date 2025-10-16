#include <iostream>
#include <print>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>

std::mutex m;
std::condition_variable terminate;

void worker(int i)
{
    std::unique_lock<std::mutex> lock(m);
    terminate.wait(lock);
    std::println("exit thread {}", i);
}

int main() {
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; i++) {
        threads.emplace_back(worker, i);
        // TODO: add a name for a thread
    }

    std::string input;
    std::println("allowed commands:");
    std::println("  exit – interrupts program.");
    while(true) {
        std::print("> ");
        std::cin >> input;
        if (input == "exit") {
            terminate.notify_all();
            break;
        }
        else {
            std::println(std::cerr, "bad command: {}", input);
        }
    }

    for (auto& thread: threads) {
        thread.join();
    }
}
