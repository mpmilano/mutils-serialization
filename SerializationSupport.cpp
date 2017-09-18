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

#ifndef NDEBUG
	void ensure_registered(ByteRepresentable& b, DeserializationManager& dm){
		b.ensure_registered(dm);
	}
#endif

	context_ptr<marshalled>
	marshalled::from_bytes_noalloc(DeserializationManager const * const, char* v) {
		return context_ptr<marshalled>((marshalled*) v);
	}

	std::function<void (char const * const, std::size_t)> post_to_buffer(std::size_t &index, char * dest_buf){
		return [&index,dest_buf](char const * const read_buf, std::size_t size){
			memcpy(dest_buf + index, read_buf, size);
			index += size;
		};
	}

	void post_object(const std::function<void (char const * const, std::size_t)>& f, const ByteRepresentable &br){
		br.post_object(f);
	}

	void post_object(const std::function<void (char const * const, std::size_t)>& f, const std::string& str) {
		f(str.c_str(), str.length() + 1);
	}

	std::size_t to_bytes_v(char *){
		return 0;
	}
	
	std::size_t from_bytes_v(DeserializationManager *, char const * const ){
		return 0;
	}

	std::size_t from_bytes_noalloc_v(DeserializationManager *, char const * const ){
		return 0;
	}

	constexpr auto boolvec_adjust_size_down(std::size_t real_size){
		return ((real_size / 8) + (real_size % 8 == 0 ? 0 : 1));
	}

	//vector of bool, my nemisis.
	std::size_t bytes_size (const std::vector<bool> &vec){
		const std::size_t real_size = vec.size();
		return bytes_size(real_size) + 8*boolvec_adjust_size_down(real_size);
	}

	auto make_compact_vector(const std::vector<bool>& vec){
		std::vector<unsigned char> converted;
		const std::size_t real_size = vec.size();
		converted.resize(boolvec_adjust_size_down(real_size));
		auto k = 0u;
		auto j = 0u;
		for (; j < real_size; ++k){
			unsigned char &output = converted[k];
			output = 0;
			for(int i=0;i<8 && j < real_size;(i++, ++j))
			{
				output=output|(vec[j]<<i);
			}
		}
		assert(k == converted.size());
		assert(j == real_size);
		assert(bytes_size(converted) + bytes_size(real_size) < (real_size*2));
	}
	
	void post_object(const std::function<void (char const * const, std::size_t)>& f, const std::vector<bool>& vec){
		auto real_size = vec.size();
		post_object(f,real_size);
		auto j = 0u;
		while (j < real_size){
			unsigned char output;
			output = 0;
			for(int i=0;i<8 && j < real_size;(i++, ++j))
			{
				output=output|(vec[j]<<i);
			}
			post_object(f,output);
		}
	}

	std::unique_ptr<std::vector<bool> > boolvec_from_bytes(DeserializationManager* dsm, char const * v){
		const std::size_t real_size = *from_bytes_noalloc<std::size_t>(dsm,v);
		v += bytes_size(real_size);
		unsigned char* converted = (unsigned char*) v;
		auto _ret = std::make_unique<std::vector<bool>>(real_size);
		std::vector<bool> &ret = *_ret;
		auto k = 0u;
		auto j = 0u;
		for (; j < real_size; ++k){
			for(int i=0;i<8 && j < real_size;(i++, ++j))
			{
				ret[j] = converted[k] & (1 << i);
			}
		}
		assert(j == real_size);
		return _ret;
	}
#ifndef NDEBUG
	void ensure_registered(const std::vector<bool>& v, DeserializationManager& dm);
#endif

}
