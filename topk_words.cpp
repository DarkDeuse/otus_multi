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
#include <condition_variable>

const size_t TOPK = 3;

using Counter = std::map<std::string, std::size_t>;

std::string tolowers(const std::string &str);

void count_words(std::istream& stream, Counter&, int readed_files);

void print_topk(std::ostream& stream, const Counter&, const size_t k);

void check_read(int count_reded_files,int all_given_files);

std::mutex mutex;

std::condition_variable read_files;

bool notificated{false};

std::condition_variable read_finished;

bool all_readed{false};


int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: topk_words [FILES...]\n";
        return EXIT_FAILURE;
    }
    auto start = std::chrono::high_resolution_clock::now();
    Counter freq_dict;
    std::vector<std::thread> threads;
    int readed_files = 0;
    int given_files = argc;
    for (int i = 1; i < argc; ++i) {
        std::ifstream input{argv[i]};
        std::cout<<"file in work: "<< argv[i]<< std::endl;
        if (!input.is_open()) {
            std::cerr << "Failed to open file " << argv[i] << '\n';
            return EXIT_FAILURE;
        }
        
        std::thread count {count_words, std::ref(input), std::ref(freq_dict), readed_files};
        std::cout << "Count " << count.get_id() << " created"
              << std::endl;
        threads.push_back(std::move(count));
    };
    std::thread check_read_files{check_read, readed_files, given_files};
    std::cout << "Count " << check_read_files.get_id() << " created"<< std::endl;
    std::thread print_words {print_topk, std::ref(std::cout), std::ref(freq_dict), TOPK};
    std::cout << "Count " << print_words.get_id() << " created"<< std::endl;
    threads.push_back(std::move(check_read_files));
    threads.push_back(std::move(print_words));
    std::cout << threads.size()<< std::endl;
    for (auto& count_func : threads){
        const auto clerk_id = count_func.get_id();
        count_func.join();
        std::cout << "Count " << clerk_id << " ended"
              << std::endl;
    };

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

void count_words(std::istream& stream, Counter& counter, int readed_files) {
    std::cout << "count_words started work"
              << std::endl;
    std::for_each(std::istream_iterator<std::string>(stream),
                  std::istream_iterator<std::string>(),
                  [&counter](const std::string &s) {std::unique_lock<std::mutex> lck{mutex}; ++counter[tolowers(s)];});
    ++readed_files;
    read_files.notify_all(); 
    notificated = true;
    std::cout << "count_words ended work"
              << std::endl;
}

void print_topk(std::ostream& stream, const Counter& counter, const size_t k) {
    std::unique_lock<std::mutex> lck{mutex};
    read_finished.wait(lck);
    std::vector<Counter::const_iterator> words;
    words.reserve(counter.size());
    for (auto it = std::cbegin(counter); it != std::cend(counter); ++it) {
        words.push_back(it);
    }
    std::partial_sort(
        std::begin(words), std::begin(words) + k, std::end(words),
        [](auto lhs, auto &rhs) { return lhs->second > rhs->second; });
    std::for_each(
        std::begin(words), std::begin(words) + k,
        [&stream](const Counter::const_iterator &pair) {
            stream << std::setw(4) << pair->second << " " << pair->first
                      << '\n';
        
        });
}

void check_read(int count_reded_files, int all_given_files){
    std::unique_lock<std::mutex> lck{mutex};
    read_files.wait(lck);
        if (count_reded_files == all_given_files){
        read_finished.notify_all();
        all_readed = true;}
    else{
        notificated = false;
    }

}

