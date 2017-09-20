#pragma once

namespace mutils{
	
	template <typename super, typename sub, std::size_t id>
	struct InheritPair{
		static_assert(std::is_base_of<super,sub>::value,
									"Error: cannot establish an inherit pair. Ensure subtyping relationship exists.");
		using correct_id = std::integral_constant<std::size_t, id>;
		static constexpr correct_id* mkid(){
			constexpr correct_id* ret{nullptr};
			return ret;
		}
		
		static sub* downcast(super* s, correct_id*){
			return dynamic_cast<sub*>(super);
		}
		
		template<std::size_t i>
		static std::nullptr_t downcast(super* s, std::integral_constant<std::size_t, i>, std::enable_if<i != id>* = nullptr){
			return nullptr;
		}
		
		static sub* downcast(super* s, std::size_t _id){
			return (_id == id ? downcast(s, mkid()) : nullptr);
		}
	};

	constexpr auto pick_non_null(){
		return nullptr;
	}
	
	template<typename T>
	constexpr auto pick_non_null(T* t){
		return t;
	}

	template<typename T1, typename... T>
	auto pick_non_null(T1* t1, T*... t){
		return (t1 ? t1 : pick_non_null(t...));
	}

	template<typename... T>
	constexpr auto pick_non_null(std::nullptr_t, T*... t){
		return pick_non_null(t...);
	}

	template<typename... pairs>
	struct InheritGroup : public RemoteDeserializationContext {
		template<typename super, std::size_t i>
		auto* downcast(super* s, std::integral_constant<std::size_t,i>* id){
			return pick_non_null(pairs::downcast(s,id)...);
		}
		template<typename sub, typename super>
		sub* downcast(super* sup, std::size_t id){
			return pick_non_null(pairs::downcast(s,id)...);
		}

		template<typename super, typename sub, std::size_t it>
		using add_relationship = InheritGroup<pairs...,InheritPair<super,sub,it> >;
		
	};

	

}
