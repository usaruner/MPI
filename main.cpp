#include <iostream>
#include <string>
#include <mutex>
#include <thread>
#include <atomic>
#include <chrono>

struct phil{
private:
	std::atomic<int>& left;
	std::atomic<int>& right;
public:
	phil(atomic<bool> l,atomic<bool> r, int name) {
		left = l;
		right = r;
	}

	void eat() {
		std::cout << name << "started eating" << std::endl;
		std::this_thread::sleep_for(std::chrono::seconds(1000));
		std::cout << name << "stopped eating" << std::endl;
	}

	void grabfork() {
		left = name;
		right = name;
		eatmeals();
	}

	void eatmeals() {
		int mealeaten = 0;
		while (mealeaten < 3) {
			if (grabfork()) {
				eat();
				mealeaten++;
			}

		}
		left = -1;
		right = -1;
	}
};

int main(int argc, char** argv) {
	int num = 0;
	if (argc == 2) {
		num = argv[1];
	}
	else {
		return -1;
	}
	phil phils[num];
	atomic<int> forks[num];
	for (int i = 0; i < num + 1; i++) {
		phils[i] = {forks[i],forks[(i+1)%num]}
	}
}

