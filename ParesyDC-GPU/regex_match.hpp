#ifndef REGEX_MATCH_HPP
#define REGEX_MATCH_HPP

#include <string>
#include <memory>
#include <stdexcept>
#include <vector>

using std::string;
using std::shared_ptr;
using std::make_shared;
using std::runtime_error;
using std::vector;

class Regex {
public:
    virtual bool match(const string& word) const = 0;
    virtual ~Regex() = default;
};

class Char : public Regex {
    char c;
public:
     Char(char c);
     bool match(const string& word) const override;
};

class Or : public Regex {
    shared_ptr<Regex> left, right;
public:
     Or(shared_ptr<Regex> l, shared_ptr<Regex> r);
     bool match(const string& word) const override;
};

class And : public Regex {
    shared_ptr<Regex> left, right;
public:
     And(shared_ptr<Regex> l, shared_ptr<Regex> r);
     bool match(const string& word) const override;
};

class Star : public Regex {
    shared_ptr<Regex> node;
public:
     Star(shared_ptr<Regex> n);
     bool match(const string& word) const override;
};

class Optional : public Regex {
    shared_ptr<Regex> node;
public:
     Optional(shared_ptr<Regex> n);
     bool match(const string& word) const override;
};

class Concat : public Regex {
    shared_ptr<Regex> left, right;
public:
     Concat(shared_ptr<Regex> l, shared_ptr<Regex> r);
     bool match(const string& word) const override;
};

class Parser {
    string regex;
    size_t pos;

     char peek();
     char get();
     shared_ptr<Regex> parseOr();
     shared_ptr<Regex> parseIntersection();
     shared_ptr<Regex> parseConcat();
     shared_ptr<Regex> parseFactor();
     shared_ptr<Regex> parseBase();

public:
     Parser(const string& s);
     shared_ptr<Regex> parse();
};

 bool match(const string& pattern, const string& word);

 vector<bool> match(const vector<string>& examples, const string& pattern);

#endif