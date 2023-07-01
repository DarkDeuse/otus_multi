// Read files and prints top k word by frequency

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <map>
#include <vector>
#include <chrono>
#include <thread>
#include <mutex>

const size_t TOPK = 3;

using Counter = std::map<std::string, std::size_t>;

std::string tolowers(const std::string &str);

void count_words(std::istream& stream, Counter& counter);

std::mutex mutex;

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: topk_words [FILES...]\n";
        return EXIT_FAILURE;
    }
    auto start = std::chrono::high_resolution_clock::now();
    Counter freq_dict;
    std::vector<std::thread> threads;
    std::vector<std::ifstream> streams;
    for (int i = 1; i < argc; ++i) {
        std::ifstream input{argv[i]};
        std::cout<<"file in work: "<< argv[i]<< std::endl;
        if (!input.is_open()) {
            std::cerr << "Failed to open file " << argv[i] << '\n';
            return EXIT_FAILURE;
        }
        streams.push_back(std::move(input));
    };
    for (auto& input: streams){
        std::thread count {count_words, std::ref(input), std::ref(freq_dict)};
        threads.push_back(std::move(count));
        }
    for(auto& thread: threads){
        thread.join();
    };
    std::vector<Counter::const_iterator> words;
    words.reserve(freq_dict.size());
    for (auto it = std::cbegin(freq_dict); it != std::cend(freq_dict); ++it) {
        words.push_back(it);
    };
    std::partial_sort(
        std::begin(words), std::begin(words) + TOPK, std::end(words),
        [](auto lhs, auto &rhs) { return lhs->second > rhs->second; });
    std::for_each(
        std::begin(words), std::begin(words) + TOPK,
        [](const Counter::const_iterator &pair) {
            std::cout << std::setw(4) << pair->second << " " << pair->first
                      << '\n';
        
        });
    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "Elapsed time is " << elapsed_ms.count() << " ms\n";
}

std::string tolowers(const std::string &str) {
    std::string lower_str;
    std::transform(std::cbegin(str), std::cend(str),
                   std::back_inserter(lower_str),
                   [](unsigned char ch) { return (char)std::tolower(ch); }); // по какой-то причине в готовом файле здесь возникает ошибка
    return lower_str;
};

void count_words(std::istream& stream, Counter& counter) {
    std::for_each(std::istream_iterator<std::string>(stream),
                  std::istream_iterator<std::string>(),
                  [&counter](const std::string &s) { std::lock_guard<std::mutex> lck{mutex};
                                                    ++counter[tolowers(s)];});
}
