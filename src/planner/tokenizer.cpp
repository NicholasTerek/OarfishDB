#include "src/include/planner/tokenizer.hpp"
#include <cctype>
#include <stdexcept>
#include <utility>

namespace oarfish {

std::vector<Token> tokenize(std::string_view input) {
    std::vector<Token> tokens;
    std::size_t pos = 0;

    while (pos < input.size()) {
        const unsigned char ch = static_cast<unsigned char>(input[pos]);
        if (std::isspace(ch)) {
            ++pos;
            continue;
        }

        if (std::isalpha(ch) || ch == '_') {
            const auto start = pos++;
            while (pos < input.size()) {
                const auto current = static_cast<unsigned char>(input[pos]);
                if (!std::isalnum(current) && current != '_') break;
                ++pos;
            }
            tokens.push_back({TokenKind::Identifier, std::string(input.substr(start, pos - start))});
            continue;
        }

        if (std::isdigit(ch)) {
            const auto start = pos++;
            while (pos < input.size() && std::isdigit(static_cast<unsigned char>(input[pos]))) ++pos;
            tokens.push_back({TokenKind::Number, std::string(input.substr(start, pos - start))});
            continue;
        }

        if (ch == '\'') {
            ++pos;
            std::string value;
            bool terminated = false;
            while (pos < input.size()) {
                if (input[pos] == '\'') {
                    ++pos;
                    if (pos < input.size() && input[pos] == '\'') {
                        value += '\'';
                        ++pos;
                        continue;
                    }
                    terminated = true;
                    break;
                }
                value += input[pos++];
            }
            if (!terminated) throw std::invalid_argument("Unterminated string literal");
            tokens.push_back({TokenKind::String, std::move(value)});
            continue;
        }

        if (ch == '*' || ch == ',' || ch == ';') {
            const auto kind = ch == '*' ? TokenKind::Star : ch == ',' ? TokenKind::Comma : TokenKind::Semicolon;
            tokens.push_back({kind, std::string(1, static_cast<char>(ch))});
            ++pos;
            continue;
        }

        if (ch == '=' || ch == '<' || ch == '>' || ch == '!') {
            const auto start = pos++;
            if (pos < input.size() && input[pos] == '=') ++pos;
            auto op = std::string(input.substr(start, pos - start));
            if (op == "!") throw std::invalid_argument("Expected != operator");
            tokens.push_back({TokenKind::Operator, std::move(op)});
            continue;
        }

        throw std::invalid_argument("Unexpected query character");
    }

    tokens.push_back({TokenKind::End, ""});
    return tokens;
}

}
