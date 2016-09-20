#pragma once
#include "SerializationSupport.hpp"
#include "connection.hpp"

namespace mutils{

	struct Bytes : public ByteRepresentable{
		
		char const * const bytes;
		const std::size_t size;
		
		Bytes(decltype(bytes) b, decltype(size) s)
			:bytes(b),size(s){}
		
		std::size_t to_bytes(char* v) const{
			((std::size_t*)(v))[0] = size;
			memcpy(v + sizeof(size),bytes,size);
			return size + sizeof(size);
		}
		
		std::size_t bytes_size() const {
			return size + sizeof(size);
		}
		
		void post_object(const std::function<void (char const * const,std::size_t)>& f) const{
			f((char*)&size,sizeof(size));
			f(bytes,size);
		}
		
		void ensure_registered(DeserializationManager&){}
		
		static std::unique_ptr<Bytes> from_bytes(DeserializationManager *, char const * const v)  {
			return std::make_unique<Bytes>(v + sizeof(std::size_t),((std::size_t*)(v))[0]);
		}

	};
}
