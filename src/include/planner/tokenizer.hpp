#pragma once
#include <string>
#include <string_view>
#include <vector>

namespace oarfish {

enum class TokenKind { Identifier, String, Number, Star, Comma, Operator, Semicolon, End };

struct Token {
    TokenKind kind;
    std::string text;
};

std::vector<Token> tokenize(std::string_view input);

}
