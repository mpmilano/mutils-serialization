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

	template<>
	struct ContextDeleter<ByteRepresentable> {
		void operator()(ByteRepresentable* br){
			delete br;
		}
	};
	
	template<typename T>
	struct ContextDeleter<std::default_delete<T> > : public std::default_delete<T> {};

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

	template<typename>
	struct get_context_obj_str;
	
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
	
	//sample of how this might work.  Nocopy, plus complete memory safety, but
	//at the cost of callback land.
	template<typename T, typename F>
	auto deserialize_and_run(DeserializationManager* dsm, char * v, const F& fun){
		using fun_t = std::function<std::result_of_t<F(T&)> (T&)>;
		//ensure implicit conversion can run
		static_assert(std::is_convertible<F, fun_t>::value,
					  "Error: type mismatch on function and target deserialialized type");
		return fun(*from_bytes_noalloc<T>(dsm,v));
	}

}
