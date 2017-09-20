#pragma once
#include "../mutils/17_type_utils.hpp"
#include "SerializationSupport.hpp"

namespace mutils{
	
	template <typename super, typename sub, std::size_t id>
	struct InheritPair{
		static_assert(std::is_base_of<super,sub>::value,
									"Error: cannot establish an inherit pair. Ensure subtyping relationship exists.");
		using correct_id = std::integral_constant<std::size_t, id>;

		template<typename T>
		static auto inherit_from_bytes(DeserializationManager *dsm, char const * const v, std::size_t _id){
			if constexpr (std::is_same<T,super>::value){
					super* ret = (id == _id ? from_bytes<sub>(dsm,v).release() : (sub*)nullptr);
					return ret;
				}
			else return nullptr;
		}

		static void* inherit_from_bytes(DeserializationManager *dsm, char const * const v, std::size_t _id){
			if (id == _id) return inherit_from_bytes<super>(dsm,v,_id);
			else return nullptr;
		}

		template<typename T> using matches_sub = std::conditional_t<std::is_same<T,sub>::value, InheritPair, mismatch>;
		template<typename T> using matches_super = std::conditional_t<std::is_same<T,super>::value, InheritPair, mismatch>;
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

	struct InheritContext : public RemoteDeserializationContext{
	protected:
		virtual void* inherit_from_bytes(DeserializationManager* , char const * const) const = 0;
	public:
		template<typename T>
		std::unique_ptr<T> inherit_from_bytes(DeserializationManager* dsm, char const * const _v) const {
			return std::unique_ptr<T>{(T*)inherit_from_bytes(dsm,_v)};
		}
		virtual ~InheritContext() = default;
	};

	template<typename... pairs>
	struct InheritGroup : public InheritContext{

		template<typename super, typename sub, std::size_t it>
		using add_relationship = InheritGroup<pairs...,InheritPair<super,sub,it> >;

		template<typename T>
		static auto add_class(const T& ){
			return InheritGroup<pairs..., typename T::InheritPair>{};
		}
	
		template<typename T> using add_class_t = DECT(add_class(std::declval<T>()));	

		template<typename T>
		static std::unique_ptr<T> inherit_from_bytes(DeserializationManager* dsm, char const * const _v){
			std::size_t id = ((std::size_t*)_v)[0];
			auto *v = _v + sizeof(std::size_t);
			return std::unique_ptr<T>{pick_non_null(pairs::template inherit_from_bytes<T>(dsm,v,id)...)};
		}

		void* inherit_from_bytes(DeserializationManager* dsm, char const * const _v) const {
			std::size_t id = ((std::size_t*)_v)[0];
			auto *v = _v + sizeof(std::size_t);
			return pick_non_null(pairs::inherit_from_bytes(dsm,v,id)...);
		}

		InheritGroup(const InheritGroup&){}
		InheritGroup() = default;
		
	};

	template<typename T>
	std::unique_ptr<T> inherit_from_bytes(DeserializationManager* dsm, char const * const v){
		return dsm->template mgr<InheritContext>().template inherit_from_bytes<T>(dsm,v);
	}

	struct InheritByteRepresentable {
	protected:
		virtual std::size_t serial_uuid() const = 0;
		virtual std::size_t to_bytes(char *) const = 0;
		virtual std::size_t bytes_size() const = 0;
	public:
		std::size_t inherit_to_bytes(char* v) const {
			((std::size_t*) v)[0] = serial_uuid();
			v += sizeof(std::size_t);
			return sizeof(std::size_t) + to_bytes(v);
		}
		std::size_t inherit_bytes_size() const {
			return sizeof(std::size_t) + bytes_size();
		}
		virtual ~InheritByteRepresentable() = default;
	};

#define INHERIT_SERIALIZATION_SUPPORT(name,base,id,fields...) \
	using inherit_id = std::integral_constant<std::size_t,id>;	\
	using InheritPair = ::mutils::InheritPair<base,name,inherit_id::value>;	\
	std::size_t serial_uuid() const { return inherit_id::value;}					\
	DEFAULT_SERIALIZATION_SUPPORT(name,fields);

}
