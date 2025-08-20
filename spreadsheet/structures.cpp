#include "common.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <sstream>
#include <stdexcept>

const int LETTERS = 26;
const int MAX_POSITION_LENGTH = 17;
const int MAX_POS_LETTER_COUNT = 3;

const Position Position::NONE = {-1, -1};

bool Position::operator==(const Position rhs) const {
    return row == rhs.row && col == rhs.col;
}

bool Position::operator<(const Position rhs) const {
    return (col == rhs.col && row < rhs.row) || (row == rhs.row && col < rhs.col) || (row < rhs.row && col < rhs.col);
}

bool Position::IsValid() const {
    return row < 16384 && row >= 0 && col < 16384 && col >= 0;
}

std::string Position::GetLettersFromInt(int num) {
    std::string result;
    int mod = num;
    while (num >= 0) {
        mod = num % LETTERS;
        num = num / LETTERS - 1;
        char letter = 'A' + mod;
        result += letter;
    }
    std::reverse(result.begin(), result.end());
    return result;
}

std::string Position::ToString() const {
    if (!IsValid()) {
        return {};
    }

    std::string result;
    result += GetLettersFromInt(col);
    result += std::to_string(row + 1);

    return result;
}

int Position::GetIntFromLetters(std::string_view str) {
    int result = 0;
    int current_pow = str.size() - 1;
    for (auto letter : str) {
        if (letter < 'A' || letter > 'Z') {
            throw std::invalid_argument("");
        }
        int index = letter - 'A' + 1;
        result += index * static_cast<int>(pow(LETTERS, current_pow));
        --current_pow;
    }
    return result;
}

Position Position::FromString(std::string_view str) {
    Position result = Position::NONE;

    size_t i = 0;
    while (i < str.size() && !isdigit(str[i])) {
        ++i;
    }

    size_t j = i + 1;
    while (j < str.size()) {
        if (!isdigit(str[j])) {
            return Position::NONE;
        }
        ++j;
    }

    if (str.size() > 1 && str.size() <= MAX_POSITION_LENGTH && i > 0 && i <= MAX_POS_LETTER_COUNT) {
        try {
            result.row = std::stoi(std::string(str.substr(i))) - 1;
            result.col = GetIntFromLetters(str.substr(0, i)) - 1;
        } catch (...) {
            return Position::NONE;
        }
    }

    return result;
}

bool Size::operator==(Size rhs) const {
    return rows == rhs.rows && cols == rhs.cols;
}

std::ostream& operator<<(std::ostream& output, const CellInterface::Value& value) {
    std::visit(
        [&](const auto& x) {
            output << x;
        },
        value);
    return output;
}