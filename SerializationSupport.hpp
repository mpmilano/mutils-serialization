#pragma once
#include "mutils.hpp"
#include "type_utils.hpp"
#include "SerializationMacros.hpp"
#include "macro_utils.hpp"
#include <vector>
#include <cstring>

//BEGIN DECLARATIONS AND USAGE INFORMATION

namespace mutils{

	//forward declaration
	struct DeserializationManager;

	/** 
	 * A non-POD type which wishes to mark itself byte representable should extend this class.
	 * Intended to use to convert classes to and from contiguous sequences of bytes.
	 */
	struct ByteRepresentable {

		/**
		 * Write this class's marshalled representation into the array found at v.
		 * assume v has at least bytes_size() of free memory available; behavior
		 * is undefined otherwise.
		 * 
		 * Returns number of bytes written, which should be the same as bytes_size().
		 *
		 * NOTE: it is recommended that users not call this directly, and prefer
		 * to use mutils::to_bytes(T,v) instead.
		 */
		virtual int to_bytes(char* v) const = 0;

		/**
		 * the size of the marshalled representation of this object. 
		 * useful when allocating arrays in which to store this object.
		 *
		 * NOTE: it is recommended that users not call this directly, and prefer
		 * to use mutils::bytes_size(T,v) instead.
		 */
		virtual int bytes_size() const = 0;

		/**
		 * If this object requires context in order to correctly 
		 * deserialize, this method will associate that context
		 * with the provided DeserializationManager.  Many objects
		 * will not require context, and so can leave this function 
		 * empty.
		 */
		virtual void ensure_registered(DeserializationManager&) = 0;
		virtual ~ByteRepresentable(){}

		/**
		 * from_bytes takes the DeserializationManager which manages this object's context
		 * (or nullptr, if this object does not require a context), a byte array of size
		 * at least bytes_size(), and returns a new heap-allocated instance of that object.
		 *
		 * NOTE: it is recommended that users not call this directly, and prefer
		 * to use mutils::from_bytes<T>(DeserializationManager*,v) instead.
		 */
		//needs to exist, but can't declare virtual statics
		//virtual static std::unique_ptr<T> from_bytes(DeserializationManager *p, char *v) const  = 0;
	};

	/**
	 * If a class which implements ByteRepresentable requires a context in order
	 * to correctly deserialize, that context should be represented as a class that extends
	 * RemoteDeserializationContext.  If no context is required, then this class is
	 * not necessary. 
	 */
	struct RemoteDeserializationContext{
		DeserializationManager* this_mgr{nullptr};
		RemoteDeserializationContext(const RemoteDeserializationContext&) = delete;
		virtual ~RemoteDeserializationContext(){}
		RemoteDeserializationContext(){}
	};

	//a pointer to a RemoteDeserializationContext. This exists
	//so that I can switch it out with smart pointer types when
	//debugging stuff.
	using RemoteDeserializationContext_p =
                RemoteDeserializationContext*;

	//a vector of RemoteDeserializationContext*.
	//it's got a using declaration for the same reason
	//as the above.
	using RemoteDeserialization_v =
		std::vector<RemoteDeserializationContext_p>;

	/**
	 * The manager for any RemoteDeserializationContexts.
	 * Don't subclass this; rather construct it with any context managers
	 * you need as arguments to it.
	 * /be sure to have a pointer to this on hand whenever you need to deserialize
	 * something. If you're dead certain you never need a deserialization
	 * context, then you can not use this at all and just pass null
	 * to from_bytes in place of this.
	 */
	struct DeserializationManager {

		/**
		 * Various registered managers. Please note that this class
		 * does *not* own these pointers; you need to keep them 
		 * managed somewhere else. Also ensure lifetime of this
		 * class is shorter than or the same as those registered
		 * contexts.
		 */
		const RemoteDeserialization_v registered_v;
		DeserializationManager(RemoteDeserialization_v rv):registered_v(rv){
			for (auto &r : registered_v) r->this_mgr = this;
		}
		DeserializationManager(const DeserializationManager&) = delete;

		/**
		 * Lookup the context registered at this DeserializationManager 
		 * whose type is T.  Note this means we assume that types uniquely 
		 * identify contexts. 
		 */
		template<typename T>
		T& mgr() {
			for (auto& candidate : registered_v){
				if (dynamic_cast<T*>(candidate))
					return dynamic_cast<T&>(*candidate);
				
			}
			assert(false && "Error: no registered manager exists");
		}

		/**
		 * As the above, but const. 
		 */
		template<typename T>
		const T& mgr() const {
			for (auto& candidate : registered_v){
				if (dynamic_cast<T*>(candidate))
					return dynamic_cast<T&>(*candidate);
			}
			assert(false && "Error: no registered manager exists");
		}

		/**
		 * checks to see if a context of type T has been 
		 * registered with this DeserializationManager.
		 */
		template<typename T>
		bool registered() const {
			for (auto& candidate : registered_v){
                                if (dynamic_cast<T const *>(candidate))
					return true;
			}
			return false;
		}
	};


	/**
	 * Calls b.ensure_registered(dm) when b is a ByteRepresentable;
	 * returns true when b is POD.
	 */
	void ensure_registered(ByteRepresentable& b, DeserializationManager& dm);

	/**
	 * Calls b.ensure_registered(dm) when b is a ByteRepresentable;
	 * returns true when b is POD.
	 */
	template<typename T, restrict(std::is_pod<T>::value)>
	void ensure_registered(const T&, DeserializationManager&){}

	/**
	 * calls b.to_bytes(v) when b is a ByteRepresentable;
	 * calls std::memcpy() when b is POD.  Custom logic 
	 * is implemented for some STL types.  When ubuntu
	 * gets GCC5.0 or better, this will also work if
	 * b is trivially copyable.
	 */
	int to_bytes(const ByteRepresentable& b, char* v);

	/**
	 * calls b.bytes_size() when b is a ByteRepresentable;
	 * calls sizeof(decay_t<decltype(b)>) when b is a POD;
	 * custom logic is implemented for some STL types.
	 */
	int bytes_size(const ByteRepresentable& b);

	/**
	 * extracts the C string (char*) equivalent to this 
	 * std::string and stores it in v
	 */
	int to_bytes(const std::string& b, char* v);

	/**
	 * effectively strlen().
	 */
	int bytes_size(const std::string& b);

	/**
	 * Calls T::from_bytes(ctx,v) when T is a ByteRepresentable. 
	 * uses std::memcpy() when T is a POD.  
	 * custom logic is implemented for some STL types.
	 */
	template<typename T>
	std::enable_if_t<std::is_base_of<ByteRepresentable CMA T>::value,
					 std::unique_ptr<T> > from_bytes(DeserializationManager* ctx, char const *v){
		return T::from_bytes(ctx,v);
	}

	/**
	 * Calls T::from_bytes(ctx,v) when T is a ByteRepresentable. 
	 * uses std::memcpy() when T is a POD.  
	 * custom logic is implemented for some STL types.
	 */
	template<typename T>
	std::enable_if_t<std::is_pod<T>::value
					 ,std::unique_ptr<std::decay_t<T> > > from_bytes(DeserializationManager*, char const *v);

	/**
	 * Serialization is also implemented for the following STL types:
	 * vector
	 * pair
	 * string
	 * set
	 */

	//end forward-declaring; everything past this point is implementation,
	//and not essential to understanding  the interface.

	template<typename T>
	int to_bytes(const std::vector<T> &vec, char* _v){
		((int*)_v)[0] = vec.size();
		char* v = _v + sizeof(int);
		if (std::is_pod<T>::value){
			int size = vec.size() * bytes_size(vec.back());
			memcpy(v, vec.data(),size);
			return size + sizeof(int);
		}
		else{
			int offset = 0;
			for (auto &e : vec){
				offset += (to_bytes(e,v + offset));
			}
			return offset + sizeof(int);
		}
	}

	template<typename T>
	int bytes_size (const std::vector<T> &v){
		if (std::is_pod<T>::value)
			return v.size() * bytes_size(v.back()) + sizeof(int);
		else {
			int accum = 0;
			for (auto &e : v) accum += bytes_size(e);
			return accum + sizeof(int);
		}
	}


	template<typename T, restrict(std::is_pod<T>::value)>
	int to_bytes(const T &t, char* v){
		auto res = std::memcpy(v,&t,sizeof(T));
		assert(res);
		return sizeof(T);
	}

	template<typename T, restrict2(std::is_pod<T>::value)>
	auto bytes_size(const T&){
		return sizeof(T);
	}
	
	template<typename T, typename V>
	int to_bytes(const std::pair<T,V> &pair, char* v){
		auto offset = to_bytes(pair.first,v);
		auto offset2 = to_bytes(pair.second,v + offset);
		return offset + offset2;
	}

	template<typename T, typename V>
	int bytes_size (const std::pair<T,V> &pair){
		return bytes_size(pair.first) + bytes_size(pair.second);
	}

	template<typename T>
	std::enable_if_t<std::is_pod<T>::value
					 ,std::unique_ptr<std::decay_t<T> > > from_bytes(DeserializationManager*, char const *v){
		using T2 = std::decay_t<T>;
		if (v) {
			auto t = std::make_unique<T2>(*(T2*)v);
			//std::memcpy(t.get(),v,sizeof(T));
			return std::move(t);
		}
		else return nullptr;
	}

	template<typename T, typename P>
	std::unique_ptr<T> from_bytes_stupid(P* p, T* t, char const * v) {
		return from_bytes<T>(p,v);
	}

	template<typename T>
	int to_bytes(const std::set<T>& s, char* _v){
		((int*)_v)[0] = s.size();
		char *v = _v + sizeof(int);
		for (auto &a : s){
			v += to_bytes(a,v);
		}
		return _v - v;
	}

	template<typename T>
	int bytes_size(const std::set<T>& s){
		int size = sizeof(int);
		for (auto &a : s) {
			size += bytes_size(a);
		}
		return size;
	}

	template<typename>
	struct is_set : std::false_type {};

	template<typename T>
	struct is_set<std::set<T> > : std::true_type {};

	template<typename>
	struct is_pair : std::false_type {};

	template<typename T, typename U>
	struct is_pair<std::pair<T,U> > : std::true_type {};

	template<typename>
	struct is_string : std::false_type {};

	template<>
	struct is_string<std::string> : std::true_type {};

	template<>
	struct is_string<const std::string> : std::true_type {};
	
	template<typename T>
	std::unique_ptr<type_check<is_string,T> > from_bytes(DeserializationManager*, char const *v){
		assert(v);
		return std::make_unique<T>(v);
	}
	
        template<typename T, typename P>
	std::unique_ptr<type_check<is_set,T> > from_bytes(P* ctx, char* const _v) {
		int size = ((int*)_v)[0];
		char* v = _v + sizeof(int);
		auto r = std::make_unique<std::set<typename T::key_type> >();
		for (int i = 0; i < size; ++i){
			auto e = from_bytes<typename T::key_type>(ctx,v);
			v += bytes_size(*e);
			r->insert(*e);
		}
		return std::move(r);
	}

        template<typename T, typename P>
	std::unique_ptr<type_check<is_pair,T > > from_bytes(P* ctx, char const * v){
		using ft = typename T::first_type;
		using st = typename T::second_type;
		auto fst = from_bytes<ft>(ctx,v);
		return std::make_unique<std::pair<ft,st> >
			(*fst, *from_bytes<st>(ctx,v + bytes_size(*fst)));
	}

        template<typename T, typename P>
	std::enable_if_t<is_vector<T>::value,std::unique_ptr<T> > from_bytes(P* ctx, char const * v){
		using member = typename T::value_type;
		if (std::is_pod<typename T::value_type>::value){
			member const * const start = (member*) (v + sizeof(int));
			const int size = ((int*)v)[0];
			return std::unique_ptr<T>{new T{start, start + size}};
		}
		else{
			int size = ((int*)v)[0];
			auto* v2 = v + sizeof(int);
			int per_item_size = -1;
			std::unique_ptr<std::vector<member> > accum{new T()};
			for(int i = 0; i < size; ++i){
				std::unique_ptr<member> item = from_bytes<typename T::value_type>(ctx,v2 + (i * per_item_size));
				if (per_item_size == -1)
					per_item_size = bytes_size(*item);
				accum->push_back(*item);
			}
			return accum;
		}
	}
}
