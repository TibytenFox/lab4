#include <iostream>
#include <string>

#include "sequence/Exceptions.hpp"
#include "sequence/MutableArraySequence.hpp"
#include "src/LazySequence.hpp"
#include "src/PriorityQueue.hpp"
#include "src/Stream.hpp"
#include "src/Sort.hpp"

#define TEST(name) void name(); \
    struct Register_##name { Register_##name() { register_test(name); std::cout << #name << '\n'; } } reg_##name; \
    void name()

static int tests_passed = 0;
static int tests_failed = 0;

void register_test(void (*f)()) {
    try {
        f();
        ++tests_passed;
        std::cout << "[PASS]\t";
    } catch (const Exception &e) {
        ++tests_failed;
        std::cout << "[FAIL] " << e.GetMessage() << "\t";
    } catch (...) {
        ++tests_failed;
        std::cout << "[FAIL] Unknown exception\t";
    }
}

#define ASSERT_EQ(x, y) \
    if ((x) != (y)) throw RunTimeError("ASSERT_EQ failed: " #x " != " #y)

#define ASSERT_THROWS(expr, exc) \
    do { \
        bool caught = false; \
        try { expr; } \
        catch (const exc&) { caught = true; } \
        catch (...) {} \
        if (!caught) throw RunTimeError("ASSERT_THROWS failed: " #expr); \
    } while(0)

#define ASSERT_NO_THROW(expr) \
    do { \
        try { expr; } \
        catch (...) { throw RunTimeError("ASSERT_NO_THROW failed: " #expr); } \
    } while(0)

// ======================================================================
// ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ
// ======================================================================

int fib_rule(const Sequence<int>* seq) {
    size_t len = seq->GetLength();
    if (len < 2) return len;
    return seq->Get(len - 1) + seq->Get(len - 2);
}

int mult_by_2(int x) { return x * 2; }
bool is_even(int x) { return x % 2 == 0; }
int sum_reduce(int a, int b) { return a + b; }

// ======================================================================
// 1. ТЕСТЫ БИНАРНОЙ КУЧИ (PriorityQueue)
// ======================================================================

TEST(PQ_EmptyQueueBehavior) {
    PriorityQueue<int> pq;
    ASSERT_EQ(pq.IsEmpty(), true);
    ASSERT_THROWS(pq.GetMin(), EmptyCollectionError);
    ASSERT_THROWS(pq.ExtractMin(), EmptyCollectionError);
}

TEST(PQ_InsertAndExtract_SortedAndReverse) {
    PriorityQueue<int> pq1;
    // Худший случай для Min-Heap (добавление по убыванию)
    pq1.Insert(100); pq1.Insert(50); pq1.Insert(10); pq1.Insert(1);
    ASSERT_EQ(pq1.ExtractMin(), 1);
    ASSERT_EQ(pq1.ExtractMin(), 10);
    
    PriorityQueue<int> pq2;
    // Лучший случай (добавление по возрастанию)
    pq2.Insert(1); pq2.Insert(10); pq2.Insert(50); pq2.Insert(100);
    ASSERT_EQ(pq2.ExtractMin(), 1);
    ASSERT_EQ(pq2.ExtractMin(), 10);
}

TEST(PQ_DuplicatesHandling) {
    PriorityQueue<int> pq;
    pq.Insert(5); pq.Insert(5); pq.Insert(1); pq.Insert(5);
    ASSERT_EQ(pq.ExtractMin(), 1);
    ASSERT_EQ(pq.ExtractMin(), 5);
    ASSERT_EQ(pq.ExtractMin(), 5);
    ASSERT_EQ(pq.ExtractMin(), 5);
    ASSERT_EQ(pq.IsEmpty(), true);
}

TEST(PQ_DynamicResizeStress) {
    PriorityQueue<int> pq;
    // Вставляем 1000 элементов в обратном порядке для стресс-теста метода Resize и SiftUp
    for (int i = 1000; i >= 1; --i) {
        pq.Insert(i);
    }
    ASSERT_EQ(pq.IsEmpty(), false);
    // Проверяем, что первые 10 элементов корректно отсортированы
    for (int i = 1; i <= 10; ++i) {
        ASSERT_EQ(pq.ExtractMin(), i);
    }
}

// ======================================================================
// 2. ТЕСТЫ КОНЕЧНЫХ ЛЕНИВЫХ ПОСЛЕДОВАТЕЛЬНОСТЕЙ
// ======================================================================

TEST(LazySeq_Finite_BoundsAndErrors) {
    LazySequence<int> emptySeq;
    ASSERT_EQ(emptySeq.GetLength(), 0);
    ASSERT_EQ(emptySeq.GetCardinalLength().IsFinite(), true);
    
    ASSERT_THROWS(emptySeq.GetFirst(), IndexOutOfRange);
    ASSERT_THROWS(emptySeq.GetLast(), IndexOutOfRange);
    ASSERT_THROWS(emptySeq.Get(0), IndexOutOfRange);
    
    int arr[] = {1};
    LazySequence<int> singleSeq(arr, 1);
    ASSERT_EQ(singleSeq.GetFirst(), 1);
    ASSERT_EQ(singleSeq.GetLast(), 1);
    ASSERT_THROWS(singleSeq.Get(-1), IndexOutOfRange); // size_t переполнится
    ASSERT_THROWS(singleSeq.Get(1), IndexOutOfRange);
}

TEST(LazySeq_Finite_BasicMutations) {
    int arr[] = {10, 20, 30};
    LazySequence<int> seq(arr, 3);
    
    auto* s1 = seq.Append(40);
    ASSERT_EQ(s1->GetLength(), 4);
    ASSERT_EQ(s1->GetLast(), 40);
    
    auto* s2 = s1->Insert(5, 0);   // {5, 10, 20, 30, 40}
    ASSERT_EQ(s2->GetFirst(), 5);
    
    auto* s3 = s2->Insert(99, 5);  // {5, 10, 20, 30, 40, 99}
    ASSERT_EQ(s3->GetLast(), 99);
    
    ASSERT_THROWS(s3->Insert(0, 10), IndexOutOfRange);

    // Тест Remove (один и несколько)
    auto* s4 = s3->Remove(0);      // {10, 20, 30, 40, 99}
    ASSERT_EQ(s4->GetFirst(), 10);
    
    auto* s5 = s4->Remove(1, 2);   // удаляем 20 и 30 -> {10, 40, 99}
    ASSERT_EQ(s5->Get(1), 40);
    ASSERT_EQ(s5->GetLength(), 3);

    delete s1; delete s2; delete s3; delete s4; delete s5;
}

TEST(LazySeq_Finite_AlgebraChaining) {
    int arr[] = {1, 2, 3, 4, 5};
    LazySequence<int> seq(arr, 5);

    // Цепочка: Map(x*2) -> Append(100)
    auto* m1 = seq.Map(mult_by_2);        // {2, 4, 6, 8, 10}
    auto* m3 = m1->Append(100);           // {2, 4, 6, 8, 10, 100}
    
    ASSERT_EQ(m3->Get(0), 2);
    ASSERT_EQ(m3->Get(5), 100);
    ASSERT_EQ(m3->GetCardinalLength().IsFinite(), true);
    ASSERT_EQ(m3->GetCardinalLength().value, 6);

    int sum = m3->Reduce(sum_reduce, 0);  // 2+4+6+8+10+100 = 130
    ASSERT_EQ(sum, 130);

    delete m1; delete m3;
}

TEST(LazySeq_Finite_Zip) {
    int arr1[] = {1, 2, 3};
    LazySequence<int> seq1(arr1, 3);
    
    int arr2[] = {10, 20, 30, 40};
    LazySequence<int> seq2(arr2, 4);

    auto* zipped = seq1.Zip(&seq2);
    ASSERT_EQ(zipped->GetCardinalLength().IsFinite(), true);
    ASSERT_EQ(zipped->GetCardinalLength().value, 3);
    
    auto p0 = zipped->Get(0);
    ASSERT_EQ(p0.first, 1);
    ASSERT_EQ(p0.second, 10);
    
    auto p2 = zipped->Get(2);
    ASSERT_EQ(p2.first, 3);
    ASSERT_EQ(p2.second, 30);

    ASSERT_THROWS(zipped->Get(3), IndexOutOfRange);

    delete zipped;
}

TEST(LazySeq_Finite_Concat) {
	int arr1[] = {1, 2, 3};
    int arr2[] = {4, 5};

    LazySequence<int> seq1(arr1, 3);
    LazySequence<int> seq2(arr2, 2);

    LazySequence<int>* connected = seq1.Concat(&seq2);
	ASSERT_EQ(connected->GetCardinalLength().value, 5);
	ASSERT_EQ(connected->Get(2), 3);
	ASSERT_EQ(connected->Get(3), 4);
	ASSERT_EQ(connected->Get(4), 5);
}

// ======================================================================
// 3. ТЕСТЫ БЕСКОНЕЧНЫХ ЛЕНИВЫХ ПОСЛЕДОВАТЕЛЬНОСТЕЙ
// ======================================================================

TEST(LazySeq_Infinite_GenerationAndBounds) {
    MutableArraySequence<int> seed;
    seed.Append(0); seed.Append(1);
    LazySequence<int> fib(fib_rule, &seed);

    ASSERT_EQ(fib.GetCardinalLength().IsInfinite(), true);
    
    ASSERT_EQ(fib.Get(10), 55); // 0, 1, 1, 2, 3, 5, 8, 13, 21, 34, 55
    
    // Запросы, требующие материализации до конца, должны блокироваться
    ASSERT_THROWS(fib.GetLast(), RunTimeError);
    ASSERT_THROWS(fib.Reduce(sum_reduce, 0), RunTimeError);
}

TEST(LazySeq_Infinite_Mutations) {
    MutableArraySequence<int> seed;
    seed.Append(0); seed.Append(1);
    LazySequence<int> fib(fib_rule, &seed);

    auto* mod1 = fib.Insert(-99, 0);  // -99, 0, 1, 1, 2, 3...
    ASSERT_EQ(mod1->GetFirst(), -99);
    ASSERT_EQ(mod1->Get(1), 0);
    ASSERT_EQ(mod1->Get(6), 5); // Смещение на 1
    ASSERT_EQ(mod1->GetCardinalLength().IsInfinite(), true); 

    auto* mod2 = mod1->Remove(0, 2); // Удаляем -99 и 0
    // 1, 1, 2, 3, 5...
    ASSERT_EQ(mod2->GetFirst(), 1);
    ASSERT_EQ(mod2->Get(2), 2);
    
    delete mod1; delete mod2;
}

TEST(LazySeq_Infinite_Zip_WithFinite) {
    MutableArraySequence<int> seed;
    seed.Append(0); seed.Append(1);
    LazySequence<int> infiniteFib(fib_rule, &seed);

    int arr[] = {100, 200};
    LazySequence<int> finiteSeq(arr, 2);

    auto* zipped = infiniteFib.Zip(&finiteSeq);
    
    // Бесконечная x Конечная = Конечная 
    ASSERT_EQ(zipped->GetCardinalLength().IsFinite(), true);
    ASSERT_EQ(zipped->GetCardinalLength().value, 2);
    
    auto p1 = zipped->Get(1);
    ASSERT_EQ(p1.first, 1);
    ASSERT_EQ(p1.second, 200);

    ASSERT_THROWS(zipped->Get(2), IndexOutOfRange);

    delete zipped;
}

// ======================================================================
// 4. ТЕСТЫ ПОТОКОВ (Streams)
// ======================================================================

TEST(Stream_Basic_Sequence_ReadWrite) {
    MutableArraySequence<int> src;
    src.Append(10); src.Append(20); src.Append(30);

    ReadOnlyStream<int> reader(&src);
    reader.Open();
    
    ASSERT_EQ(reader.IsEndOfStream(), false);
    ASSERT_EQ(reader.GetPosition(), 0);
    ASSERT_EQ(reader.Read(), 10);
    ASSERT_EQ(reader.GetPosition(), 1);
    
    // Пишем в другой массив
    MutableArraySequence<int> dest;
    WriteOnlyStream<int> writer(&dest);
    writer.Open();
    
    writer.Write(reader.Read()); // Читаем 20, пишем 20
    writer.Write(reader.Read()); // Читаем 30, пишем 30
    
    ASSERT_EQ(reader.IsEndOfStream(), true);
    ASSERT_THROWS(reader.Read(), EndOfStream); // Попытка чтения за концом
    
    ASSERT_EQ(dest.GetLength(), 2);
    ASSERT_EQ(dest.Get(0), 20);
    ASSERT_EQ(dest.Get(1), 30);
    
    reader.Close();
    writer.Close();
}

TEST(Stream_SeekAndRewind) {
    MutableArraySequence<int> src;
    for (int i = 0; i < 10; ++i) src.Append(i * 10); // 0, 10, 20, ..., 90

    ReadOnlyStream<int> reader(&src);
    reader.Open();

    reader.Seek(5); // Перемотка вперед
    ASSERT_EQ(reader.Read(), 50);
    ASSERT_EQ(reader.GetPosition(), 6);

    reader.Seek(2); // Перемотка назад 
    ASSERT_EQ(reader.Read(), 20);

    reader.Close();
}

// ======================================================================
// 5. ТЕСТ АЛГОРИТМА СОРТИРОВКИ ПОТОКА
// ======================================================================

TEST(Sort_Stream_Normal) {
    MutableArraySequence<int> in_seq;
    in_seq.Append(50); in_seq.Append(10); in_seq.Append(30); in_seq.Append(20);
    ReadOnlyStream<int> in_stream(&in_seq);
    
    MutableArraySequence<int> out_seq;
    WriteOnlyStream<int> out_stream(&out_seq);

    in_stream.Open(); out_stream.Open();
    ASSERT_NO_THROW(SortStreamWithPriorityQueue(in_stream, out_stream));
    in_stream.Close(); out_stream.Close();

    ASSERT_EQ(out_seq.GetLength(), 4);
    ASSERT_EQ(out_seq.Get(0), 10);
    ASSERT_EQ(out_seq.Get(3), 50);
}

TEST(Sort_Stream_Empty) {
    MutableArraySequence<int> in_seq; // Пустой вход
    ReadOnlyStream<int> in_stream(&in_seq);
    
    MutableArraySequence<int> out_seq;
    WriteOnlyStream<int> out_stream(&out_seq);

    in_stream.Open(); out_stream.Open();
    ASSERT_NO_THROW(SortStreamWithPriorityQueue(in_stream, out_stream));
    in_stream.Close(); out_stream.Close();

    ASSERT_EQ(out_seq.GetLength(), 0);
}

TEST(Sort_Stream_WithDuplicates) {
    MutableArraySequence<int> in_seq;
    in_seq.Append(5); in_seq.Append(1); in_seq.Append(5); in_seq.Append(1);
    ReadOnlyStream<int> in_stream(&in_seq);
    
    MutableArraySequence<int> out_seq;
    WriteOnlyStream<int> out_stream(&out_seq);

    in_stream.Open(); out_stream.Open();
    SortStreamWithPriorityQueue(in_stream, out_stream);
    in_stream.Close(); out_stream.Close();

    ASSERT_EQ(out_seq.GetLength(), 4);
    ASSERT_EQ(out_seq.Get(0), 1);
    ASSERT_EQ(out_seq.Get(1), 1);
    ASSERT_EQ(out_seq.Get(2), 5);
    ASSERT_EQ(out_seq.Get(3), 5);
}

// ======================================================================
// MAIN 
// ======================================================================

int main() {    
    std::cout << "\n--------------------------------------------------\n";
    std::cout << " ИТОГОВАЯ СТАТИСТИКА:\n";
    std::cout << "--------------------------------------------------\n";
    std::cout << " Пройдено: " << tests_passed << "\n";
    std::cout << " Провалено: " << tests_failed << "\n";

    if (tests_failed == 0) {
        std::cout << "\n >>> [ УСПЕХ ] <<<\n\n";
        return 0;
    } else {
        std::cout << "\n >>> [ ОШИБКА ] <<<\n\n";
        return 1;
    }
}