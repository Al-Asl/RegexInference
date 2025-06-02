#ifndef REGEX_MATCH_HPP
#define REGEX_MATCH_HPP

#include <iostream>
#include <string>
#include <memory>
#include <set>

using namespace std;

class Regex {
public:
    virtual bool match(const string& word) const = 0;
    virtual ~Regex() = default;
};

class Char : public Regex {
    char c;
public:
    Char(char c) : c(c) {}
    bool match(const string& word) const override {
        return word.size() == 1 && word[0] == c;
    }
};

class Or : public Regex {
    shared_ptr<Regex> left, right;
public:
    Or(shared_ptr<Regex> l, shared_ptr<Regex> r) : left(l), right(r) {}
    bool match(const string& word) const override {
        return left->match(word) || right->match(word);
    }
};

class And : public Regex {
    shared_ptr<Regex> left, right;
public:
    And(shared_ptr<Regex> l, shared_ptr<Regex> r) : left(l), right(r) {}
    bool match(const string& word) const override {
        return left->match(word) && right->match(word);
    }
};

class Star : public Regex {
    shared_ptr<Regex> node;
public:
    Star(shared_ptr<Regex> n) : node(n) {}
    bool match(const string& word) const override {
        for (size_t i = 1; i <= word.size(); ++i) {
            string prefix = word.substr(0, i);
            string suffix = word.substr(i);
            if (node->match(prefix)) {
                if (suffix.empty() || this->match(suffix)) {
                    return true;
                }
            }
        }
        return false;
    }
};

class Optional : public Regex {
    shared_ptr<Regex> node;
public:
    Optional(shared_ptr<Regex> n) : node(n) {}
    bool match(const string& word) const override {
        return word.empty() || node->match(word);
    }
};

class Concat : public Regex {
    shared_ptr<Regex> left, right;
public:
    Concat(shared_ptr<Regex> l, shared_ptr<Regex> r) : left(l), right(r) {}
    bool match(const string& word) const override {
        for (size_t i = 0; i <= word.size(); ++i) {
            string leftPart = word.substr(0, i);
            string rightPart = word.substr(i);
            if (left->match(leftPart) && right->match(rightPart)) {
                return true;
            }
        }
        return false;
    }
};

class Parser {
    string regex;
    size_t pos;

    char peek() {
        return pos < regex.size() ? regex[pos] : '\0';
    }

    char get() {
        return regex[pos++];
    }

    shared_ptr<Regex> parseOr() {
        shared_ptr<Regex> node = parseIntersection();
        while (peek() == '+') {
            get();
            shared_ptr<Regex> right = parseIntersection();
            node = make_shared<Or>(node, right);
        }
        return node;
    }

    shared_ptr<Regex> parseIntersection() {
        shared_ptr<Regex> node = parseConcat();
        while (peek() == '&') {
            get();
            shared_ptr<Regex> right = parseConcat();
            node = make_shared<And>(node, right);
        }
        return node;
    }

    shared_ptr<Regex> parseConcat() {
        shared_ptr<Regex> node = parseFactor();
        while (true) {
            char c = peek();
            // we keep consuming if the next token is valid
            if (c == '\0' || c == ')' || c == '+' || c == '&') break;

            shared_ptr<Regex> next = parseFactor();
            node = make_shared<Concat>(node, next);
        }
        return node;
    }

    // Parse factors (* ?)
    shared_ptr<Regex> parseFactor() {
        shared_ptr<Regex> node = parseBase();
        while (peek() == '*' || peek() == '?') {
            char op = get();
            if (op == '*') node = make_shared<Star>(node);
            else if (op == '?') node = make_shared<Optional>(node);
        }
        return node;
    }

    // Parse base (grouping or character)
    shared_ptr<Regex> parseBase() {
        if (peek() == '(') {
            get();
            shared_ptr<Regex> node = parseOr();
            if (get() != ')') throw runtime_error("Missing ')'");
            return node;
        }
        else {
            char c = get();
            return make_shared<Char>(c);
        }
    }

public:
    Parser(const string& s) : regex(s), pos(0) {}
    shared_ptr<Regex> parse() {
        return parseOr();
    }
};

bool match(const string& pattern, const string& word) {
    Parser parser(pattern);
    shared_ptr<Regex> tree = parser.parse();
    return tree->match(word);
}

#endif