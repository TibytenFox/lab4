#pragma once

#include "sequence/Sequence.hpp"
#include "sequence/MutableArraySequence.hpp"
#include "sequence/Exceptions.hpp"

template <class T> class LazySequence;

template <class T>
class Generator {
public:
	virtual ~Generator() = default;
	virtual T GetNext() = 0;
	virtual bool HasNext() const = 0;

	virtual Generator<T>* Append(const T& item) const = 0;
	virtual Generator<T>* Append(const Sequence<T>* items) const = 0;
	virtual Generator<T>* Insert(const T& item, size_t index) const = 0;
	virtual Generator<T>* Insert(const Sequence<T>* items, size_t index) const = 0;
	virtual Generator<T>* Remove(size_t index) const = 0;
	virtual Generator<T>* Remove(size_t index, size_t count) const = 0;

	virtual size_t GetMaterializedCount() const = 0;
};

template <class T> class AppendGenerator;
template <class T> class InsertGenerator;
template <class T> class RemoveGenerator;

template <class T>
class RecurrentGenerator : public Generator<T> {
public:
	RecurrentGenerator(const Sequence<T>* seed, T (*rule)(const Sequence<T>*)) 
		: seed_(seed->Clone()), rule_(rule), infinite_(true), cache_(seed->Clone()), size_(0) {}
	RecurrentGenerator(const Sequence<T>* seed, T (*rule)(const Sequence<T>*), size_t total_length) 
		: seed_(seed->Clone()), rule_(rule), infinite_(false), cache_(seed->Clone()), size_(total_length) {}

	~RecurrentGenerator() override {
        delete seed_;
        delete cache_;
    }
	
	T GetNext() override {
		size_t next_index = cache_->GetLength();
		if (!infinite_ && next_index >= size_) throw RunTimeError("End of collection");
		return compute(next_index);
	}

	bool HasNext() const override {
		if (infinite_) return true;
		return cache_->GetLength() < size_;
	}

	size_t GetMaterializedCount() const override {
		return cache_->GetLength();
	}

	Generator<T>* Append(const T& item) const override {
		return new AppendGenerator(const_cast<RecurrentGenerator<T>*>(this), item);
	}
	Generator<T>* Append(const Sequence<T>* items) const override {
		return new AppendGenerator(const_cast<RecurrentGenerator<T>*>(this), items);
	}
	Generator<T>* Insert(const T& item, size_t index) const override {
		return new InsertGenerator(const_cast<RecurrentGenerator<T>*>(this), item, index);
	}
	Generator<T>* Insert(const Sequence<T>* items, size_t index) const override {
		return new InsertGenerator(const_cast<RecurrentGenerator<T>*>(this), items, index);
	}
	Generator<T>* Remove(size_t index) const override {
		return new RemoveGenerator(const_cast<RecurrentGenerator<T>*>(this), index);
	}
	Generator<T>* Remove(size_t index, size_t count) const override {
		return new RemoveGenerator(const_cast<RecurrentGenerator<T>*>(this), index, count);
	}

private:
	T (*rule_)(const Sequence<T>*);
	Sequence<T>* seed_;
	Sequence<T>* cache_;
	bool infinite_;
	size_t size_; // максимальный размер, если бесконечный, то равен 0

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
        return new AppendGenerator<T>(const_cast<GeneratorDecorator<T>*>(this), item);
    }
    Generator<T>* Append(const Sequence<T>* items) const override {
        return new AppendGenerator<T>(const_cast<GeneratorDecorator<T>*>(this), items);
    }
    Generator<T>* Insert(const T& item, size_t index) const override {
        return new InsertGenerator<T>(const_cast<GeneratorDecorator<T>*>(this), item, index);
    }
    Generator<T>* Insert(const Sequence<T>* items, size_t index) const override {
        return new InsertGenerator<T>(const_cast<GeneratorDecorator<T>*>(this), items, index);
    }
    Generator<T>* Remove(size_t index) const override {
        return new RemoveGenerator<T>(const_cast<GeneratorDecorator<T>*>(this), index);
    }
    Generator<T>* Remove(size_t index, size_t count) const override {
        return new RemoveGenerator<T>(const_cast<GeneratorDecorator<T>*>(this), index, count);
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
		
	T GetNext() override {
		if (start_ <= pos_ && pos_ < start_ + insert_items_->GetLength()) {
			return insert_items_->Get((pos_++) - start_);
		} else {
			if (this->base_->HasNext()) {
				++pos_;
				return this->base_->GetNext();
			} else {
				throw RunTimeError("End of collection");
			}
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