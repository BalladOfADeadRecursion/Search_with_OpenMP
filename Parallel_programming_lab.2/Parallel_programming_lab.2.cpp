#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <string>
#include <chrono>
#include <omp.h>

using namespace std;

// Структура для хранения результата поиска в файле
struct SearchResult {
    string filename;
    bool found;
    int count;
    double duration; // Время выполнения поиска в секундах
    int threadId;   // Номер потока
};

unordered_map<string, vector<string>> createHashTableForFile(const string& filename, const string& fragment) {
    unordered_map<string, vector<string>> fragmentMap;
    ifstream file(filename);
    if (file.is_open()) {
        string line;
        while (getline(file, line)) {
            size_t pos = line.find(fragment);
            while (pos != string::npos) {
                fragmentMap[line.substr(pos, fragment.size())].push_back(filename);
                pos = line.find(fragment, pos + 1);
            }
        }
        file.close();
    }
    else {
        cerr << "Не удалось открыть файл: " << filename << endl;
    }
    return fragmentMap;
}

SearchResult searchFragmentInHashTable(const unordered_map<string, vector<string>>& fragmentMap, const string& fragment, const string& filename) {
    SearchResult result;
    result.filename = filename;
    result.threadId = omp_get_thread_num(); // Получаем номер текущего потока
    auto start = chrono::high_resolution_clock::now();

    if (fragmentMap.find(fragment) != fragmentMap.end()) {
        const auto& foundFragments = fragmentMap.at(fragment);
        result.found = true;
        result.count = foundFragments.size();
    }
    else {
        result.found = false;
        result.count = 0;
    }

    auto end = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end - start);
    result.duration = duration.count() * 1000.0; // Преобразуем в секунды

    return result;
}

int main() {
    setlocale(LC_ALL, "Russian");
    vector<string> filenames = { "source1.txt", "source2.txt", "source3.txt", "source4.txt", "source5.txt",
                                  "source6.txt", "source7.txt", "source8.txt", "source9.txt", "source10.txt" };
    string fragment;

    // Ввод фрагмента для поиска
    cout << "Введите фрагмент для поиска: ";
    getline(cin, fragment);

    int numThreads;
    // Запрос у пользователя выбора количества потоков
    do {
        cout << "Введите желаемое количество потоков (от 2 до 10): ";
        cin >> numThreads;
    } while (numThreads < 2 || numThreads > 10);

    omp_set_num_threads(numThreads);

    // Создание хэш-таблиц для каждого файла с использованием OpenMP
    unordered_map<string, unordered_map<string, vector<string>>> allFragmentMaps;
#pragma omp parallel for shared(allFragmentMaps)
    for (int i = 0; i < filenames.size(); ++i) {
        const string& filename = filenames[i];
        auto fragmentMap = createHashTableForFile(filename, fragment);
#pragma omp critical
        {
            allFragmentMaps[filename] = move(fragmentMap);
            cout << "Поток " << omp_get_thread_num() << " создал хэш-таблицу для файла " << filename << endl;
        }
    }

    // Поиск фрагмента в файлах с использованием созданных хэш-таблиц
    vector<SearchResult> results(filenames.size());
#pragma omp parallel for
    for (int i = 0; i < filenames.size(); ++i) {
        const string& filename = filenames[i];
        const auto& fragmentMap = allFragmentMaps[filename];
        results[i] = searchFragmentInHashTable(fragmentMap, fragment, filename);
    }

    // Вывод результатов после завершения всех задач
    cout << "Результаты поиска:" << endl;
    for (const auto& result : results) {
        cout << "Поток " << result.threadId << ": ";
        cout << (result.found ? "Фрагмент найден" : "Фрагмент не найден") << " в файле " << result.filename;
        if (result.found) {
            cout << ". Количество соответствующих фрагментов: " << result.count;
        }
        cout << ". Время затраченное на поиск: " << result.duration << " секунд" << endl;
    }

    return 0;
}
