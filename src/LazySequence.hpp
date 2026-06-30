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
    // Разрешаем обращение к приватным полям классам LazySequence любого типа
    template <class AnyType> friend class LazySequence;

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
    }
    LazySequence(const LazySequence<T>& other) : length_(other.length_) {
        generator_ = other.generator_->Clone();
        cache_ = other.cache_->Clone();
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
        if (length_.value == 0) throw IndexOutOfRange();
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

	// Append одного элемента
    LazySequence<T>* Append(const T& item) const {
        if (!generator_) {
			MutableArraySequence<T>* new_cache = new MutableArraySequence<T>();
            for (size_t i = 0; i < cache_->GetLength(); ++i) {
                new_cache->Append(cache_->Get(i));
            }
            new_cache->Append(item);
            return new LazySequence<T>(new_cache);
		}
        Cardinal new_length = length_.IsInfinite() ? Cardinal::infinite() : Cardinal::finite(length_.value + 1);
        return new LazySequence<T>(generator_->Append(item), new_length);
    }

    // Append последовательности
    LazySequence<T>* Append(const Sequence<T>* items) const {
        if (!generator_) {
			MutableArraySequence<T>* new_cache = new MutableArraySequence<T>();
            for (size_t i = 0; i < cache_->GetLength(); ++i) {
                new_cache->Append(cache_->Get(i));
            }
            for (size_t i = 0; i < items->GetLength(); ++i) {
                new_cache->Append(items->Get(i));
            }
            return new LazySequence<T>(new_cache);
		}
        Cardinal new_length = length_.IsInfinite() ? Cardinal::infinite() : Cardinal::finite(length_.value + items->GetLength());
        return new LazySequence<T>(generator_->Append(items), new_length);
    }

    // Insert одного элемента
    LazySequence<T>* Insert(const T& item, size_t index) const {
		if (length_.IsFinite() && index > length_.value) throw IndexOutOfRange();
        if (!generator_) {
			if (index > cache_->GetLength()) throw IndexOutOfRange();
            MutableArraySequence<T>* new_cache = new MutableArraySequence<T>();
            for (size_t i = 0; i < cache_->GetLength(); ++i) {
                new_cache->Append(cache_->Get(i));
            }
            new_cache->InsertAt(item, index); 
            return new LazySequence<T>(new_cache);
		}
        
        Cardinal new_length = length_.IsInfinite() ? Cardinal::infinite() : Cardinal::finite(length_.value + 1);
        return new LazySequence<T>(generator_->Insert(item, index), new_length);
    }

    // Insert последовательности
    LazySequence<T>* Insert(const Sequence<T>* items, size_t index) const {
		if (length_.IsFinite() && index > length_.value) throw IndexOutOfRange();
        if (!generator_) {
			if (index > cache_->GetLength()) throw IndexOutOfRange();
            MutableArraySequence<T>* new_cache = new MutableArraySequence<T>();
            for (size_t i = 0; i < cache_->GetLength(); ++i) {
                new_cache->Append(cache_->Get(i));
            }
            for (size_t i = 0; i < items->GetLength(); ++i) {
                new_cache->InsertAt(items->Get(i), index + i);
            }
            return new LazySequence<T>(new_cache);
		}
        Cardinal new_length = length_.IsInfinite() ? Cardinal::infinite() : Cardinal::finite(length_.value + items->GetLength());
        return new LazySequence<T>(generator_->Insert(items, index), new_length);
    }

    // Remove одного элемента
    LazySequence<T>* Remove(size_t index) const {
        return Remove(index, 1);
    }

    // Remove нескольких элементов
    LazySequence<T>* Remove(size_t index, size_t count) const {
		if (length_.IsFinite() && index + count > length_.value) throw IndexOutOfRange();
        if (!generator_) {
			if (index + count > cache_->GetLength()) throw IndexOutOfRange();
            MutableArraySequence<T>* new_cache = new MutableArraySequence<T>();
            for (size_t i = 0; i < index; ++i) {
                new_cache->Append(cache_->Get(i));
            }
			for (size_t i = index + count; i < cache_->GetLength(); ++i) {
				new_cache->Append(cache_->Get(i));
			}
            return new LazySequence<T>(new_cache);
		}
        if (length_.IsFinite()) {
            if (index + count > length_.value) throw IndexOutOfRange();
            Cardinal new_length = Cardinal::finite(length_.value - count);
            return new LazySequence<T>(generator_->Remove(index, count), new_length);
        } else {
            // Для бесконечной последовательности удаление конечного числа элементов не меняет мощность
            return new LazySequence<T>(generator_->Remove(index, count), Cardinal::infinite());
        }
    }

    template <class T2>
    LazySequence<T2>* Map(T2 (*func)(T)) const {
        if (!generator_) {
            Generator<T>* seq_gen = new SequenceGenerator<T>(cache_);
            return new LazySequence<T2>(new MapGenerator<T, T2>(seq_gen, func), length_);
        }
        
        Generator<T>* cloned_gen = generator_->Clone();
        return new LazySequence<T2>(cloned_gen->Map(func), length_);
    }

    LazySequence<T>* Where(bool (*predicate)(T)) const {
        if (!generator_) {
            Generator<T>* seq_gen = new SequenceGenerator<T>(cache_);
            // Infinite как неопределенная длина
            return new LazySequence<T>(new WhereGenerator<T>(seq_gen, predicate), Cardinal::infinite());
        }

        Generator<T>* cloned_gen = generator_->Clone();
        return new LazySequence<T>(cloned_gen->Where(predicate), Cardinal::infinite());
    }

    template <class T2>
    T2 Reduce(T2 (*func)(T2, T), T2 init_value) const {
        if (length_.IsInfinite()) throw RunTimeError();
        T2 result = init_value;
        size_t target_len = length_.value;
        for (size_t i = 0; i < target_len; ++i) {
            result = func(result, Get(i));
        }

        return result;
    }

    template <class T2>
    LazySequence<Pair<T, T2>>* Zip(const LazySequence<T2>* other) const {
        Generator<T>* first = generator_ ? generator_->Clone() : new SequenceGenerator<T>(cache_);
        Generator<T2>* second = other->generator_ ? other->generator_->Clone() : new SequenceGenerator<T2>(other->cache_);

        Cardinal new_length = Cardinal::infinite();
        if (length_.IsFinite() || other->length_.IsFinite()) {
            size_t min_length;
            if (length_.IsFinite() && other->length_.IsFinite()) min_length = (length_.value < other->length_.value) ? length_.value : other->length_.value;
            else {
                if (length_.IsFinite()) min_length = length_.value;
                else                    min_length = other->length_.value;
            }
            new_length = Cardinal::finite(min_length);
        }
        return new LazySequence<Pair<T, T2>>(new ZipGenerator<T, T2>(first, second), new_length);
    }

private:
	Generator<T>* generator_;
	MutableArraySequence<T>* cache_;
	Cardinal length_;

	LazySequence(Generator<T>* new_generator, Cardinal new_length) 
		: generator_(new_generator), cache_(new MutableArraySequence<T>()), length_(new_length) {}

	LazySequence(Generator<T>* new_gen, MutableArraySequence<T>* new_cache, Cardinal new_length)
        : generator_(new_gen), cache_(new_cache), length_(new_length) {}

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
