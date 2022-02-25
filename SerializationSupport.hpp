#pragma once
#include "mutils/mutils.hpp"
#include "mutils/type_utils.hpp"
#include "SerializationMacros.hpp"
#include "context_ptr.hpp"
#include "mutils/macro_utils.hpp"
#include "mutils/17_type_utils.hpp"
#include "mutils/typelist.hpp"
#include <vector>
#include <cstring>

//BEGIN DECLARATIONS AND USAGE INFORMATION

namespace mutils{

	//forward declaration
	template<typename...>
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
		virtual std::size_t to_bytes(char* v) const = 0;

		/**
		 * Pass a pointer to a buffer containing this class's marshalled representation 
		 * into the function f.  This pointer is not guaranteed to live beyond the duration
		 * of the call to f, so make a copy if you need to keep it around.
		 *
		 * NOTE: it is recommended that users not call this directly, and prefer
		 * to use mutils::post_object(f,T) instead.
		 */
		virtual void post_object(const std::function<void (char const * const,std::size_t)>&) const = 0;

		/**
		 * the size of the marshalled representation of this object. 
		 * useful when allocating arrays in which to store this object.
		 *
		 * NOTE: it is recommended that users not call this directly, and prefer
		 * to use mutils::bytes_size(T,v) instead.
		 */
		virtual std::size_t bytes_size() const = 0;

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
		//virtual static std::unique_ptr<T> from_bytes(DeserializationManager<ctxs...>*p, const char *v) const  = 0;

		/**
		 * from_bytes_noalloc takes the DeserializationManager which manages this object's context
		 * (or nullptr, if this object does not require a context), a byte array of size
		 * at least bytes_size(), and returns an instance of that object.  This instance may share storage
		 * with the provided byte array, and is not valid past the end of life of the byte array.
		 *
		 * NOTE: it is recommended that users not call this directly, and prefer
		 * to use mutils::deserialize_and_run<T>(DeserializationManager*,v, f) instead.  If the cost of passing a
		 * function is too high, please still prefer mutils::from_bytes_noalloc<T>(DeserializationManager*,v).
		 */
		//needs to exist, but can't declare virtual statics
		//virtual static context_ptr<T> from_bytes_noalloc(DeserializationManager<ctxs...>*p, const char *v) const  = 0;
	};


	/**
	 * If a class which implements ByteRepresentable requires a context in order
	 * to correctly deserialize, that context should be represented as a class that extends
	 * RemoteDeserializationContext.  If no context is required, then this class is
	 * not necessary. 
	 */
	struct RemoteDeserializationContext{
		RemoteDeserializationContext(const RemoteDeserializationContext&) = delete;
		virtual ~RemoteDeserializationContext(){}
		RemoteDeserializationContext(){}
	};

	template<typename T>
	struct DeserializationContextHolder{
		static_assert(std::is_base_of<RemoteDeserializationContext, T>::value, "Error: Not a remote deserialization context" );
		T* holder;
		DeserializationContextHolder(T* holder):holder(holder){}
		using stored_type = T;
		template<typename T2>
		using match = std::conditional_t<std::is_base_of<T2,T>::value || std::is_same<T2,T>::value,
																		 DeserializationContextHolder, mismatch>;
	protected:
		~DeserializationContextHolder() = default;
	};

	/**
	 * The manager for any RemoteDeserializationContexts.
	 * Don't subclass this; rather construct it with any context managers
	 * you need as arguments to it.
	 * /be sure to have a pointer to this on hand whenever you need to deserialize
	 * something. If you're dead certain you never need a deserialization
	 * context, then you can not use this at all and just pass null
	 * to from_bytes* in place of this.
	 */
	template<typename... DeserializationContexts> struct DeserializationManager;

	template<> struct DeserializationManager<>{
		template<typename> constexpr static bool contains_mgr() {return false;}
		template<typename> constexpr static auto _mgr() { return mismatch{};}
	};
	template<typename CT1, typename... DeserializationContexts>
	struct DeserializationManager<CT1, DeserializationContexts...> : public DeserializationContextHolder<CT1>, public DeserializationManager<DeserializationContexts...>{

		/**
		 * Various registered managers. Please note that this class
		 * does *not* own these pointers; you need to keep them 
		 * managed somewhere else. Also ensure lifetime of this
		 * class is shorter than or the same as those registered
		 * contexts.
		 */
		using DeserializationContextHolder<CT1>::holder;
		DeserializationManager(CT1* ptr1, DeserializationContexts*... ptrs):
			DeserializationContextHolder<CT1>(ptr1),DeserializationManager<DeserializationContexts...>(ptrs...){}
		
		DeserializationManager(const DeserializationManager&) = delete;
		
		DeserializationManager(DeserializationManager&& o) = delete;

		/**
		 * Lookup the context registered at this DeserializationManager 
		 * whose type is T.  Note this means we assume that types uniquely 
		 * identify contexts. 
		 */
		template<typename T>
		static auto _mgr() {
			return find_match_if_exists<typename DeserializationContextHolder<CT1>::template match<T>,
																	remove_ptr<DECT(DeserializationManager<DeserializationContexts...>::template _mgr<T>())> > ();
		}

		template<typename T>
		auto& mgr() {
			return *DECT(*_mgr<T>())::holder;
		}

		/**
		 * As the above, but const. 
		 */
		template<typename T>
		auto& mgr() const {
			return *DECT(*_mgr<T>())::holder;
		}

		template<typename T>
		constexpr static bool contains_mgr(){
			constexpr auto ret = !(std::is_same<mutils::mismatch,typename DeserializationContextHolder<DeserializationContexts>::template match<T> >::value
														 && ... && std::is_same<mutils::mismatch,typename DeserializationContextHolder<CT1>::template match<T> >::value);
			return ret;
		}
	};

	/**
     * Just calls sizeof(T)
     */
    template<typename T, restrict2((std::is_standard_layout<T>::value && std::is_trivial<T>::value))>
    auto bytes_size(const T&){
        return sizeof(T);
    }

    /**
     * calls b.bytes_size() when b is a ByteRepresentable;
     * calls sizeof(decay_t<decltype(b)>) when b is a POD;
     * custom logic is implemented for some STL types.
     */
    std::size_t bytes_size(const ByteRepresentable& b);

    /**
     * effectively strlen().
     */
    std::size_t bytes_size(const std::string& b);

    /**
     * sums the size of both pair elements
     */
    template<typename T, typename V>
    std::size_t bytes_size (const std::pair<T,V>& pair){
        return bytes_size(pair.first) + bytes_size(pair.second);
    }

    /**
     * all of the elements of this vector, plus one int for the number of elements.
     */
	std::size_t bytes_size (const std::vector<bool> &v);
	
	template<typename T>
	std::size_t bytes_size (const std::vector<T> &v){
		whendebug(static const auto typenonce_size = bytes_size(type_name<std::vector<T> >());)
        if ((std::is_standard_layout<T>::value && std::is_trivial<T>::value))
					return v.size() * bytes_size(v.back()) + sizeof(int) whendebug(+ typenonce_size);
        else {
            int accum = 0;
            for (auto &e : v) accum += bytes_size(e);
            return accum + sizeof(int) whendebug(+ typenonce_size);
        }
    }

    /**
     * Sums the size of all elements of this list, plus one int for the number
     * of elements.
     */
    template<typename T>
    std::size_t bytes_size(const std::list<T>& list) {
        if((std::is_standard_layout<T>::value && std::is_trivial<T>::value))
            return list.size() * bytes_size(list.back()) + sizeof(int);
        else {
            int accum = 0;
            for(const auto& e : list) accum += bytes_size(e);
            return accum + sizeof(int);
        }
    }

    /**
     * All the elements of the set, plus one int for the number of elements.
     */
    template<typename T>
    std::size_t bytes_size(const std::set<T>& s){
        int size = sizeof(int);
        for (auto &a : s) {
            size += bytes_size(a);
        }
        return size;
    }

    /**
     * Sums the size of each key and value in the map, plus one int for the
     * number of entries
     */
    template<typename K, typename V>
    std::size_t bytes_size(const std::map<K,V>& m) {
        int size = sizeof(int);
        for(const auto& p : m) {
            size += bytes_size(p.first);
            size += bytes_size(p.second);
        }
        return size;
    }

    /**
     * In-place serialization is also sometimes possible.
     * This will take a function that expects buffers to be posted,
     * and will post the object (potentially in multiple buffers)
     * via repeated calls to the function
     */
	template<typename F, typename BR, typename... Args>
	std::enable_if_t<std::is_pod<BR>::value>
	post_object(const F& f, const BR& br, Args&&... args){
		f(std::forward<Args>(args)...,(char*)&br,sizeof(BR));
	}

    void post_object(const std::function<void (char const * const, std::size_t)>& f, const ByteRepresentable& br);

#ifndef NDEBUG
	/**
	 * Calls b.ensure_registered(dm) when b is a ByteRepresentable;
	 * returns true when b is POD.
	 */
	template<typename BR, typename... T>
	void ensure_registered(BR& b, DeserializationManager<T...>& dm, std::enable_if_t<std::is_base_of<ByteRepresentable, BR>::value >* = nullptr){
		b.ensure_registered(dm);
	}

	/**
	 * Calls b.ensure_registered(dm) when b is a ByteRepresentable;
	 * returns true when b is POD.
	 */
	template<typename T, typename DSM, restrict((std::is_standard_layout<T>::value && std::is_trivial<T>::value))>
	void ensure_registered(const T&, DSM&){}
#endif

	/**
	 * calls b.to_bytes(v) when b is a ByteRepresentable;
	 * calls std::memcpy() when b is POD.  Custom logic 
	 * is implemented for some STL types.  When ubuntu
	 * gets GCC5.0 or better, this will also work if
	 * b is trivially copyable.
	 */
	std::size_t to_bytes(const ByteRepresentable& b, char* v);

	/**
	 * extracts the C string (char*) equivalent to this 
	 * std::string and stores it in v
	 */
	std::size_t to_bytes(const std::string& b, char* v);
	
	/**
	 * Calls T::from_bytes(ctx,v) when T is a ByteRepresentable. 
	 * uses std::memcpy() when T is a POD.  
	 * custom logic is implemented for some STL types.
	 */
	template<typename T, typename... ctxs>
	std::enable_if_t<std::is_base_of<ByteRepresentable CMA T>::value,
									 std::unique_ptr<T> > from_bytes(DeserializationManager<ctxs...>* ctx, char const *v){
		return T::from_bytes(ctx,v);
	}

	/**
	 * Calls T::from_bytes(ctx,v) when T is a ByteRepresentable. 
	 * uses std::memcpy() when T is a POD.  
	 * custom logic is implemented for some STL types.
	 */
	template<typename T, typename... ctxs>
	std::enable_if_t<(std::is_standard_layout<T>::value && std::is_trivial<T>::value)
									 ,std::unique_ptr<std::decay_t<T> > > from_bytes(DeserializationManager<ctxs...>*, char const *v);
	
	/**
	 * Calls T::from_bytes_noalloc(ctx,v) when T is a ByteRepresentable. 
	 * returns raw pointer when T is a POD
	 * custom logic is implemented for some STL types.
	 */
	template<typename T, typename... ctxs>
	std::enable_if_t<std::is_base_of<ByteRepresentable CMA T>::value,
									 context_ptr<T> > from_bytes_noalloc(DeserializationManager<ctxs...>* ctx, char *v){
		return T::from_bytes_noalloc(ctx,v);
	}
	template<typename T, typename... ctxs>
	std::enable_if_t<std::is_base_of<ByteRepresentable CMA T>::value,
									 context_ptr<T> > from_bytes_noalloc(DeserializationManager<ctxs...>* ctx, char const * const v){
		return T::from_bytes_noalloc(ctx,v);
	}

	/**
	 * Calls T::from_bytes_noalloc(ctx,v) when T is a ByteRepresentable. 
	 * returns raw pointer when T is a POD
	 * custom logic is implemented for some STL types.
	 */

	template<typename T, typename... ctxs>
	std::enable_if_t<(std::is_standard_layout<T>::value && std::is_trivial<T>::value)
									 ,context_ptr<std::decay_t<T> > > from_bytes_noalloc(DeserializationManager<ctxs...>*, char *v);

	template<typename T, typename... ctxs>
	std::enable_if_t<(std::is_standard_layout<T>::value && std::is_trivial<T>::value)
									 ,context_ptr<const std::decay_t<T> > > from_bytes_noalloc(DeserializationManager<ctxs...>*, char const * const v, context_ptr<T> = context_ptr<T>{});

	/**
	 * Calls mutils::from_bytes_noalloc<T>(ctx,v), dereferences the result, and passes
	 * it to fun.  Returns whatever fun returns.  Memory safe, assuming fun doesn't do
	 * something stupid.
	 */
	template<typename T, typename F, typename... ctxs>
	auto deserialize_and_run(DeserializationManager<ctxs...>* dsm, char * v, const F& fun);
	
	/** 
	 * The "marshalled" type is a wrapper for already-serialized types; 
	 */

	struct marshalled : public ByteRepresentable {
		const std::size_t size;
		char const * const data;

		marshalled(decltype(size) size, decltype(data) data)
			:size(size),data(data){}

		std::size_t to_bytes(char* v) const {
			assert(false && "revisit this");
			std::memcpy(v,data,size);
			return size;
		}
		std::size_t bytes_size() const {
			return size;
		}

#ifndef NDEBUG

		template<typename... T>
		void ensure_registered(DeserializationManager<T...>&){}

#endif

		template<typename DSM>
		static std::unique_ptr<marshalled>
		from_bytes(DSM const * const, char const * const){
			static_assert(std::is_same<DSM,void>::value &&
						  !std::is_same<DSM,void>::value,
						  "Do not deserialize into a marshalled. please."
				);
			return nullptr;
		}		

		template<typename... ctxs>
		static context_ptr<marshalled>
		from_bytes_noalloc(DeserializationManager<ctxs...> const * const, char* v){
			return context_ptr<marshalled>((marshalled*) v);
		}
	};


	/**
	 * Serialization is also implemented for the following STL types:
	 * vector
	 * pair
	 * string
	 * set
	 */

	//end forward-declaring; everything past this point is implementation,
	//and not essential to understanding  the interface.

	/**
	 * Constructs a buffer-consuming function that will copy its input to the
	 * provided destination buffer at the specified index. The created function
	 * can be used as an input to post_object to make post_object serialize the
	 * object to a buffer.
	 * @param index The offset within dest_buf at which the function should copy inputs
	 * @param dest_buf The buffer that should receive bytes read by the function
	 * @return A function that consumes a byte buffer and writes it to dest_buf
	 */
	std::function<void (char const * const, std::size_t)> post_to_buffer(std::size_t& index, char * dest_buf);


	//post_object definitions -- must come before to_bytes definitions that use them
	void post_object(const std::function<void (char const * const, std::size_t)>& f, const std::string& str);
	

    template<typename T, typename V>
    void post_object(const std::function<void (char const * const, std::size_t)>& f, const std::pair<T,V>& pair){
        post_object(f,pair.first);
        post_object(f,pair.second);
    }

	void post_object(const std::function<void (char const * const, std::size_t)>& f, const std::vector<bool>& vec);
	
	template<typename T>
    void post_object(const std::function<void (char const * const, std::size_t)>& f, const std::vector<T>& vec){
		static_assert(!std::is_same<T,bool>::value);
		whendebug(post_object(f,type_name<std::vector<T> >());)
        int size = vec.size();
        f((char*)&size,sizeof(size));
        if ((std::is_standard_layout<T>::value && std::is_trivial<T>::value)){
            std::size_t size = vec.size() * bytes_size(vec.back());
            f((char*) vec.data(), size);
        }
        else{
            for (const auto &e : vec){
                post_object(f,e);
            }
        }
    }

	template<typename T>
    void post_object(const std::function<void (char const * const, std::size_t)>& f, const std::list<T>& list){
	    int size = list.size();
	    f((char*)&size,sizeof(size));
	    for(const auto& e : list) {
	        post_object(f, e);
	    }
	}

    template<typename T>
    void post_object(const std::function<void (char const * const, std::size_t)>& f, const std::set<T>& s){
        int size = s.size();
        f((char*)&size,sizeof(size));
        for (const auto &a : s){
            post_object(f,a);
        }
    }

    template<typename K, typename V>
    void post_object(const std::function<void (char const * const, std::size_t)>& f, const std::map<K,V>& map) {
        int size = map.size();
        f((char*)&size, sizeof(size));
        for(const auto& pair : map) {
            post_object(f, pair.first);
            post_object(f, pair.second);
        }
    }

    //end post_object section

    //to_bytes definitions -- these must come after bytes_size and post_object definitions
    //To reduce code duplication, these are all implemented in terms of post_object

    /**
     * Special to_bytes for POD types, which just uses memcpy
     */
    template<typename T, restrict((std::is_standard_layout<T>::value && std::is_trivial<T>::value))>
    std::size_t to_bytes(const T &t, char* v){
        auto res = std::memcpy(v,&t,sizeof(T));
        assert(res);
		(void)res;
        return sizeof(T);
    }
	
	template<typename T>
	std::size_t to_bytes(const std::vector<T> &vec, char* v){
		auto size = bytes_size(vec);
		std::size_t index = 0;
		post_object(post_to_buffer(index,v), vec);
		return size;

	}

	template<typename T>
	std::size_t to_bytes(const std::list<T>& list, char* buffer) {
	    auto size = bytes_size(list);
	    std::size_t offset = 0;
	    post_object(post_to_buffer(offset, buffer), list);
	    return size;
	}

	template<typename T, typename V>
	std::size_t to_bytes(const std::pair<T,V>& pair, char* buffer){
		std::size_t index = 0;
		post_object(post_to_buffer(index, buffer), pair);
		return bytes_size(pair);
	}

	template<typename T>
    std::size_t to_bytes(const std::set<T>& s, char* _v){
        std::size_t index = 0;
        auto size = bytes_size(s);
        post_object(post_to_buffer(index,_v),s);
        return size;

    }

	template<typename K, typename V>
	std::size_t to_bytes(const std::map<K,V>& m, char* buffer) {
	    std::size_t index = 0;
	    std::size_t size = bytes_size(m);
	    post_object(post_to_buffer(index, buffer), m);
	    return size;
	}
	//end to_bytes section

#ifndef NDEBUG
	//ensure_registered definitions -- these could go anywhere since they don't depend on any other functions
	template<typename... ctxs>
	void ensure_registered(const std::vector<bool>& v, DeserializationManager<ctxs...>& dm);
	template<typename T, typename... ctxs>
	void ensure_registered(const std::vector<T>& v, DeserializationManager<ctxs...>& dm){
		for (auto &e : v) ensure_registered(e,dm);
	}

	template<typename L, typename R, typename... ctxs>
	void ensure_registered(const std::pair<L,R>& v, DeserializationManager<ctxs...>& dm){
		ensure_registered(v.first,dm);
		ensure_registered(v.second,dm);
	}

	template<typename T, typename... ctxs>
    void ensure_registered(const std::set<T>& v, DeserializationManager<ctxs...>& dm){
        for (auto &e : v) ensure_registered(e,dm);
    }

	template<typename T, typename... ctxs>
    void ensure_registered(const std::list<T>& v, DeserializationManager<ctxs...>& dm){
        for (auto &e : v) ensure_registered(e,dm);
    }
	template<typename... ctxs>
	inline void ensure_registered(const std::string&, DeserializationManager<ctxs...>& ){}
    //end ensure_registered section
#endif

	//from_string definition

	template<typename T, typename... ctxs>
	std::unique_ptr<type_check<std::is_integral,T> > from_string(DeserializationManager<ctxs...>*, char const *v, std::size_t length){
		return std::make_unique<T>(std::stoll(std::string{v,length}));
	}

	template<typename T, typename... ctxs>
	std::unique_ptr<type_check<std::is_floating_point,T> > from_string(DeserializationManager<ctxs...>*, char const *v, std::size_t length){
		return std::make_unique<T>(std::stold(std::string{v,length}));
	}

	template<typename>
	struct is_string : std::false_type {};

	template<>
	struct is_string<std::string> : std::true_type {};

	template<>
	struct is_string<const std::string> : std::true_type {};

	template<typename T, typename... ctxs>
	std::unique_ptr<type_check<is_string,T> > from_string(DeserializationManager<ctxs...>*, char const *v, std::size_t length){
		return std::make_unique<T>(std::string{v,length});
	}
	
	//from_bytes definitions
	template<typename T, typename... ctxs>
	std::enable_if_t<(std::is_standard_layout<T>::value && std::is_trivial<T>::value)
					 ,std::unique_ptr<std::decay_t<T> > > from_bytes(DeserializationManager<ctxs...>*, char const *v){
		using T2 = std::decay_t<T>;
		if (v) {
			auto t = std::make_unique<T2>(*(T2*)v);
			//std::memcpy(t.get(),v,sizeof(T));
			return std::move(t);
		}
		else return nullptr;
	}

	template<typename T, typename... ctxs>
	std::enable_if_t<(std::is_standard_layout<T>::value && std::is_trivial<T>::value)
					 ,context_ptr<std::decay_t<T> > > from_bytes_noalloc(DeserializationManager<ctxs...>*, char *v, context_ptr<T> = context_ptr<T>{}){
		using T2 = std::decay_t<T>;
		return context_ptr<T2>{(T2*)v};
	}

	template<typename T, typename... ctxs>
	std::enable_if_t<(std::is_standard_layout<T>::value && std::is_trivial<T>::value)
					 ,context_ptr<const std::decay_t<T> > > from_bytes_noalloc(DeserializationManager<ctxs...>*, char const * const v, context_ptr<T>){
		using T2 = std::decay_t<T>;
		return context_ptr<const T2>{(const T2*)v};
	}

	//Templates that become true_type when matched to the thing they identify,
	//or become false_type if they fail to match, similar to std::is_pod

	template<typename>
	struct is_pair : std::false_type {};

	template<typename T, typename U>
	struct is_pair<std::pair<T,U> > : std::true_type {};
	
	template<typename>
    struct is_list : std::false_type {};

    template<typename T>
    struct is_list<std::list<T> > : std::true_type {};

    template<typename>
    struct is_map : std::false_type{};

    template<typename K, typename V>
    struct is_map<std::map<K,V>>: std::true_type {};

	template<typename T, typename... ctxs>
	std::unique_ptr<type_check<is_string,T> > from_bytes(DeserializationManager<ctxs...>*, char const *v){
		assert(v);
		return std::make_unique<T>(v);
	}
	

	template<typename T, typename... ctxs>
	context_ptr<type_check<is_string,T> > from_bytes_noalloc(DeserializationManager<ctxs...>*, char const *v, context_ptr<T> = context_ptr<T>{}){
		assert(v);
		return context_ptr<T>(new std::string{v});
	}
	
	template<typename T, typename... ctxs>
	std::unique_ptr<type_check<is_set,T> > from_bytes(DeserializationManager<ctxs...>* ctx, const char* _v) {
		int size = ((int*)_v)[0];
		const char* v = _v + sizeof(int);
		auto r = std::make_unique<std::set<typename T::key_type> >();
		for (int i = 0; i < size; ++i){
			auto e = from_bytes<typename T::key_type>(ctx,v);
			v += bytes_size(*e);
			r->insert(*e);
		}
		return std::move(r);
	}

	template<typename T, typename... ctxs>
	context_ptr<type_check<is_set,T> > from_bytes_noalloc(DeserializationManager<ctxs...>* ctx, char const *v, context_ptr<T> = context_ptr<T>{}){
		return context_ptr<T>{from_bytes<T>(ctx,v).release()};
	}

	template<typename T, typename... ctxs>
	std::unique_ptr<type_check<is_pair,T > > from_bytes(DeserializationManager<ctxs...>* ctx, const char * v){
		using ft = typename T::first_type;
		using st = typename T::second_type;
		auto fst = from_bytes_noalloc<ft>(ctx,v);
		return std::make_unique<std::pair<ft,st>>(
		        *fst, *from_bytes_noalloc<st>(ctx,v + bytes_size(*fst)));
	}

	template<typename L, typename... ctxs>
    std::unique_ptr<type_check<is_list,L> > from_bytes(DeserializationManager<ctxs...>* ctx, const char * buffer){
	    using elem = typename L::value_type;
	    int size = ((int*) buffer)[0];
	    const char* buf_ptr = buffer + sizeof(int);
	    std::unique_ptr<std::list<elem>> return_list{new L()};
	    for(int i = 0; i < size; ++i) {
				auto item = from_bytes_noalloc<elem>(ctx, buf_ptr, context_ptr<elem>{});
	        buf_ptr += bytes_size(*item);
	        return_list->push_back(*item);
	    }
	    return std::move(return_list);
	}


	template<typename T, typename... ctxs>
	context_ptr<type_check<is_pair,T> > from_bytes_noalloc(DeserializationManager<ctxs...>* ctx, char const *v, context_ptr<T> = context_ptr<T>{}){
		return context_ptr<T>{from_bytes<T>(ctx,v).release()};
	}

	template<typename... ctxs>
	std::unique_ptr<std::vector<bool> > boolvec_from_bytes(DeserializationManager<ctxs...>* ctx, char const * v){
		const std::size_t real_size = *from_bytes_noalloc<std::size_t>(ctx,v);
		v += bytes_size(real_size);
		unsigned char* converted = (unsigned char*) v;
		auto _ret = std::make_unique<std::vector<bool>>(real_size);
		std::vector<bool> &ret = *_ret;
		auto k = 0u;
		auto j = 0u;
		for (; j < real_size; ++k){
			for(int i=0;i<8 && j < real_size;(i++, ++j))
			{
				ret[j] = converted[k] & (1 << i);
			}
		}
		assert(j == real_size);
		return _ret;
	}

	template<typename T, typename... ctxs> std::unique_ptr<T> boolvec_trampoline(DeserializationManager<ctxs...>* ctx, char const * v, std::true_type*){
		return boolvec_from_bytes(ctx,v);
	}
	
	template<typename T, typename... ctxs> std::unique_ptr<T> boolvec_trampoline(DeserializationManager<ctxs...>*, char const * , std::false_type*){
		struct impossible{}; throw impossible{};
	}
	
	//Note: T is the type of the vector, not the vector's type parameter T
	template<typename T, typename... ctxs>
	std::enable_if_t<is_vector<T>::value,std::unique_ptr<T> > from_bytes(DeserializationManager<ctxs...>* ctx, char const * v){
		using member = typename T::value_type;
		if (std::is_same<bool,member>::value){
			std::integral_constant<bool, std::is_same<bool,member>::value> *choice{nullptr};
			return boolvec_trampoline<T>(ctx,v,choice);
		}
		else {
#ifndef NDEBUG
		const static std::string typenonce = type_name<T>();
		const auto typenonce_size = bytes_size(typenonce);
		auto remote_string = *from_bytes<std::string>(ctx,v);
		if (typenonce != remote_string) {
			std::cout << typenonce << std::endl << std::endl;
			std::cout << remote_string << std::endl;
		}
		assert(typenonce == v);
		assert(typenonce == remote_string);
		v += typenonce_size;
#endif

		if (std::is_pod<member>::value && !std::is_same<bool,member>::value){
			member const * const start = (member*) (v + sizeof(int));
			const int size = ((int*)v)[0];
			return std::unique_ptr<T>{new T{start, start + size}};
		}
		else{
			int size = ((int*)v)[0];
			auto* v2 = v + sizeof(int);
			std::size_t accumulated_offset = 0;
			std::unique_ptr<std::vector<member> > accum{new T()};
			for(int i = 0; i < size; ++i){
				std::unique_ptr<member> item = from_bytes<member>(ctx,v2 + accumulated_offset);
				accumulated_offset += bytes_size(*item);
				accum->push_back(*item);
			}
			return accum;
		}
		}
	}


	template<typename T, typename... ctxs>
	context_ptr<type_check<is_vector,T> > from_bytes_noalloc(DeserializationManager<ctxs...>* ctx, char const *v, context_ptr<T> = context_ptr<T>{}){
		return context_ptr<T>{from_bytes<T>(ctx,v).release()};
	}

	
	template<typename T, typename... ctxs>
	std::enable_if_t<is_map<T>::value, std::unique_ptr<T>> from_bytes(DeserializationManager<ctxs...>* ctx, char const* buffer) {
	    using key_t = typename T::key_type;
	    using value_t = typename T::mapped_type;
        int size = ((int*) buffer)[0];
	    const char* buf_ptr = buffer + sizeof(int);

	    auto new_map = std::make_unique<T>();
	    for(int i = 0; i < size; ++i) {
			auto key = from_bytes_noalloc<key_t>(ctx, buf_ptr, context_ptr<key_t>{});
	        buf_ptr += bytes_size(*key);
	        auto value = from_bytes_noalloc<value_t>(ctx, buf_ptr, context_ptr<value_t>{});
	        buf_ptr += bytes_size(*value);
	        new_map->emplace(*key, *value);
	    }
	    return std::move(new_map);
	}

	template<typename T, typename... ctxs>
    context_ptr<type_check<is_map,T> > from_bytes_noalloc(DeserializationManager<ctxs...>* ctx, char const *v, context_ptr<T> = context_ptr<T>{}){
        return context_ptr<T>{from_bytes<T>(ctx,v).release()};
    }

	/**
	   For Serializing and Deserializing many objects at once.
	   The deserialization expects unique_ptr references; this
	   is important!
	   Also the buffer is at the beginning! This is important.
	 */
	
	std::size_t to_bytes_v(char *);
	
	template<typename T, typename... Rest>
	std::size_t to_bytes_v(char *buf, const T &first, const Rest& ... rest){
		auto size = to_bytes(first,buf);
		return size + to_bytes_v(buf + size,rest...);
	}

	template<typename... ctxs>
	std::size_t from_bytes_v(DeserializationManager<ctxs...>*, char const * const ){
		return 0;
	}
	
	template<typename T, typename DSM, typename... Rest>
	std::size_t from_bytes_v(DSM *dsm, char const * const buf, std::unique_ptr<T> &first, Rest& ... rest){
		first = from_bytes<T>(dsm,buf);
		auto size = bytes_size(*first);
		return size + from_bytes_v(dsm,buf + size,rest...);
	}

	template<typename... ctxs>
	std::size_t from_bytes_noalloc_v(DeserializationManager<ctxs...>*, char const * const ){
		return 0;
	}
	
	template<typename T, typename DSM, typename... Rest>
	std::size_t from_bytes_noalloc_v_nc(DSM *dsm, char * buf, context_ptr<T> &first, context_ptr<Rest>& ... rest){
		first = from_bytes_noalloc<T>(dsm,buf,context_ptr<T>{});
		auto size = bytes_size(*first);
		return size + from_bytes_noalloc_v(dsm,buf + size,rest...);
	}

	template<typename T, typename DSM, typename... Rest>
	std::size_t from_bytes_noalloc_v(DSM *dsm, char * buf, context_ptr<T> &first, context_ptr<Rest>& ... rest){
		return from_bytes_noalloc_v_nc(dsm,buf,first,rest...);
	}

	template<typename T, typename DSM, typename... Rest>
	std::size_t from_bytes_noalloc_v(DSM *dsm, char const * const buf, context_ptr<const T> &first, context_ptr<const Rest>& ... rest){
		first = from_bytes_noalloc<T>(dsm,buf, context_ptr<T>{});
		auto size = bytes_size(*first);
		return size + from_bytes_noalloc_v(dsm,buf + size,rest...);
	}
	
	//sample of how this might work.  Nocopy, plus complete memory safety, but
	//at the cost of callback land.
	template<typename T, typename F, typename... ctxs>
	auto deserialize_and_run(DeserializationManager<ctxs...>* dsm, char * v, const F& fun){
		using fun_t = std::function<std::result_of_t<F(T&)> (T&)>;
		//ensure implicit conversion can run
		static_assert(std::is_convertible<F, fun_t>::value,
					  "Error: type mismatch on function and target deserialialized type");
		return fun(*from_bytes_noalloc<T>(dsm,v));
	}

	//sample of how this might work.  Nocopy, plus complete memory safety, but
	//at the cost of callback land.
	template<typename F, typename R, typename DSM, typename... Args>
	auto deserialize_and_run(DSM* dsm, char * v, const F& fun, std::function<R (Args&...)> const * const){
		using result_t = std::result_of_t<F(Args&...)>;
		static_assert(std::is_same<result_t,R>::value,"Error: function types mismatch.");
		using fun_t = std::function<result_t (Args&...)>;
		//ensure implicit conversion can run
		static_assert(std::is_convertible<F, fun_t>::value,
					  "Error: type mismatch on function and target deserialialized type");
		std::tuple<DSM*, char*, context_ptr<Args>...> args_tuple;
		std::get<0>(args_tuple) = dsm;
		std::get<1>(args_tuple) = v;
		auto size = callFunc<std::size_t,decltype(args_tuple), /*Args: */ DSM*, char *, context_ptr<Args>&...>
			(from_bytes_noalloc_v_nc<Args...>,args_tuple);
		return callFunc([&fun](const auto&, const auto&, auto&... a){return fun(*a...);},args_tuple);
	}

	template<typename F, typename... ctxs>
	auto deserialize_and_run(DeserializationManager<ctxs...>* dsm, char * v, const F& fun){
		using fun_t = std::decay_t<decltype(convert(fun))>;
		return deserialize_and_run<F>(dsm,v,fun,(fun_t*) nullptr);
	}

	template<typename T>
	auto from_bytes(std::nullptr_t, char const * v){
		constexpr DeserializationManager<>* np{nullptr};
		return from_bytes<T>(np,v);
	}

}
