#include "SerializationSupport.hpp"
#include <iostream>

using namespace mutils;

int main(){
	std::vector<bool> b;
	std::cout << mutils::bytes_size(0) << std::endl;
	{
		char buf[256];
		to_bytes(b,buf);
		assert(b == *from_bytes<std::vector<bool>>(nullptr,buf));
		assert(mutils::bytes_size(b) == mutils::bytes_size(std::size_t{0}));
		std::cout << mutils::bytes_size(std::size_t{0}) << std::endl;
	}
	auto i = 0u;
	while (better_rand() < .8 || i < 100) {
		b.push_back(better_rand() > .5 ? true : false);
		++i;
	}
	assert(b.size() > 5);
	std::cout << b.size() << std::endl;
	std::size_t canary;
	canary = 471341890183081ul;
	char data[b.size()];
	auto canary2 = canary;
	assert(data);
	bzero(data,b.size());
	assert(canary == 471341890183081ul);
	assert(canary2 == canary);
	const auto size = bytes_size(b);
	const auto size2 = to_bytes(b,data);
	assert(size == size2);
	std::cout << size << std::endl;
	assert(canary == 471341890183081ul);
	assert(canary2 == canary);
	std::cout << data << std::endl;
	std::cout << (std::size_t) data << std::endl;
	std::cout << "i am concerned" << std::endl;
	assert(data);
	assert(canary == 471341890183081ul);
	canary = 348920;
	auto newb = from_bytes<std::vector<bool> >(nullptr,data);
	assert(*newb == b);
}
