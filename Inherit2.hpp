#pragma once
#include "../mutils/17_type_utils.hpp"

namespace mutils{

	struct InheritByteRepresentable{
		InheritContext &ih;
		virtual std::size_t serial_uuid();
		virtual std::size_t subclass_to_bytes(char *);
		std::size_t inherit_to_bytes(char* v){
			(std::size_t*) v[0] = serial_uuid();
			v += sizeof(std::size_t);
			return sizeof(std::size_t) + subclass_to_bytes(v):
		}
	};

}
