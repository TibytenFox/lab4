#include "LazySequence.hpp"
#include "Stream.hpp"
#include "PriorityQueue.hpp"
#include "sequence/MutableArraySequence.hpp"

template <class T>
void SortStreamWithPriorityQueue(ReadOnlyStream<T>& istream, WriteOnlyStream<T>& ostream) {
	PriorityQueue<T> min_heap;

	while (!istream.IsEndOfStream()) {
		try {
			T item = istream.Read();
			min_heap.Insert(item);
		} catch (const EndOfStream&) {
			break;
		}
	}

	while (!min_heap.IsEmpty()) {
		T min_item = min_heap.ExtractMin();
		ostream.Write(min_item);
	}
}