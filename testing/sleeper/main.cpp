#include<iostream>
#include<chrono>
#include<thread>


int main(int argc, char* argv[]){
	std::cout << "Hello, I'm feeling sleepy" << std::endl;
	std::cout << "Yahwn" << std::endl;
	std::this_thread::sleep_for(std::chrono::milliseconds(5000));
	std::cout << "I'm awake!" << std::endl;
	return 0;
}
