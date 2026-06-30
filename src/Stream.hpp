#pragma once

#include <stdio.h> // Для FILE*, fopen, fclose, feof
#include "sequence/Sequence.hpp"
#include "LazySequence.hpp"
#include "sequence/Exceptions.hpp"

class EndOfStream : public RunTimeError {
public:
    EndOfStream() : RunTimeError("End of stream reached") {}
};

template <class T>
class ReadOnlyStream {
private:
    enum SourceType { SEQUENCE, LAZY_SEQUENCE, FILE_SOURCE, STRING_SOURCE };
    SourceType type_;

    const Sequence<T>* seq_source_ = nullptr;
    const LazySequence<T>* lazy_source_ = nullptr;
    
    FILE* file_stream_ = nullptr;
    const char* filename_ = nullptr;
    
    const char* string_data_ = nullptr;
    size_t string_pos_ = 0;

    T (*file_deserializer_)(FILE*) = nullptr;
    T (*string_deserializer_)(const char*, size_t&) = nullptr;
    
    size_t position_ = 0;
    bool is_open_ = false;

public:
    explicit ReadOnlyStream(const Sequence<T>* seq) : type_(SEQUENCE), seq_source_(seq) {}
    
    explicit ReadOnlyStream(const LazySequence<T>* lazy_seq) : type_(LAZY_SEQUENCE), lazy_source_(lazy_seq) {}
    
    ReadOnlyStream(const char* filename, T (*deserializer)(FILE*)) 
        : type_(FILE_SOURCE), filename_(filename), file_deserializer_(deserializer) {}
        
    ReadOnlyStream(const char* data, T (*deserializer)(const char*, size_t&)) 
        : type_(STRING_SOURCE), string_data_(data), string_deserializer_(deserializer) {}

    ~ReadOnlyStream() { Close(); }

    void Open() {
        if (is_open_) return;
        if (type_ == FILE_SOURCE) {
            file_stream_ = fopen(filename_, "rb");
            if (!file_stream_) throw RunTimeError("Cannot open file");
        } else if (type_ == STRING_SOURCE) {
            string_pos_ = 0;
        }
        position_ = 0;
        is_open_ = true;
    }

    void Close() {
        if (!is_open_) return;
        if (type_ == FILE_SOURCE && file_stream_) {
            fclose(file_stream_);
            file_stream_ = nullptr;
        }
        is_open_ = false;
    }

    bool IsEndOfStream() const {
        if (!is_open_) return true;
        switch (type_) {
            case SEQUENCE:
                return position_ >= seq_source_->GetLength();
            case LAZY_SEQUENCE:
                if (lazy_source_->GetCardinalLength().IsFinite()) {
                    return position_ >= lazy_source_->GetCardinalLength().value;
                }
                return false; 
            case FILE_SOURCE:
                return feof(file_stream_) != 0;
            case STRING_SOURCE:
                return string_data_[string_pos_] == '\0';
        }
        return true;
    }

    T Read() {
        if (!is_open_) throw RunTimeError("Stream is not open");
        if (IsEndOfStream()) throw EndOfStream();

        T value;
        switch (type_) {
            case SEQUENCE:
                value = seq_source_->Get(position_);
                break;
            case LAZY_SEQUENCE:
                value = lazy_source_->Get(position_);
                break;
            case FILE_SOURCE:
                value = file_deserializer_(file_stream_);
                break;
            case STRING_SOURCE:
                value = string_deserializer_(string_data_, string_pos_);
                break;
        }
        position_++;
        return value;
    }

    size_t GetPosition() const { return position_; }
    bool IsCanSeek() const { return true; }
    bool IsCanGoBack() const { return true; }

    size_t Seek(size_t index) {
        if (!is_open_) throw RunTimeError("Stream is not open");
        
        if (type_ == SEQUENCE || type_ == LAZY_SEQUENCE) {
            position_ = index;
        } else {
            if (index < position_) {
                if (!IsCanGoBack()) throw RunTimeError("Cannot go back");
				// Сброс позиции
                Close();
                Open();
            }
            while (position_ < index && !IsEndOfStream()) {
                Read();
            }
        }
        return position_;
    }
};

template <class T>
class WriteOnlyStream {
private:
    enum DestType { SEQUENCE, FILE_DEST };
    DestType type_;

    Sequence<T>* seq_dest_ = nullptr;
    FILE* file_stream_ = nullptr;
    const char* filename_ = nullptr;
    void (*file_serializer_)(FILE*, const T&) = nullptr;
    
    size_t position_ = 0;
    bool is_open_ = false;

public:
    explicit WriteOnlyStream(Sequence<T>* seq) : type_(SEQUENCE), seq_dest_(seq) {}
    
    WriteOnlyStream(const char* filename, void (*serializer)(FILE*, const T&)) 
        : type_(FILE_DEST), filename_(filename), file_serializer_(serializer) {}

    ~WriteOnlyStream() { Close(); }

    void Open() {
        if (is_open_) return;
        if (type_ == FILE_DEST) {
            file_stream_ = fopen(filename_, "ab");
            if (!file_stream_) throw RunTimeError("Cannot open file for writing");
        }
        position_ = 0;
        is_open_ = true;
    }

    void Close() {
        if (!is_open_) return;
        if (type_ == FILE_DEST && file_stream_) {
            fclose(file_stream_);
            file_stream_ = nullptr;
        }
        is_open_ = false;
    }

    size_t GetPosition() const { return position_; }

    size_t Write(const T& item) {
        if (!is_open_) throw RunTimeError("Stream is not open");

        if (type_ == SEQUENCE) {
            auto mutable_seq = dynamic_cast<MutableArraySequence<T>*>(seq_dest_);
            if (mutable_seq) {
                mutable_seq->Append(item);
            } else {
                throw RunTimeError("Sequence destination is not mutable");
            }
        } else if (type_ == FILE_DEST) {
            file_serializer_(file_stream_, item);
        }

        position_++;
        return position_;
    }
};