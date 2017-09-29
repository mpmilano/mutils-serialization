#pragma once
#include "../mutils/17_type_utils.hpp"
#include "SerializationSupport.hpp"

namespace mutils{

	struct InheritMissException{
		const std::size_t id;
		char const * const buffer_after_id;
	};
	
	template <typename super, typename sub, std::size_t id>
	struct InheritPair{
		static_assert(std::is_base_of<super,sub>::value,
									"Error: cannot establish an inherit pair. Ensure subtyping relationship exists.");
		using correct_id = std::integral_constant<std::size_t, id>;

		template<typename T, typename... ctxs>
		static auto inherit_from_bytes(DeserializationManager<ctxs...> *dsm, char const * const v, std::size_t _id){
			if constexpr (std::is_same<T,super>::value){
					super* ret = (id == _id ? sub::from_bytes(dsm,v).release() : (sub*)nullptr);
					return ret;
				}
			else {
				(void) dsm;
				(void) v;
				(void) _id;
				assert(id != _id);
				return nullptr;
			}
		}

		template<typename F> static auto run_on_match(const F& f, super& sup, std::size_t _id){
			if (_id == id)
				return f(dynamic_cast<sub&>(sup));
			else throw InheritMissException{};
		}

		template<typename sup_cand> static bool matches(sup_cand& , std::size_t _id){
			assert((id != _id || std::is_same<sup_cand,super>::value));
			return std::is_same<sup_cand,super>::value && _id == id;
		}

		template<typename sup_cand> static constexpr bool possible_match(){
			return std::is_same<sup_cand,super>::value;
		}
	};

	template <template<typename> class super, template<typename> class sub, std::size_t id>
	struct InheritPairAbs1{
		template<typename A>
		using concretized_t = InheritPair<super<A>,sub<A>,id>;

		template<typename T>
		constexpr static auto concretized(super<T>*){
			return concretized_t<T>{};
		}

		template<typename A>
		constexpr static bool super_matches(super<A>*){
			return true;
		}
		
		template<typename T>
		constexpr static bool super_matches(T*){
			return false;
		}

		template<typename A>
		constexpr static bool sub_matches(sub<A>*){
			return true;
		}
		
		template<typename T>
		constexpr static bool sub_matches(T*){
			return false;
		}

		template<typename T, typename... ctxs>
		static auto inherit_from_bytes(DeserializationManager<ctxs...> *dsm, char const * const v, std::size_t _id){
			constexpr T* choice{nullptr};
			if constexpr(super_matches(choice)){
					return concretized(choice).template inherit_from_bytes<T>(dsm,v,_id);
				}
			else {
				(void) dsm;
				(void) v;
				(void) _id;
				assert(id != _id);
				return nullptr;
			}
		}
		
		template<typename F, typename sup_cand> static auto run_on_match(const F& f, sup_cand& sup, std::size_t _id){
			constexpr sup_cand* choice{nullptr};
			if constexpr(super_matches(choice)){
					return concretized(choice).run_on_match(f,sup,_id);
				}
			else {
				(void) f;
				(void) sup;
				(void) _id;
				assert(id != _id);
				throw InheritMissException{};
			}
		}

		template<typename sup_cand> static bool matches(sup_cand& sup, std::size_t _id){
			constexpr sup_cand* choice{nullptr};
			if constexpr(super_matches(choice)){
					return concretized(choice).matches(sup,_id);
				}
			else {
				(void) sup;
				(void) _id;
				assert(id != _id);
				return false;
			}
		}

		template<typename sup_cand> static constexpr bool possible_match(){
			constexpr sup_cand* choice{nullptr};
			if constexpr(super_matches(choice)){
					return concretized(choice).template possible_match<sup_cand>();
				}
			else {
				return false;
			}
		}
		
	};
	
	template<typename... T>
	constexpr auto pick_non_null(std::nullptr_t, T... t);

	template<typename T1, typename... T>
	auto pick_non_null(T1* t1, T... t){
		if constexpr ((std::is_null_pointer<T>::value && ... && true)){
				return t1;
			}
		else return (t1 ? t1 : pick_non_null(t...));
	}

	template<typename... T>
	constexpr auto pick_non_null(std::nullptr_t, T... t){
		if constexpr (sizeof...(T) > 0) return pick_non_null(t...);
		else return nullptr;
	}

	struct InheritManager{};

	template<typename... pairs>
	struct InheritGroup : public RemoteDeserializationContext, public InheritManager{

		template<typename super, typename sub, std::size_t it>
		using add_relationship = InheritGroup<pairs...,InheritPair<super,sub,it> >;

		template<typename T>
		static auto add_class(const T& ){
			return InheritGroup<pairs..., typename T::InheritPair>{};
		}
	
		template<typename T> using add_class_t = DECT(add_class(std::declval<T>()));

		template<typename T, typename... ctxs>
		static std::unique_ptr<T> inherit_from_bytes(DeserializationManager<ctxs...>* dsm, char const * const _v){
			std::size_t id = ((std::size_t*)_v)[0];
			auto *v = _v + sizeof(std::size_t);
			auto ret = std::unique_ptr<T>{pick_non_null(pairs::template inherit_from_bytes<T>(dsm,v,id)...)};
			if (ret) return ret;
			else throw InheritMissException{id, v};
		}

	private:
		template<typename RET, typename F, typename super>
		static RET run_on_match_fold(const F&, super&, std::size_t){
			throw InheritMissException{};
		}
		
		template<typename RET, typename F, typename super, typename fstpair, typename... _pairs>
		static RET run_on_match_fold(const F& f, super& sup, std::size_t id){
			if constexpr (fstpair::template possible_match<super>()){
					if (fstpair::matches(sup,id)){
						return fstpair::run_on_match(f,sup,id);
					}
				}
			return InheritGroup::run_on_match_fold<RET,F,super,_pairs...>(f,sup,id);
		}

		template<typename F, typename super, typename fstpair, typename... _pairs>
		static auto run_on_match_fold(const F& f, super& sup, std::size_t id){
			if constexpr (fstpair::template possible_match<super>()){
					using RET = decltype(fstpair::run_on_match(f,sup,id));
					return run_on_match_fold<RET,F,super,fstpair,_pairs...>(f,sup,id);
				}
			else return InheritGroup::run_on_match_fold<F,super,_pairs...>(f,sup,id);
		}
		
	public:

		template<typename super>
		static constexpr auto contains_possible_match(){
			return (pairs::template possible_match<super>() || ... || false);
		}
		
		template<typename F, typename super>
		static auto run_on_match(const F& f, super& sup, std::size_t id){
			return run_on_match_fold<F,super,pairs...>(f,sup,id);
		}

		InheritGroup(const InheritGroup&){}
		InheritGroup() = default;
		
	};

	template<typename> struct is_inherit_group;
	template<typename... T> struct is_inherit_group<InheritGroup<T...>> : public std::true_type{};
	template<typename> struct is_inherit_group : public std::false_type{};

	template<typename DSM, typename ctx1, typename... ctxs> auto& find_inherit_group(typelist<ctx1, ctxs...>, DSM* dsm){
		if constexpr (is_inherit_group<ctx1>::value) {
				return dsm->template mgr<ctx1>();
			}
		else return find_inherit_group(typelist<ctxs...>{}, dsm);
	}

	template<typename T, typename... ctxs>
	std::unique_ptr<T> inherit_from_bytes(DeserializationManager<ctxs...>* dsm, char const * const v){
		return find_inherit_group(typelist<ctxs...>{}, dsm).template inherit_from_bytes<T>(dsm,v);
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
