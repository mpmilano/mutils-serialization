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

	//vector of bool, my nemisis.
	std::size_t bytes_size (const std::vector<bool> &v);
	void post_object(const std::function<void (char const * const, std::size_t)>& f, const std::vector<bool>& vec);
	
	std::size_t to_bytes(const std::vector<bool> &vec, char* v){
		const std::size_t real_size = vec.size();
		std::vector<unsigned char> converted;
		converted.resize((real_size / sizeof(char)) + (real_size % sizeof(char) == 0 ? 0 : 1));
		auto k = 0u;
		for (int j = 0; j < real_size; (++j, ++k)){
			unsigned char &output = converted[k];
			for(int i=0;i<8;(i++, ++j))
			{
				output=output|(vec[j]<<i);
			}
		}
		assert(bytes_size(converted) + bytes_size(real_size) < (real_size*2));
		auto first_sent = to_bytes(real_size, v);
		auto ret = first_sent + to_bytes(converted, v + first_sent);
		return ret;
	}

	std::unique_ptr<std::vector<bool> > boolvec_from_bytes(DeserializationManager* dsm, char const * v){
		const std::size_t real_size = *from_bytes_noalloc<std::size_t>(dsm,v);
		v += bytes_size(real_size);
		auto _converted = from_bytes_noalloc<std::vector<unsigned char> >(dsm,v);
		auto& converted = *_converted;
		v += bytes_size(converted);
		auto _ret = std::make_unique<std::vector<bool>>(real_size);
		std::vector<bool> &ret = *_ret;
		auto k = 0u;
		for (int j = 0; j < real_size; (++j, ++k)){
			for(int i=0;i<8;(i++, ++j))
			{
				ret[j] = converted[k] & (1 << i);
			}
		}
		return _ret;
	}
#ifndef NDEBUG
	void ensure_registered(const std::vector<bool>& v, DeserializationManager& dm);
#endif

}
