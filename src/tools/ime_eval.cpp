#include "../engine/pinyin_engine.h"
#include "../common/utf_utils.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {
std::string json_string(const std::string& value) {
    std::string result = "\"";
    for (char character : value) {
        switch (character) {
            case '\\': result += "\\\\"; break;
            case '"': result += "\\\""; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default: result += character;
        }
    }
    return result + "\"";
}
}

// UTF-8 TSV input: pinyin<TAB>expected text<TAB>source/case ID[<TAB>preceding text].
// The optional preceding text is the committed document text before the caret.
// Evaluation is separate from training and never learns the expected answers,
// except with --learn: it then simulates one person typing the cases in order,
// choosing the expected text each time (as committing it does), starting from
// an empty learning record.
int wmain(int argc, wchar_t** argv) {
    bool keystrokes = false, use_context = true, scores = false, learn = false, valid = argc >= 3;
    unsigned domains = 0;
    for (int index = 3; index < argc; ++index) {
        const std::wstring flag = argv[index];
        if (flag == L"--keystrokes") keystrokes = true;
        else if (flag == L"--no-context") use_context = false;
        else if (flag == L"--scores") scores = true;
        else if (flag == L"--learn") learn = true;
        else if (flag.starts_with(L"--weights=")) valid = valid && localpinyin::PinyinEngine::set_evaluation_weights(flag.substr(10));
        else if (flag.starts_with(L"--domains=")) domains = localpinyin::parse_usage_domains(flag.substr(10));
        else valid = false;
    }
    if (!valid) {
        std::cerr << "Usage: LocalPinyinImeEval <directory containing dictionary/> <cases.tsv> [--keystrokes] [--no-context] [--scores] [--learn] [--domains=coding,daily]\n";
        return 2;
    }
    localpinyin::PinyinEngine engine(localpinyin::DictionaryLoadMode::Empty);
    const auto isolated_user = std::filesystem::path(argv[2]).parent_path() / L"eval-user" / L"user_lexicon.tsv";
    if (learn) {
        std::error_code ignored;
        std::filesystem::remove_all(isolated_user.parent_path(), ignored);
    }
    if (!engine.load_dictionary_resource_directory(argv[1], isolated_user.wstring())) return 3;
    localpinyin::EngineInputOptions options;
    options.domains = domains;
    engine.set_input_options(options);
    std::ifstream cases(std::filesystem::path(argv[2]), std::ios::binary);
    if (!cases) return 4;
    std::string line;
    while (std::getline(cases, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#') continue;
        const auto first = line.find('\t');
        const auto second = line.find('\t', first == std::string::npos ? 0 : first + 1);
        if (first == std::string::npos || second == std::string::npos) return 5;
        const auto pinyin = line.substr(0, first);
        const auto expected = line.substr(first + 1, second - first - 1);
        const auto third = line.find('	', second + 1);
        const auto source = line.substr(second + 1, third == std::string::npos ? std::string::npos : third - second - 1);
        const auto preceding = third == std::string::npos || !use_context ? std::wstring() :
            localpinyin::utf8_to_wide(line.substr(third + 1));
        const auto wide_pinyin = localpinyin::utf8_to_wide(pinyin);
        std::vector<long long> typing_durations;
        if (keystrokes) {
            for (size_t end = 1; end < wide_pinyin.size(); ++end) {
                const auto started = std::chrono::steady_clock::now();
                const auto interim = engine.lookup(wide_pinyin.substr(0, end), preceding);
                if (interim.empty()) return 6;
                typing_durations.push_back(std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::steady_clock::now()-started).count());
            }
        }
        const auto begin = std::chrono::steady_clock::now();
        const auto candidates = engine.lookup(wide_pinyin, preceding);
        const auto micros = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - begin).count();
        int rank = 0;
        std::cout << "{\"pinyin\":" << json_string(pinyin)
                  << ",\"expected\":" << json_string(expected)
                  << ",\"source\":" << json_string(source)
                  << ",\"lookup_us\":" << micros << ",\"candidates\":[";
        for (size_t index = 0; index < candidates.size(); ++index) {
            const auto text = localpinyin::wide_to_utf8(candidates[index].text);
            if (index) std::cout << ',';
            std::cout << json_string(text);
            if (text == expected) rank = static_cast<int>(index + 1);
        }
        std::cout << "],\"expected_rank\":" << rank;
        if (scores) {
            std::cout << ",\"scores\":[";
            for (size_t index = 0; index < candidates.size(); ++index)
                std::cout << (index ? "," : "") << candidates[index].total_score();
            // Typed letters each candidate reads (0: the whole input).
            std::cout << "],\"consumed\":[";
            for (size_t index = 0; index < candidates.size(); ++index)
                std::cout << (index ? "," : "") << candidates[index].consumed_pinyin;
            std::cout << ']';
        }
        if (keystrokes) {
            typing_durations.push_back(micros);
            std::cout << ",\"typing_lookup_us\":[";
            for (size_t index = 0; index < typing_durations.size(); ++index) {
                if (index) std::cout << ',';
                std::cout << typing_durations[index];
            }
            std::cout << ']';
        }
        std::cout << "}\n";
        if (learn) engine.learn(wide_pinyin, localpinyin::utf8_to_wide(expected), rank == 1, preceding);
    }
    return 0;
}
