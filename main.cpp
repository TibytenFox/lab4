#include <iostream>
#include <string>
#include <limits>

#include "src/LazySequence.hpp"
#include "src/Stream.hpp"
#include "src/PriorityQueue.hpp"
#include "src/Sort.hpp"
#include "sequence/MutableArraySequence.hpp"

// Безопасный ввод целого числа
int ReadInt(const std::string& prompt) {
    int val;
    std::cout << prompt;
    while (!(std::cin >> val)) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "Ошибка ввода. Пожалуйста, введите целое число: ";
    }
    return val;
}

// Функции для Map, Where, Reduce
int multiply_by_two(int x) { return x * 2; }
bool is_even(int x) { return x % 2 == 0; }
int sum_reduce(int a, int b) { return a + b; }

// Правило для бесконечной последовательности Фибоначчи
int fib_rule(const Sequence<int>* seq) {
    size_t len = seq->GetLength();
    if (len < 2) return len;
    return seq->Get(len - 1) + seq->Get(len - 2);
}

// Вывод ленивой последовательности
void PrintLazySequence(const LazySequence<int>* seq, size_t count) {
    std::cout << "Элементы последовательности: [ ";
    for (size_t i = 0; i < count; ++i) {
        try {
            std::cout << seq->Get(i) << " ";
        } catch (const IndexOutOfRange&) {
            std::cout << "(конец) ";
            break;
        } catch (const std::exception& e) {
            std::cout << "[Ошибка: " << e.what() << "] ";
            break;
        }
    }
    std::cout << "]" << std::endl;
}

void LazySequenceMenu() {
    LazySequence<int>* current_seq = new LazySequence<int>();
    int choice;

    do {
        std::cout << "\n--- Меню LazySequence (Ленивая последовательность) ---" << std::endl;
        std::cout << "1. Создать новую пустую последовательность" << std::endl;
        std::cout << "2. Создать бесконечную последовательность (Фибоначчи)" << std::endl;
        std::cout << "3. Добавить элемент в конец (Append)" << std::endl;
        std::cout << "4. Вставить элемент по индексу (Insert)" << std::endl;
        std::cout << "5. Удалить элемент по индексу (Remove)" << std::endl;
        std::cout << "6. Применить Map (x * 2)" << std::endl;
        std::cout << "7. Применить Where (только четные)" << std::endl;
        std::cout << "8. Применить Reduce (Сумма элементов)" << std::endl;
        std::cout << "9. Вывести первые N элементов" << std::endl;
        std::cout << "0. Назад" << std::endl;
        
        choice = ReadInt("Ваш выбор: ");

        switch (choice) {
            case 1: {
                delete current_seq;
                current_seq = new LazySequence<int>();
                std::cout << "Создана пустая последовательность." << std::endl;
                break;
            }
            case 2: {
                delete current_seq;
                MutableArraySequence<int> seed;
                seed.Append(0);
                seed.Append(1);
                current_seq = new LazySequence<int>(fib_rule, &seed);
                std::cout << "Создана бесконечная последовательность Фибоначчи." << std::endl;
                break;
            }
            case 3: {
                int val = ReadInt("Введите число для добавления: ");
                LazySequence<int>* new_seq = current_seq->Append(val);
                delete current_seq;
                current_seq = new_seq;
                std::cout << "Элемент лениво добавлен." << std::endl;
                break;
            }
            case 4: {
                int index = ReadInt("Введите индекс для вставки: ");
                int val = ReadInt("Введите число: ");
                try {
                    LazySequence<int>* new_seq = current_seq->Insert(val, index);
                    delete current_seq;
                    current_seq = new_seq;
                    std::cout << "Элемент лениво вставлен." << std::endl;
                } catch (const Exception& e) { std::cout << "Ошибка: " << e.GetMessage() << std::endl; }
                break;
            }
            case 5: {
                int index = ReadInt("Введите индекс для удаления: ");
                try {
                    LazySequence<int>* new_seq = current_seq->Remove(index);
                    delete current_seq;
                    current_seq = new_seq;
                    std::cout << "Элемент лениво удален." << std::endl;
                } catch (const Exception& e) { std::cout << "Ошибка: " << e.GetMessage() << std::endl; }
                break;
            }
            case 6: {
                LazySequence<int>* new_seq = current_seq->Map(multiply_by_two);
                delete current_seq;
                current_seq = new_seq;
                std::cout << "Применен Map (* 2)." << std::endl;
                break;
            }
            case 7: {
                LazySequence<int>* new_seq = current_seq->Where(is_even);
                delete current_seq;
                current_seq = new_seq;
                std::cout << "Применен Where (четные)." << std::endl;
                break;
            }
            case 8: {
                try {
                    int res = current_seq->Reduce(sum_reduce, 0);
                    std::cout << "Результат Reduce (Сумма): " << res << std::endl;
                } catch (const Exception& e) { std::cout << "Ошибка: " << e.GetMessage() << std::endl; }
                break;
            }
            case 9: {
                int count = ReadInt("Сколько элементов вывести? ");
                PrintLazySequence(current_seq, count);
                break;
            }
            case 0: break;
            default: std::cout << "Неверный пункт меню." << std::endl;
        }
    } while (choice != 0);

    delete current_seq;
}

void PriorityQueueMenu() {
    PriorityQueue<int> pq;
    int choice;

    do {
        std::cout << "\n--- Меню PriorityQueue (Очередь с приоритетами / Min-Heap) ---" << std::endl;
        std::cout << "1. Добавить элемент (Insert)" << std::endl;
        std::cout << "2. Извлечь минимум (Extract Min)" << std::endl;
        std::cout << "3. Посмотреть минимум (Get Min)" << std::endl;
        std::cout << "4. Проверить на пустоту" << std::endl;
        std::cout << "0. Назад" << std::endl;
        
        choice = ReadInt("Ваш выбор: ");

        switch (choice) {
            case 1: {
                int val = ReadInt("Введите число: ");
                pq.Insert(val);
                std::cout << "Элемент добавлен." << std::endl;
                break;
            }
            case 2: {
                try {
                    int min_val = pq.ExtractMin();
                    std::cout << "Извлечен минимальный элемент: " << min_val << std::endl;
                } catch (const Exception& e) { std::cout << "Ошибка: " << e.GetMessage() << std::endl; }
                break;
            }
            case 3: {
                try {
                    std::cout << "Текущий минимум: " << pq.GetMin() << std::endl;
                } catch (const Exception& e) { std::cout << "Ошибка: " << e.GetMessage() << std::endl; }
                break;
            }
            case 4: {
                std::cout << "Очередь пуста? " << (pq.IsEmpty() ? "Да" : "Нет") << std::endl;
                break;
            }
            case 0: break;
            default: std::cout << "Неверный пункт меню." << std::endl;
        }
    } while (choice != 0);
}

void StreamSortMenu() {
    int choice;
    do {
        std::cout << "\n--- Меню Сортировки Потоков ---" << std::endl;
        std::cout << "1. Ввести данные потока вручную и отсортировать" << std::endl;
        std::cout << "2. Вывести один элемент из выходного потока" << std::endl;
        std::cout << "0. Назад" << std::endl;

        choice = ReadInt("Ваш выбор: ");

        if (choice == 1) {
            int count = ReadInt("Сколько чисел вы хотите передать в поток? ");
            MutableArraySequence<int> input_seq;
            
            for (int i = 0; i < count; ++i) {
                int val = ReadInt("Элемент " + std::to_string(i + 1) + ": ");
                input_seq.Append(val);
            }

            ReadOnlyStream<int> in_stream(&input_seq);
            MutableArraySequence<int> output_seq;
            WriteOnlyStream<int> out_stream(&output_seq);

            in_stream.Open();
            out_stream.Open();

            std::cout << "\nСортировка потока с использованием PriorityQueue..." << std::endl;
            SortStreamWithPriorityQueue(in_stream, out_stream);

            in_stream.Close();
            out_stream.Close();

            std::cout << "Результат сортировки (чтение из выходного потока): [ ";
            for (size_t i = 0; i < output_seq.GetLength(); ++i) {
                std::cout << output_seq.Get(i) << " ";
            }
            std::cout << "]" << std::endl;
        }
    } while (choice != 0);
}

int main() {
    int main_choice;

    do {
        std::cout << "\n======== ГЛАВНОЕ МЕНЮ ========" << std::endl;
        std::cout << "1. Тестировать LazySequence " << std::endl;
        std::cout << "2. Тестировать PriorityQueue " << std::endl;
        std::cout << "3. Тестировать Streams + Сортировка " << std::endl;
        std::cout << "0. Выход" << std::endl;
        
        main_choice = ReadInt("Выберите подсистему для тестирования: ");

        switch (main_choice) {
            case 1: LazySequenceMenu(); break;
            case 2: PriorityQueueMenu(); break;
            case 3: StreamSortMenu(); break;
            case 0: std::cout << "Завершение программы..." << std::endl; break;
            default: std::cout << "Неверный пункт меню." << std::endl;
        }
    } while (main_choice != 0);

    return 0;
}