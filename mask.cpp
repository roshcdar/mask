#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>

std::mutex mtx;
std::vector<std::tuple<int, int, std::string>> results;

bool matches_mask(const std::string& text, const std::string& mask) {
    if (text.size() != mask.size()) return false;
    for (size_t i = 0; i < mask.size(); ++i) {
        if (mask[i] != '?' && mask[i] != text[i]) {
            return false;
        }
    }
    return true;
}

void search_in_chunk(const std::vector<std::string>& lines, const std::string& mask, int start, int end) {
    for (int i = start; i < end; ++i) {
        const std::string& line = lines[i];
        for (size_t j = 0; j <= line.size() - mask.size(); ++j) {
            std::string substring = line.substr(j, mask.size());
            if (matches_mask(substring, mask)) {
                std::lock_guard<std::mutex> lock(mtx);
                results.emplace_back(i + 1, j + 1, substring);
                j += mask.size() - 1; // Skip overlapping matches
            }
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <file_name> <mask>\n";
        return 1;
    }

    std::string file_name = argv[1];
    std::string mask = argv[2];

    std::ifstream file(file_name);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << file_name << "\n";
        return 1;
    }

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(file, line)) {
        lines.push_back(line);
    }
    file.close();

    int num_threads = std::thread::hardware_concurrency();
    if (num_threads == 0) 
     {
        num_threads = 4;
     }

    int chunk_size = lines.size() / num_threads;
    std::vector<std::thread> threads;

    for (int i = 0; i < num_threads; ++i) {
        int start = i * chunk_size;
        int end = (i == num_threads - 1) ? lines.size() : start + chunk_size;
        threads.emplace_back(search_in_chunk, std::ref(lines), std::ref(mask), start, end);
    }

    for (auto& t : threads) {
        t.join();
    }

    std::cout << results.size() << std::endl;
    for (const auto& result : results) {
        int line_num, pos;
        std::string match;
        std::tie(line_num, pos, match) = result;
        std::cout << line_num << " " << pos << " " << match << std::endl;
    }

    return 0;
}