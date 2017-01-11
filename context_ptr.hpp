#pragma once
#include <memory>
#include <type_traits>

namespace mutils{

	struct ByteRepresentable;

	template<typename>
	struct ContextDeleter;

	template<>
	struct ContextDeleter<void> {
		void operator()(...){}
	};
	
	template<typename T>
	struct ContextDeleter<std::default_delete<T> > : public std::default_delete<T> {};

	template<typename>
	struct get_context_obj_str;
	
	namespace internal_ctx_ptr{

		template<typename T>
		struct passthrough {
			using type = std::default_delete<T>;
		};
		
		template<typename T>
		using context_obj_choice1 =
			std::conditional_t<std::is_base_of<ByteRepresentable, T>::value,
							   get_context_obj_str<ByteRepresentable>,
							   std::conditional_t<std::is_pod<T>::value,
												  get_context_obj_str<void>,
												  passthrough<T>
												  >
							   
							   >;
	}
	
	template<>
	struct get_context_obj_str<ByteRepresentable> {
		using type = ByteRepresentable;
	};
	
	template<typename T>
	struct get_context_obj_str : public internal_ctx_ptr::context_obj_choice1<T> {
		using typename internal_ctx_ptr::context_obj_choice1<T>::type;
	};

	template<>
	struct get_context_obj_str<void> {
		using type = void;
	};
	
	template<typename T>
	using get_context_obj = typename get_context_obj_str<T>::type;
	
	template<typename T>
	using context_ptr = std::unique_ptr<T,ContextDeleter<get_context_obj<T> > >;

}
