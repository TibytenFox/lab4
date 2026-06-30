#pragma once

#include "sequence/Sequence.hpp"
#include "sequence/MutableArraySequence.hpp"
#include "sequence/Exceptions.hpp"

template <class T1, class T2>
struct Pair {
    T1 first;
    T2 second;

    Pair() {}
    Pair(const T1& f, const T2& s) : first(f), second(s) {}
};

template <class T> class LazySequence;
template <class T> class AppendGenerator;
template <class T> class InsertGenerator;
template <class T> class RemoveGenerator;
template <class T, class T2> class MapGenerator;
template <class T> class WhereGenerator;


template <class T>
class Generator {
public:
	virtual ~Generator() = default;
	virtual T GetNext() = 0;
	virtual bool HasNext() const = 0;

	virtual Generator<T>* Clone() const = 0;

	virtual Generator<T>* Append(const T& item) const = 0;
	virtual Generator<T>* Append(const Sequence<T>* items) const = 0;
	virtual Generator<T>* Insert(const T& item, size_t index) const = 0;
	virtual Generator<T>* Insert(const Sequence<T>* items, size_t index) const = 0;
	virtual Generator<T>* Remove(size_t index) const = 0;
	virtual Generator<T>* Remove(size_t index, size_t count) const = 0;
	template <class T2>
	Generator<T2>* Map(T2 (*func)(T)) const { return new MapGenerator<T, T2>(this->Clone(), func); }
	virtual Generator<T>* Where(bool (*predicate)(T)) const = 0;

	virtual size_t GetMaterializedCount() const = 0;
};

template <class T>
class SequenceGenerator : public Generator<T> {
public:
    explicit SequenceGenerator(const Sequence<T>* seq) : seq_(seq->Clone()), index_(0) {}
    ~SequenceGenerator() override { delete seq_; }
    Generator<T>* Clone() const override { return new SequenceGenerator<T>(seq_); }
    T GetNext() override { if (!HasNext()) throw RunTimeError("End"); return seq_->Get(index_++); }
    bool HasNext() const override { return index_ < seq_->GetLength(); }
    size_t GetMaterializedCount() const override { return seq_->GetLength(); }
    Generator<T>* Append(const T& item) const override { return new AppendGenerator<T>(this->Clone(), item); }
    Generator<T>* Append(const Sequence<T>* items) const override { return new AppendGenerator<T>(this->Clone(), items); }
    Generator<T>* Insert(const T& item, size_t index) const override { return new InsertGenerator<T>(this->Clone(), item, index); }
    Generator<T>* Insert(const Sequence<T>* items, size_t index) const override { return new InsertGenerator<T>(this->Clone(), items, index); }
    Generator<T>* Remove(size_t index) const override { return new RemoveGenerator<T>(this->Clone(), index); }
    Generator<T>* Remove(size_t index, size_t count) const override { return new RemoveGenerator<T>(this->Clone(), index, count); }
	template <class T2>
	Generator<T2>* Map(T2 (*func)(T)) const { return new MapGenerator<T, T2>(this->Clone(), func); }
	Generator<T>* Where(bool (*predicate)(T)) const override { return new WhereGenerator<T>(this->Clone(), predicate); }
private:
    const Sequence<T>* seq_;
    size_t index_ = 0;
};

template <class T>
class RecurrentGenerator : public Generator<T> {
public:
	RecurrentGenerator(const Sequence<T>* seed, T (*rule)(const Sequence<T>*)) 
		: seed_(seed->Clone()), rule_(rule), infinite_(true), cache_(seed->Clone()), size_(0), next_index_(0) {}
	RecurrentGenerator(const Sequence<T>* seed, T (*rule)(const Sequence<T>*), size_t total_length) 
		: seed_(seed->Clone()), rule_(rule), infinite_(false), cache_(seed->Clone()), size_(total_length), next_index_(0) {}

	~RecurrentGenerator() override {
        delete seed_;
        delete cache_;
    }

	Generator<T>* Clone() const override {
        if (infinite_) return new RecurrentGenerator<T>(seed_, rule_);
        return new RecurrentGenerator<T>(seed_, rule_, size_);
    }
	
	T GetNext() override {
		if (!HasNext()) throw RunTimeError("End of collection");
		return compute(next_index_++);
	}

	bool HasNext() const override {
		if (infinite_) return true;
		return cache_->GetLength() < size_;
	}

	size_t GetMaterializedCount() const override {
		return cache_->GetLength();
	}

	Generator<T>* Append(const T& item) const override {
		return new AppendGenerator<T>(this->Clone(), item);
	}
	Generator<T>* Append(const Sequence<T>* items) const override {
		return new AppendGenerator<T>(this->Clone(), items);
	}
	Generator<T>* Insert(const T& item, size_t index) const override {
		return new InsertGenerator<T>(this->Clone(), item, index);
	}
	Generator<T>* Insert(const Sequence<T>* items, size_t index) const override {
		return new InsertGenerator<T>(this->Clone(), items, index);
	}
	Generator<T>* Remove(size_t index) const override {
		return new RemoveGenerator<T>(this->Clone(), index);
	}
	Generator<T>* Remove(size_t index, size_t count) const override {
		return new RemoveGenerator<T>(this->Clone(), index, count);
	}
	template <class T2>
	Generator<T2>* Map(T2 (*func)(T)) const {
		return new MapGenerator<T, T2>(this->Clone(), func);
	}
	Generator<T>* Where(bool (*predicate)(T)) const override {
		return new WhereGenerator<T>(this->Clone(), predicate);
	}

private:
	T (*rule_)(const Sequence<T>*);
	Sequence<T>* seed_;
	Sequence<T>* cache_;
	bool infinite_;
	size_t size_; // максимальный размер, если бесконечный, то равен 0
	size_t next_index_;

	T compute(size_t index) {
		if (index < cache_->GetLength()) return cache_->Get(index);
		if (!infinite_ && index >= size_) throw IndexOutOfRange();

		for (size_t i = cache_->GetLength(); i <= index; ++i) {
			if (i < seed_->GetLength()) { 
				cache_->Append(seed_->Get(i)); 
			} else {
				T next = rule_(cache_);
				cache_->Append(next);
			}
		}

		return cache_->Get(index);
	}
};


template <class T>
class GeneratorDecorator : public Generator<T> {
public:
	explicit GeneratorDecorator(Generator<T>* base) : base_(base) {}

	~GeneratorDecorator() override {
        delete base_; 
    }

	T GetNext() override { return base_->GetNext(); }
    bool HasNext() const override { return base_->HasNext(); }
	size_t GetMaterializedCount() const override { return base_->GetMaterializedCount(); }

	Generator<T>* Append(const T& item) const override {
        return new AppendGenerator<T>(this->Clone(), item);
    }
    Generator<T>* Append(const Sequence<T>* items) const override {
        return new AppendGenerator<T>(this->Clone(), items);
    }
    Generator<T>* Insert(const T& item, size_t index) const override {
        return new InsertGenerator<T>(this->Clone(), item, index);
    }
    Generator<T>* Insert(const Sequence<T>* items, size_t index) const override {
        return new InsertGenerator<T>(this->Clone(), items, index);
    }
    Generator<T>* Remove(size_t index) const override {
        return new RemoveGenerator<T>(this->Clone(), index);
    }
    Generator<T>* Remove(size_t index, size_t count) const override {
        return new RemoveGenerator<T>(this->Clone(), index, count);
    }
	template <class T2>
	Generator<T2>* Map(T2 (*func)(T)) const {
		return new MapGenerator<T, T2>(this->Clone(), func);
	}
	Generator<T>* Where(bool (*predicate)(T)) const override {
		return new WhereGenerator<T>(this->Clone(), predicate);
	}


protected:
	Generator<T>* base_;
};

template <class T>
class AppendGenerator : public GeneratorDecorator<T> {
public:
	AppendGenerator(Generator<T>* base, const T& item) : GeneratorDecorator<T>(base), appended_consumed_(false), pos_(0) {
		MutableArraySequence<T>* items = new MutableArraySequence<T>();
		items->Append(item);
		append_items_ = items;
	}
	AppendGenerator(Generator<T>* base, const Sequence<T>* items) 
		: GeneratorDecorator<T>(base), append_items_(items->Clone()), appended_consumed_(false), pos_(0) {}

	~AppendGenerator() override {
    	delete append_items_;
	}

	Generator<T>* Clone() const override {
        return new AppendGenerator<T>(this->base_->Clone(), append_items_);
    }

	T GetNext() override {
		if (this->base_->HasNext()) {
			return this->base_->GetNext();
		} else {
			if (pos_ < append_items_->GetLength()) {
				return append_items_->Get(pos_++);
			} else {
				throw RunTimeError("End of collection");
			}
		}
	}

	bool HasNext() const override {
		if (this->base_->HasNext()) return true;
		return pos_ < append_items_->GetLength();
	}

private:
	Sequence<T>* append_items_;
	bool appended_consumed_;
	size_t pos_;
};

template <class T>
class InsertGenerator : public GeneratorDecorator<T> {
public:
	InsertGenerator(Generator<T>* base, const T& item, size_t index) : GeneratorDecorator<T>(base), inserted_consumed_(false), pos_(0), start_(index) {
		MutableArraySequence<T>* items = new MutableArraySequence<T>();
		items->Append(item);
		insert_items_ = items;
	}
	InsertGenerator(Generator<T>* base, const Sequence<T>* items, size_t index) 
		: GeneratorDecorator<T>(base), insert_items_(items->Clone()), inserted_consumed_(false), pos_(0), start_(index) {}

	~InsertGenerator() override {
		delete insert_items_;
	}	

	Generator<T>* Clone() const override {
        return new InsertGenerator<T>(this->base_->Clone(), insert_items_, start_);
    }
		
	T GetNext() override {
		if (pos_ < start_ || pos_ >= start_ + insert_items_->GetLength()) {
			if (!this->base_->HasNext()) throw RunTimeError();
			T val = this->base_->GetNext();
			++pos_;
			return val;
		} else if (pos_ < start_ + insert_items_->GetLength()) {
			T val = insert_items_->Get(pos_ - start_);
			++pos_;
			return val;
		} else {
			if (!this->base_->HasNext()) throw RunTimeError("End of collection");
            T val = this->base_->GetNext();
            ++pos_;
            return val;
		}
	}

	bool HasNext() const override {
		if (start_ <= pos_ && pos_ < start_ + insert_items_->GetLength()) return true;
		return this->base_->HasNext();
	}

private:
	Sequence<T>* insert_items_;
	bool inserted_consumed_;
	size_t pos_;
	size_t start_;
};

template <class T>
class RemoveGenerator : public GeneratorDecorator<T> {
public:
	RemoveGenerator(Generator<T>* base, size_t start, size_t count = 1)
        : GeneratorDecorator<T>(base), start_(start), count_(count), exhausted_(false), pos_(0) {}

	Generator<T>* Clone() const override {
        return new RemoveGenerator<T>(this->base_->Clone(), start_, count_);
    }

	T GetNext() override {
		if (exhausted_) throw EmptyCollectionError();
		while (pos_ >= start_ && pos_ < start_ + count_) {
			if (!this->base_->HasNext()) {
				exhausted_ = true;
				throw RunTimeError("End of collection");
			}
			this->base_->GetNext();
			++pos_;
		}

		if (!this->base_->HasNext()) {
			exhausted_ = true;
			throw RunTimeError("End of collection");
		}

		++pos_;
		return this->base_->GetNext();
	}

	bool HasNext() const override {
		if (exhausted_) return false;
		return this->base_->HasNext();
	}

private:
	size_t start_;
	size_t count_;
	size_t pos_;
	bool exhausted_;
};

template <class T, class T2>
class MapGenerator : public Generator<T2> {
public:
	MapGenerator(Generator<T>* base, T2 (*func)(T)) : base_(base->Clone()), func_(func) {}
	Generator<T2>* Clone() const override { return new MapGenerator<T, T2>(base_, func_); }
	T2 GetNext() override { return func_(base_->GetNext()); }
	bool HasNext() const override { return base_->HasNext(); }
	size_t GetMaterializedCount() const override { return base_->GetMaterializedCount(); }
	Generator<T2>* Append(const T2& item) const override { return new AppendGenerator<T2>(this->Clone(), item); }
    Generator<T2>* Append(const Sequence<T2>* items) const override { return new AppendGenerator<T2>(this->Clone(), items); }
    Generator<T2>* Insert(const T2& item, size_t index) const override { return new InsertGenerator<T2>(this->Clone(), item, index); }
    Generator<T2>* Insert(const Sequence<T2>* items, size_t index) const override { return new InsertGenerator<T2>(this->Clone(), items, index); }
    Generator<T2>* Remove(size_t index) const override { return new RemoveGenerator<T2>(this->Clone(), index); }
    Generator<T2>* Remove(size_t index, size_t count) const override { return new RemoveGenerator<T2>(this->Clone(), index, count); }
	template <class T3>
	Generator<T3>* Map(T3 (*func)(T2)) const { return new MapGenerator<T2, T3>(this->Clone(), func); }
	Generator<T2>* Where(bool (*predicate)(T)) const override { return new WhereGenerator<T>(this->Clone(), predicate); }
private:
	Generator<T>* base_;
	T2 (*func_)(T);
};

template <class T>
class WhereGenerator : public GeneratorDecorator<T> {
public:
	WhereGenerator(Generator<T>* base, bool (*predicate)(T)) 
		: GeneratorDecorator<T>(base), predicate_(predicate) {}

	Generator<T>* Clone() const override { return new WhereGenerator<T>(this->base_->Clone(), predicate_); }

	T GetNext() override {
        if (!HasNext()) throw RunTimeError("End");
        has_cached_ = false;
        return cached_item_;
    }

    bool HasNext() const override {
        if (has_cached_) return true;
        while (this->base_->HasNext()) {
            T item = this->base_->GetNext();
            if (predicate_(item)) {
                const_cast<WhereGenerator<T>*>(this)->cached_item_ = item;
                const_cast<WhereGenerator<T>*>(this)->has_cached_ = true;
                return true;
            }
        }
        return false;
    }

private:
	bool (*predicate_)(T);
	T cached_item_;
	bool has_cached_ = false;
};


template <class T, class T2>
class ZipGenerator : public Generator<Pair<T, T2>> {
public:
	ZipGenerator(Generator<T>* first, Generator<T>* second) : first_(first), second_(second) {}
	~ZipGenerator() { delete first_; delete second_; }
	ZipGenerator<T, T2>* Clone() const override { return new ZipGenerator<T, T2>(first_, second_); }
	Pair<T, T2> GetNext() override { return Pair<T, T2>(first_->GetNext(), second_->GetNext()); }
	bool HasNext() const override { return first_->HasNext() && second_->HasNext(); }

	size_t GetMaterializedCount() const override {
		size_t c1 = first_->GetMaterializedCount();
        size_t c2 = second_->GetMaterializedCount();
        return (c1 < c2) ? c1 : c2;
	}

	Generator<Pair<T, T2>>* Append(const Pair<T, T2>& item) const override { return new AppendGenerator<Pair<T, T2>>(this->Clone(), item); }
    Generator<Pair<T, T2>>* Append(const Sequence<Pair<T, T2>>* items) const override { return new AppendGenerator<Pair<T, T2>>(this->Clone(), items); }
    Generator<Pair<T, T2>>* Insert(const Pair<T, T2>& item, size_t index) const override { return new InsertGenerator<Pair<T, T2>>(this->Clone(), item, index); }
    Generator<Pair<T, T2>>* Insert(const Sequence<Pair<T, T2>>* items, size_t index) const override { return new InsertGenerator<Pair<T, T2>>(this->Clone(), items, index); }
    Generator<Pair<T, T2>>* Remove(size_t index) const override { return new RemoveGenerator<Pair<T, T2>>(this->Clone(), index); }
    Generator<Pair<T, T2>>* Remove(size_t index, size_t count) const override { return new RemoveGenerator<Pair<T, T2>>(this->Clone(), index, count); }
	Generator<Pair<T, T2>>* Where(bool (*predicate)(Pair<T, T2>)) const override { return new WhereGenerator<Pair<T, T2>>(this->Clone(), predicate); }
private:
	Generator<T>* first_;
	Generator<T>* second_;
};