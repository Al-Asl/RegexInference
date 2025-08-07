#include "regex_match.hpp"

 Char::Char(char c) : c(c) {}
 bool Char::match(const string& word) const {
    return word.size() == 1 && word[0] == c;
}

 Or::Or(shared_ptr<Regex> l, shared_ptr<Regex> r) : left(l), right(r) {}
 bool Or::match(const string& word) const {
    return left->match(word) || right->match(word);
}

 And::And(shared_ptr<Regex> l, shared_ptr<Regex> r) : left(l), right(r) {}
 bool And::match(const string& word) const {
    return left->match(word) && right->match(word);
}

 Star::Star(shared_ptr<Regex> n) : node(n) {}
 bool Star::match(const string& word) const {
    if (word.empty()) return true;
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

 Optional::Optional(shared_ptr<Regex> n) : node(n) {}
 bool Optional::match(const string& word) const {
    return word.empty() || node->match(word);
}

 Concat::Concat(shared_ptr<Regex> l, shared_ptr<Regex> r) : left(l), right(r) {}
 bool Concat::match(const string& word) const {
    for (size_t i = 0; i <= word.size(); ++i) {
        string leftPart = word.substr(0, i);
        string rightPart = word.substr(i);
        if (left->match(leftPart) && right->match(rightPart)) {
            return true;
        }
    }
    return false;
}

 Parser::Parser(const string& s) : regex(s), pos(0) {}

 char Parser::peek() {
    return pos < regex.size() ? regex[pos] : '\0';
}

 char Parser::get() {
    return regex[pos++];
}

 shared_ptr<Regex> Parser::parse() {
     return parseOr();
 }

 shared_ptr<Regex> Parser::parseOr() {
    shared_ptr<Regex> node = parseIntersection();
    while (peek() == '+') {
        get();
        shared_ptr<Regex> right = parseIntersection();
        node = make_shared<Or>(node, right);
    }
    return node;
}

 shared_ptr<Regex> Parser::parseIntersection() {
    shared_ptr<Regex> node = parseConcat();
    while (peek() == '&') {
        get();
        shared_ptr<Regex> right = parseConcat();
        node = make_shared<And>(node, right);
    }
    return node;
}

 shared_ptr<Regex> Parser::parseConcat() {
    shared_ptr<Regex> node = parseFactor();
    while (true) {
        char c = peek();
        if (c == '\0' || c == ')' || c == '+' || c == '&') break;
        shared_ptr<Regex> next = parseFactor();
        node = make_shared<Concat>(node, next);
    }
    return node;
}

 shared_ptr<Regex> Parser::parseFactor() {
    shared_ptr<Regex> node = parseBase();
    while (peek() == '*' || peek() == '?') {
        char op = get();
        if (op == '*') node = make_shared<Star>(node);
        else if (op == '?') node = make_shared<Optional>(node);
    }
    return node;
}

 shared_ptr<Regex> Parser::parseBase() {
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

 bool match(const string& pattern, const string& word) {
    Parser parser(pattern);
    shared_ptr<Regex> tree = parser.parse();
    return tree->match(word);
}

 vector<bool> match(const vector<string>& examples, const string& pattern)
 {
     vector<bool> res(examples.size());

     if (pattern != "eps")
     {
         Parser parser(pattern);
         shared_ptr<Regex> tree = parser.parse();

         for (int i = 0; i < examples.size(); i++)
             res[i] = tree->match(examples[i]);
     }
     else
     {
         for (int i = 0; i < examples.size(); i++)
             res[i] = examples[i].empty();
     }

     return res;
 }