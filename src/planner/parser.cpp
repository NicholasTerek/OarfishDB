#include "src/include/planner/parser.hpp"
#include "src/include/planner/tokenizer.hpp"
#include <cctype>
#include <limits>
#include <stdexcept>
#include <utility>

namespace oarfish {
namespace {

bool is_keyword(const Token& token, std::string_view keyword) {
    if (token.kind != TokenKind::Identifier || token.text.size() != keyword.size()) return false;
    for (std::size_t index = 0; index < keyword.size(); ++index) {
        if (std::toupper(static_cast<unsigned char>(token.text[index])) != keyword[index]) return false;
    }
    return true;
}

class Parser {
public:
    explicit Parser(std::string_view input) : tokens_(tokenize(input)) {}

    SelectQuery parse() {
        expect_keyword("SELECT");
        SelectQuery query;
        if (accept(TokenKind::Star)) {
            query.columns.push_back("*");
        } else {
            query.columns.push_back(identifier());
            while (accept(TokenKind::Comma)) query.columns.push_back(identifier());
        }

        expect_keyword("FROM");
        query.table = identifier();

        if (accept_keyword("WHERE")) {
            Predicate predicate;
            predicate.column = identifier();
            const auto op = consume(TokenKind::Operator).text;
            if (op == "=") predicate.op = Comparison::Equal;
            else if (op == "!=") predicate.op = Comparison::NotEqual;
            else if (op == "<") predicate.op = Comparison::Less;
            else if (op == "<=") predicate.op = Comparison::LessEqual;
            else if (op == ">") predicate.op = Comparison::Greater;
            else if (op == ">=") predicate.op = Comparison::GreaterEqual;
            else throw std::invalid_argument("Invalid comparison operator");

            if (current().kind != TokenKind::String && current().kind != TokenKind::Number) {
                throw std::invalid_argument("Expected literal");
            }
            predicate.value = tokens_[position_++].text;
            query.predicate = std::move(predicate);
        }

        if (accept_keyword("LIMIT")) {
            const auto token = consume(TokenKind::Number).text;
            unsigned long long limit;
            try {
                limit = std::stoull(token);
            } catch (const std::exception&) {
                throw std::invalid_argument("Invalid LIMIT");
            }
            if (limit > std::numeric_limits<std::size_t>::max()) {
                throw std::invalid_argument("LIMIT too large");
            }
            query.limit = static_cast<std::size_t>(limit);
        }

        accept(TokenKind::Semicolon);
        consume(TokenKind::End);
        return query;
    }

private:
    const Token& current() const { return tokens_.at(position_); }

    bool accept(TokenKind kind) {
        if (current().kind != kind) return false;
        ++position_;
        return true;
    }

    Token consume(TokenKind kind) {
        if (current().kind != kind) throw std::invalid_argument("Unexpected query token");
        return tokens_[position_++];
    }

    bool accept_keyword(std::string_view keyword) {
        if (!is_keyword(current(), keyword)) return false;
        ++position_;
        return true;
    }

    void expect_keyword(std::string_view keyword) {
        if (!accept_keyword(keyword)) throw std::invalid_argument("Expected SQL keyword");
    }

    std::string identifier() { return consume(TokenKind::Identifier).text; }

    std::vector<Token> tokens_;
    std::size_t position_ = 0;
};

}

SelectQuery parse_select(std::string_view sql) {
    return Parser(sql).parse();
}

}
