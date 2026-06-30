#pragma once

#include <algorithm>
#include "sequence/MutableArraySequence.hpp"
#include "sequence/Exceptions.hpp"

template <class T>
static void swap(T& a, T& b) {
	T temp = a;
	a = b;
	b = temp;
}

template <class T>
class PriorityQueue {
public:
	PriorityQueue(): data_(new T[10]), size_(0), capacity_(10) {}

	bool IsEmpty() const { return size_ == 0; }

	T GetMin() const {
		if (size_ == 0) throw EmptyCollectionError();
		return data_[0];
	}

	T ExtractMin() {
		T min = GetMin();
		data_[0] = data_[size_ - 1];
		--size_;
		SiftDown(0);
		return min;
	}

	void Insert(T key) {
		if (size_ >= capacity_) Resize(capacity_ + 10);
		++size_;
		data_[size_ - 1] = key;
		SiftUp(size_ - 1);
	}

private:
	T* data_;
	size_t size_;
	size_t capacity_;

	void SiftDown(int i) {
		int j;
		while (2 * i + 1 < size_) {
			int left = 2 * i + 1;
			int right = 2 * i + 2;
			j = left;
			if (right < size_ && data_[right] < data_[left]) j = right;
			
			if (data_[j] < data_[i]) swap(data_[i], data_[j]);
			i = j;
		}
	}

	void SiftUp(int i) {
		while (data_[i] < data_[(i - 1) / 2]) {
			swap(data_[i], data_[(i - 1) / 2]);
			i = (i - 1) / 2;
		}
	}
	
	void Resize(size_t new_size) {
		if (new_size < 0) throw IndexOutOfRange("Resize(): Size must be positive");
		T* new_data = new T[new_size];
		const int count = (size_ < new_size) ? size_ : new_size;
		std::copy(data_, data_ + count, new_data);
		delete[] data_;
		data_ = new_data;
		capacity_ = new_size;
		size_ = count;
	}
};