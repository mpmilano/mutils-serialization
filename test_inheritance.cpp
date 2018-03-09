#include "Inheritance.hpp"
#include "SerializationMacros.hpp"
#include <iostream>

using namespace mutils;

struct A : public InheritByteRepresentable {
	virtual ~A() = default;
};

struct B: public A, public ByteRepresentable {
	const int data;
	B(int data):data(data){}

	INHERIT_SERIALIZATION_SUPPORT(B,A,9846,data);
	
};

template<typename, typename> struct D : public InheritByteRepresentable{
};

template<typename T>
struct C: public D<T,int>, public ByteRepresentable{
	const int data;
	C(int data):data(data){}

	INHERIT_SERIALIZATION_SUPPORT(C,A,98467,data);	
};

template<typename Y> using Dindirect = D<Y,int>;

int main(){
	B b{234};
	C<int> c{23423};
	auto ig = InheritGroup<InheritPairAbs1<Dindirect, C, 98467> >::add_class(b);
	DeserializationManager<DECT(ig)> dsm{&ig};
	
	A& ab = b;
	D<int,int>& ac = c;
	char bbuf[256];
	char cbuf[256];
	ab.inherit_to_bytes(bbuf);
	ac.inherit_to_bytes(cbuf);
	auto ab2 = inherit_from_bytes<A>(&dsm,bbuf);
	auto ac2 = inherit_from_bytes<Dindirect<int>>(&dsm,cbuf);
	assert(inherit_from_bytes<Dindirect<int>>(&dsm,cbuf));
	assert(ab2);
	assert(ac2);
	assert(dynamic_cast<B*>(ab2.get())->data == b.data);	
	assert(dynamic_cast<C<int>*>(ac2.get())->data == c.data);
	std::cout << dynamic_cast<C<int>*>(ac2.get())->data << " " << dynamic_cast<B*>(ab2.get())->data << std::endl;
}
