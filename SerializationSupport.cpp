#include "SerializationSupport.hpp"
#include <string.h>
using namespace std;

namespace mutils {

	std::size_t to_bytes(const ByteRepresentable& b, char* v){
		return b.to_bytes(v);
	}

	std::size_t bytes_size(const ByteRepresentable& b){
		return b.bytes_size();
	}

	std::size_t to_bytes(const std::string& b, char* v){
		strcpy(v,b.c_str());
		return b.length() + 1;
	}

	std::size_t bytes_size(const std::string& b){
		return b.length() + 1;
	}
	
	void ensure_registered(ByteRepresentable& b, DeserializationManager& dm){
		b.ensure_registered(dm);
	}

}
