#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <variant>
#include <fstream>

// #include <immer/vector.hpp>
#include <vector>

namespace impl {
    template <typename... Bases> struct visitor : Bases... {
        using Bases::operator()...;
    };

    template <typename... Bases> visitor(Bases...) -> visitor<Bases...>;

    template <typename G> struct Y {
      template <typename... X> decltype(auto) operator()(X &&...x) const & {
        return g(*this, std::forward<X>(x)...);
      }
      G g;
    };

    template <typename G> Y(G) -> Y<G>;
}

namespace pat {
    struct top; // forward declaration
    struct seq; // forward declaration
    struct seq; // forward declaration
    struct par; // forward declaration
    struct prm; // forward declaration
    struct pmm; // forward declaration

    using value = std::variant<std::string, top, seq, par, prm, pmm>;

    struct top : std::vector<value> {
      using std::vector<value>::vector;
    };

    struct seq : std::vector<value> {
        using std::vector<value>::vector;
    };

    struct par : std::vector<value> {
        using std::vector<value>::vector;
    };

    // invariant must be two elements long, this is used rather 
    // than tuple to enable explicit allocation for recursion
    struct prm : std::vector<value> {
        using std::vector<value>::vector;
    };

    // invariant must be two elements long, this is used rather
    // than tuple to enable explicit allocation for recursion
    struct pmm : std::vector<value> {
      using std::vector<value>::vector;
    };

    struct printer {
    private:
      std::ostream &os_;
      bool toplevel_;

    public:
      printer(std::ostream &os) : os_{os}, toplevel_{true} {}

      std::ostream &operator()(std::string const &s) const {
        return os_ << s;
      }
      
      std::ostream &operator()(top const &a) const {

        for (int i = 0; i < a.size(); i++) {
          if (i != a.size() - 1) {
            std::visit(*this, a[i]) << ' ';
          } else {
            std::visit(*this, a[i]);
          }
        }
        // for (auto const &v : a)

        return os_;
      }
      
      std::ostream &operator()(seq const &a) const {
        os_ << '[';
                
        for (int i = 0; i < a.size(); i++) {
          if (i != a.size() - 1) {
            std::visit(*this, a[i]) << ' ';
          }
          else {
            std::visit(*this, a[i]);
          }
        }
          //for (auto const &v : a)
            
        return os_ << ']';
      }
      std::ostream &operator()(par const &a) const {
        os_ << '[';
        for (auto const &v : a)
          std::visit(*this, v) << ' ';
        return os_ << ']';
      }
      std::ostream &operator()(prm const &a) const {
        std::visit(*this, a[0]) << " |:| ";
        std::visit(*this, a[1]);
        return os_;
      }
      std::ostream &operator()(pmm const &a) const {
        std::visit(*this, a[0]) << " |+| ";
        std::visit(*this, a[1]);
        return os_;
      }
    };

    std::ostream &operator<<(std::ostream &os, value const &val) {
      return std::visit(printer{os}, val);
    }

    // Simple parser

    enum class TOKEN {
      LIST_OPEN,
      LIST_CLOSE,
      PRM,
      PMM,
      VARIABLE,
      SLIENT,
    };

    struct Token {
        std::string value_;
        TOKEN type_;
    
        std::string toString() {
            switch (type_) {
                case TOKEN::LIST_OPEN: {
                    return "[";
                }
                case TOKEN::LIST_CLOSE: {
                    return "]";
                }
                case TOKEN::PMM: {
                    return "|+|";
                }
                case TOKEN::PRM: {
                    return "|:|";
                }
                case TOKEN::VARIABLE: {
                    return value_;
                }
                case TOKEN::SLIENT: {
                  return "-";
                }
                default:
                    break;
            }

            return "";
        }
    };

    class Tokenizer {
      std::istream& file_;
      size_t prevPos_;

    public:
      Tokenizer(std::istream& s) : file_{s} { }

      char getWithoutWhiteSpace() {
        char c = ' ';
        // std::cout << "e" << file_.eof() << std::endl;
        while ((c == ' ' || c == '\n')) {
          file_.get(c); // check

          // std::cout << "**" << c << "**" << file_.eof() << std::endl;
          if ((c == ' ' || c == '\n') && !file_.good()) {
            // std::cout << file.eof() << " " << file.fail() << std::endl;
            throw std::logic_error("Ran out of tokens");
          } else if (!file_.good()) {
            return c;
          }
        }

        return c;
      }

      Token getToken() {
        char c;
        if (file_.eof()) {
          std::cout << "Exhaused tokens" << std::endl;
          // throw std::exception("Exhausted tokens");
        }
        prevPos_ = file_.tellg();
        c = getWithoutWhiteSpace();

        struct Token token;
        if (c == '[') {
          token.type_ = TOKEN::LIST_OPEN;
        } else if (c == ']') {
          token.type_ = TOKEN::LIST_CLOSE;
        } else if (c == '|') { // handle PRM or PMM operator
          file_.get(c);
          if (c == ':') {
            token.type_ = TOKEN::PRM;
          } else if (c == '+') {
            token.type_ = TOKEN::PMM;
          } else {
            throw std::logic_error("Expected : or +");
          }
          file_.get(c);
          if (c != '|') {
            throw std::logic_error("Expected |");
          }
        } else if (c == '-') {
          token.type_ = TOKEN::SLIENT;
        } else if (isalpha(c)) {
          token.type_ = TOKEN::VARIABLE;
          token.value_ = "";
          token.value_.push_back(c);
          std::streampos prevCharPos = file_.tellg();
          // file_.get(c);
          while (isalpha(c)) {
            prevCharPos = file_.tellg();
            file_.get(c);

            if (file_.eof()) {
              break;
            } else {
              if (isalpha(c)) {
                token.value_.push_back(c);
              } else {
                file_.seekg(prevCharPos);
              }
            }
          }
        }

        return token;
      }

      bool hasMoreTokens() const {
        size_t prevPos = file_.tellg();
        char c;
        file_.get(c);
        if (file_.eof()) {
          return false;
        }
        file_.seekg(prevPos);
        return true;
      }

      void rollBackToken() const {
        if (file_.eof()) {
          file_.clear();
        }
        file_.seekg(prevPos_);
      }
    };

    class Parser {
    private:
        Tokenizer tokenizer_;
          
        value root_;
        //value current_;
        std::vector<value> current_;

        void parseMany() {
          while (tokenizer_.hasMoreTokens()) {
            value node = parseNode();
            current_.push_back(node);
          }
        }

      public:
        Parser(std::istream &s) : tokenizer_{s} {}

        value parse() {
          parseMany();
          top tt;
          
          for (auto &v : current_) {
            tt.push_back(v);
          }
          
          return value{tt};
        }

        value parseNode() { 
            Token token = tokenizer_.getToken();

            // std::cout << token.toString() << "\n";
            if (token.type_ == TOKEN::VARIABLE) {
              return value{token.value_};
            }
            else if (token.type_ == TOKEN::SLIENT) {
              return value{token.toString()};
            }
            else if (token.type_ == TOKEN::LIST_OPEN) {
              return parseList();
            }
            else if (token.type_ == TOKEN::PRM) {
                std::vector<value> left{current_};
                current_.clear();
                parseMany();
                std::vector<value> right{current_};
                current_.clear();

                top l;
                top r;

                for (auto &v: left) {
                  l.push_back(v);
                }

                for (auto &v : right) {
                  r.push_back(v);
                }

                return value{prm{l, r}};
            } 
            else if (token.type_ == TOKEN::PMM) {
              std::vector<value> left{current_};
              current_.clear();
              parseMany();
              std::vector<value> right{current_};
              current_.clear();

              top l;
              top r;

              for (auto &v : left) {
                l.push_back(v);
              }

              for (auto &v : right) {
                r.push_back(v);
              }

              return value{pmm{l, r}};
            }

          throw std::logic_error("Unexpected token");
        }

        value parseList() { 
            seq list;
            bool hasCompleted = false;
            while (!hasCompleted) {
                if (tokenizer_.hasMoreTokens()) {
                    Token nextToken = tokenizer_.getToken();  // lookahead
                    if (nextToken.type_ == TOKEN::LIST_CLOSE) {
                      hasCompleted = true;
                    }
                    else {
                      tokenizer_.rollBackToken();
                      value node = parseNode();
                      list.push_back(node);
                    }
                } else {
                  throw std::logic_error("No more tokens");
                }
            }

            return value{list};
        }
    };
} // namespace pat