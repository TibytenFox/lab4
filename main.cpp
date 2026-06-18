#include <iostream>
#include "sequence/Sequence.hpp"
#include "sequence/MutableArraySequence.hpp"
#include "src/LazySequence.hpp"


int fib(const Sequence<int>* seq) {
	size_t l = seq->GetLength();
	return seq->Get(l - 1) + seq->Get(l - 2);
}

int main() {
	MutableArraySequence<int>* s = new MutableArraySequence<int>();
	s->Append(0);
	s->Append(1);

	LazySequence<int> lazy(fib, s);
	int x;
	std::cin >> x;
	std::cout << lazy.Get(x) << '\n';
	return 0;
}