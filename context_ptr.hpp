#pragma once
#include <memory>
#include <type_traits>

namespace mutils{

	template<typename T>
	struct ContextDeleter : public std::conditional_t<(std::is_standard_layout<T>::value && std::is_trivial<T>::value),
													  ContextDeleter<void>,
													  std::default_delete<T> > {
	};

	template<>
	struct ContextDeleter<void> {
		void operator()(...){}
	};			   

	
	template<typename T>
	using context_ptr = std::unique_ptr<T,ContextDeleter<T> >;

}
