#pragma once

#include "sequence/Sequence.hpp"
#include "sequence/MutableArraySequence.hpp"
#include "sequence/Exceptions.hpp"
#include "Generator.hpp"

struct Cardinal {
    enum class Kind { Finite, Infinite };
    Kind kind;
    size_t value;  // если Finite

	explicit Cardinal(size_t val) : value(val), kind(Kind::Finite) {}
	Cardinal(Kind k, size_t val) : kind(k), value(val) {}

    static Cardinal finite(size_t n) { return {Kind::Finite, n}; }
    static Cardinal infinite() { return {Kind::Infinite, 0}; }

    bool IsFinite() const { return kind == Kind::Finite; }
    bool IsInfinite() const { return kind == Kind::Infinite; }
};

template <class T>
class LazySequence {
public:
	LazySequence() : generator_(nullptr), cache_(new MutableArraySequence<T>()), length_(0) {}
	LazySequence(T* items, int count) : generator_(nullptr), cache_(new MutableArraySequence<T>()), length_(count) {
		for (int i = 0; i < count; ++i) {
            cache_->Append(items[i]);
        }
	}
	LazySequence(const Sequence<T>* seq) 
        : generator_(nullptr), cache_(new MutableArraySequence<T>()), length_(seq->GetLength()) {
        for (size_t i = 0; i < seq->GetLength(); ++i) {
            cache_->Append(seq->Get(i));
        }
    }
	LazySequence(T (*rule)(const Sequence<T>*), const Sequence<T>* seed)
        : cache_(new MutableArraySequence<T>()), length_(Cardinal::Kind::Infinite, 0) {
        generator_ = new RecurrentGenerator<T>(seed, rule);

		for (size_t i = 0; i < seed->GetLength(); ++i) {
			cache_->Append(seed->Get(i));
		}
    }
	~LazySequence() {
        delete generator_;
        delete cache_;
    }

	T Get(size_t index) const {
        return MaterializeUpTo(index);
    }

	T GetFirst() const {
        if (length_.IsFinite() && length_.value == 0) throw IndexOutOfRange();
        return Get(0);
    }

    T GetLast() const {
        if (length_.IsInfinite()) {
            throw RunTimeError();
        }
        if (length_.value == 0) throw IndexOutOfRange("");
        return Get(length_.value - 1);
    }

	size_t GetLength() const { 
        return cache_->GetLength(); 
    }

	Cardinal GetCardinalLength() const {
        return length_;
    }

	size_t GetMaterializedCount() const {
        return cache_->GetLength();
    }

private:
	Generator<T>* generator_;
	MutableArraySequence<T>* cache_;
	Cardinal length_;

	LazySequence(Generator<T>* new_generator, Cardinal new_length) 
		: generator_(new_generator), cache_(new MutableArraySequence<T>()), length_(new_length) {}

	T MaterializeUpTo(size_t index) const {
		if (index < cache_->GetLength()) return cache_->Get(index);
		if (!generator_) throw IndexOutOfRange();

		while (cache_->GetLength() <= index) {
			if (!generator_->HasNext()) throw IndexOutOfRange();
			cache_->Append(generator_->GetNext());
		}

		return cache_->Get(index);
	}
};
