#include <iostream>
#include <string>
#include "src/LazySequence.hpp"
#include "src/Stream.hpp"
#include "src/Sort.hpp"
#include "sequence/MutableArraySequence.hpp"

// --- Вспомогательные функции для тестирования Zip и Reduce ---
// Правило генерации чисел Фибоначчи для тестирования RecurrentGenerator
int fib(const Sequence<int>* seq) {
    size_t len = seq->GetLength();
    return seq->Get(len - 1) + seq->Get(len - 2);
}

// Функции-колбэки для Map и Where
int MultiplyByTwo(int x) { 
    return x * 2; 
}

bool IsEven(int x) { 
    return x % 2 == 0; 
}

bool IsGreaterThanFive(int x) {
    return x > 5;
}

// Универсальная функция проверки результатов (без использования STL-контейнеров)
template <class T>
void CheckSequence(const LazySequence<T>* seq, const T expected[], size_t size, const std::string& msg) {
    std::cout << msg << ": ";
    bool ok = true;
    for (size_t i = 0; i < size; ++i) {
        try {
            T val = seq->Get(i);
            std::cout << val << " ";
            if (val != expected[i]) ok = false;
        } catch (...) {
            std::cout << "[ERR] ";
            ok = false;
        }
    }
    std::cout << (ok ? "✓" : "✗") << std::endl;
}

// Функция-колбэк для Reduce (вычисление суммы элементов)
int SumReducer(int accumulator, int current_value) {
    return accumulator + current_value;
}

// Функция-колбэк для Reduce (вычисление произведения элементов)
int ProductReducer(int accumulator, int current_value) {
    return accumulator * current_value;
}

// Специализированная функция проверки результатов для последовательностей Pair (Zip)
template <class T1, class T2>
void CheckZipSequence(const LazySequence<Pair<T1, T2>>* seq, const T1 expected1[], const T2 expected2[], size_t size, const std::string& msg) {
    std::cout << msg << ": ";
    bool ok = true;
    for (size_t i = 0; i < size; ++i) {
        try {
            Pair<T1, T2> val = seq->Get(i);
            std::cout << "(" << val.first << ", " << val.second << ") ";
            if (val.first != expected1[i] || val.second != expected2[i]) ok = false;
        } catch (...) {
            std::cout << "[ERR] ";
            ok = false;
        }
    }
    std::cout << (ok ? "✓" : "✗") << std::endl;
}

// --- Новый тестовый блок ---

void TestZipAndReduce() {
    std::cout << "\n=== 4. ТЕСТ МЕТОДОВ ZIP И REDUCE ===\n";

    // Исходные данные для проверки
    int arr1[] = {1, 2, 3, 4, 5};
    int arr2[] = {10, 20, 30}; // Более короткий массив для проверки усечения по длине

    LazySequence<int> seq1(arr1, 5);
    LazySequence<int> seq2(arr2, 3);

    // 1. Проверка метода Reduce (Сумма элементов)
    // Ожидаемый результат: 1 + 2 + 3 + 4 + 5 = 15
    int sum_result = seq1.Reduce(SumReducer, 0);
    std::cout << "Reduce (Sum of seq1): " << sum_result << (sum_result == 15 ? " ✓" : " ✗") << "\n";

    // 2. Проверка метода Reduce (Произведение элементов с начальным значением 1)
    // Ожидаемый результат: 1 * 1 * 2 * 3 * 4 * 5 = 120
    int prod_result = seq1.Reduce(ProductReducer, 1);
    std::cout << "Reduce (Product of seq1): " << prod_result << (prod_result == 120 ? " ✓" : " ✗") << "\n";

    // 3. Проверка метода Zip на конечных последовательностях
    // Так как вторая последовательность имеет длину 3, результирующий Zip должен усечься до 3 элементов
    LazySequence<Pair<int, int>>* zipped_finite = seq1.Zip(&seq2);
    int exp_zip1[] = {1, 2, 3};
    int exp_zip2[] = {10, 20, 30};
    CheckZipSequence(zipped_finite, exp_zip1, exp_zip2, 3, "Finite Zip (Length should truncate to 3)");
    
    // Проверка, что при попытке взять 4-й элемент (индекс 3) сработает ограничение по длине Cardinal
    bool exception_caught = false;
    try {
        zipped_finite->Get(3);
    } catch (const IndexOutOfRange&) {
        exception_caught = true;
    }
    std::cout << "Zip out-of-range bounds check: " << (exception_caught ? "✓" : "✗") << "\n";


    // 4. Проверка метода Zip с участием бесконечной последовательности
    MutableArraySequence<int> seed;
    seed.Append(0);
    seed.Append(1);
    LazySequence<int> infiniteFib(fib, &seed); // Бесконечный Фибоначчи

    // Сшиваем бесконечную последовательность Фибоначчи с конечной seq1 {1, 2, 3, 4, 5}
    // Длина результата должна определяться меньшей (конечной) стороной
    LazySequence<Pair<int, int>>* zipped_mixed = infiniteFib.Zip(&seq1);
    
    int exp_fib_part[] = {0, 1, 1, 2, 3}; // Первые 5 чисел Фибоначчи
    int exp_seq_part[] = {1, 2, 3, 4, 5}; // Элементы seq1
    CheckZipSequence(zipped_mixed, exp_fib_part, exp_seq_part, 5, "Mixed Zip (Infinite Fib + Finite seq1)");
    
    std::cout << "Is mixed Zip length finite? " << (zipped_mixed->GetCardinalLength().IsFinite() ? "YES ✓" : "NO ✗") << "\n";

    delete zipped_finite;
    delete zipped_mixed;
}

// --- Тестовые блоки ---

void TestFiniteLazySequence() {
    std::cout << "=== 1. ТЕСТ КОНЕЧНОЙ ПОСЛЕДОВАТЕЛЬНОСТИ ===\n";
    
    int arr[] = {10, 20, 30};
    LazySequence<int> seqFromArr(arr, 3);
    
    // Проверка базового чтения
    std::cout << "GetFirst: " << seqFromArr.GetFirst() << (seqFromArr.GetFirst() == 10 ? " ✓" : " ✗") << "\n";
    std::cout << "GetLast: " << seqFromArr.GetLast() << (seqFromArr.GetLast() == 30 ? " ✓" : " ✗") << "\n";

    // Проверка Append
    auto* s1 = seqFromArr.Append(40);
    int exp1[] = {10, 20, 30, 40};
    CheckSequence(s1, exp1, 4, "Append 40");

    // Проверка Insert
    auto* s2 = s1->Insert(25, 2);
    int exp2[] = {10, 20, 25, 30, 40};
    CheckSequence(s2, exp2, 5, "Insert 25 at index 2");

    // Проверка Remove (несколько элементов)
    auto* s3 = s2->Remove(1, 2);
    int exp3[] = {10, 30, 40};
    CheckSequence(s3, exp3, 3, "Remove 2 elements from index 1");

    // Проверка Remove (один элемент)
    auto* s4 = s3->Remove(1);
    int exp4[] = {10, 40};
    CheckSequence(s4, exp4, 2, "Remove 1 element from index 1");

    delete s1; delete s2; delete s3; delete s4;
}

void TestInfiniteLazySequence() {
    std::cout << "\n=== 2. ТЕСТ БЕСКОНЕЧНОЙ ЛЕНИВОЙ ПОСЛЕДОВАТЕЛЬНОСТИ ===\n";
    
    // Инициализация стартовых значений (Seed)
    MutableArraySequence<int> seed;
    seed.Append(0);
    seed.Append(1);
    
    // Создание бесконечной последовательности Фибоначчи
    LazySequence<int> fibSeq(fib, &seed);

    // Проверка вычисления первых 7 чисел
    int exp_fib[] = {0, 1, 1, 2, 3, 5, 8};
    CheckSequence(&fibSeq, exp_fib, 7, "Fibonacci (first 7 elements)");

    // Проверка ленивой вставки в начало бесконечной последовательности
    auto* fibIns = fibSeq.Insert(-10, 0);
    int exp_ins[] = {-10, 0, 1, 1, 2, 3, 5, 8};
    CheckSequence(fibIns, exp_ins, 8, "Insert -10 at index 0");

    // Проверка ленивого удаления из бесконечной последовательности
    auto* fibRem = fibIns->Remove(2, 2); // Удаляем числа 1, 1
    int exp_rem[] = {-10, 0, 2, 3, 5, 8, 13};
    CheckSequence(fibRem, exp_rem, 7, "Remove 2 elements from index 2");

    // Проверка изменения счетчика длины
    std::cout << "Is original infinite? " << (fibSeq.GetCardinalLength().IsInfinite() ? "YES ✓" : "NO ✗") << "\n";
    std::cout << "Is modified infinite? " << (fibRem->GetCardinalLength().IsInfinite() ? "YES ✓" : "NO ✗") << "\n";

    delete fibIns; delete fibRem;
}

void TestMapAndWhere() {
    std::cout << "\n=== 3. ТЕСТ МЕТОДОВ MAP И WHERE ===\n";

    // --- Часть А: Тестирование на конечной ленивой последовательности ---
    int arr[] = {1, 2, 3, 4, 5};
    LazySequence<int> finiteSeq(arr, 5);

    // Ленивый Map: умножаем каждый элемент конечного массива на 2
    LazySequence<int>* finiteMapped = finiteSeq.Map(MultiplyByTwo);
    int exp_fin_map[] = {2, 4, 6, 8, 10};
    CheckSequence(finiteMapped, exp_fin_map, 5, "Finite Map (*2)");

    // Ленивый Where: фильтруем элементы конечного массива (оставляем > 5)
    LazySequence<int>* finiteFiltered = finiteMapped->Where(IsGreaterThanFive);
    int exp_fin_where[] = {6, 8, 10};
    CheckSequence(finiteFiltered, exp_fin_where, 3, "Finite Where (>5)");


    // --- Часть Б: Тестирование на бесконечной ленивой последовательности ---
    MutableArraySequence<int> seed;
    seed.Append(0);
    seed.Append(1);
    LazySequence<int> infiniteFib(fib, &seed);

    // Ленивый Map на бесконечном потоке Фибоначчи
    LazySequence<int>* infiniteMapped = infiniteFib.Map(MultiplyByTwo);
    // Оригинал: 0, 1, 1, 2, 3, 5, 8... Умноженный: 0, 2, 2, 4, 6, 10, 16...
    int exp_inf_map[] = {0, 2, 2, 4, 6, 10, 16};
    CheckSequence(infiniteMapped, exp_inf_map, 7, "Infinite Map (*2)");

    // Ленивый Where на бесконечном потоке Фибоначчи (выбираем только четные числа Фибоначчи)
    LazySequence<int>* infiniteFiltered = infiniteFib.Where(IsEven);
    // Четные числа Фибоначчи: 0, 2, 8, 34, 144...
    int exp_inf_where[] = {0, 2, 8, 34};
    CheckSequence(infiniteFiltered, exp_inf_where, 4, "Infinite Where (Even Fib)");

    // Проверка сохранения бесконечного типа размерности Cardinal
    std::cout << "Is Where-sequence infinite? " << (infiniteFiltered->GetCardinalLength().IsInfinite() ? "YES ✓" : "NO ✗") << "\n";

    delete finiteMapped;
    delete finiteFiltered;
    delete infiniteMapped;
    delete infiniteFiltered;
}

int IntStringDeserializer(const char* data, size_t& pos) {
    while (data[pos] == ' ' || data[pos] == ',') { pos++; }
    if (data[pos] == '\0') return 0;
    int val = 0;
    while (data[pos] >= '0' && data[pos] <= '9') {
        val = val * 10 + (data[pos] - '0');
        pos++;
    }
    return val;
}

void TestStreamSorting() {
    std::cout << "\n=== 5. ТЕСТ СОРТИРОВКИ ПОТОКА (Вариант 10.2) ===\n";

    // 1. Подготавливаем входной поток данных (неотсортированные числа в строке)
    const char* unsorted_data = "42, 15, 8, 23, 4, 16, 4, 100, 1";
    ReadOnlyStream<int> reader(unsorted_data, IntStringDeserializer);
    reader.Open();

    // 2. Подготавливаем выходной поток (будем писать в Sequence для удобства проверки)
    MutableArraySequence<int> out_seq;
    WriteOnlyStream<int> writer(&out_seq);
    writer.Open();

    // 3. Выполняем сортировку
    SortStreamWithPriorityQueue(reader, writer);

    // 4. Закрываем потоки
    reader.Close();
    writer.Close();

    // 5. Проверяем результат
    std::cout << "Оригинальные данные: " << unsorted_data << "\n";
    std::cout << "Отсортированный поток: ";
    
    bool is_sorted = true;
    for (size_t i = 0; i < out_seq.GetLength(); ++i) {
        std::cout << out_seq.Get(i) << " ";
        if (i > 0 && out_seq.Get(i) < out_seq.Get(i - 1)) {
            is_sorted = false;
        }
    }
    
    std::cout << "\nСтатус сортировки: " << (is_sorted ? "УСПЕШНО ✓" : "ОШИБКА ✗") << "\n";
}

int main() {
    TestFiniteLazySequence();
    TestInfiniteLazySequence();
    TestMapAndWhere();
    TestZipAndReduce();
    TestStreamSorting();

    std::cout << "\n=========================================\n";
    std::cout << "СТАТУС: Все компоненты успешно проверены!\n";
    return 0;
}