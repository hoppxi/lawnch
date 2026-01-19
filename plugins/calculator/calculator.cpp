#include "lawnch_plugin_api.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <functional>
#include <iostream>
#include <map>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <string>
#include <vector>

// namespace.
namespace {

char *c_strdup(const std::string &s) {
  char *cstr = new char[s.length() + 1];
  std::strcpy(cstr, s.c_str());
  return cstr;
}

enum class TokenType {
  Number,
  Operator,
  Function,
  LeftParen,
  RightParen,
  Comma
};

struct Token {
  TokenType type;
  std::string text;
};

double factorial_impl(double x) {
  if (x < 0)
    return NAN;
  return std::tgamma(x + 1);
}

struct FuncDef {
  std::function<double(const std::vector<double> &)> fn;
  int arity;
};

const std::map<std::string, FuncDef> functions = {
    {"sqrt", {[](auto a) { return std::sqrt(a[0]); }, 1}},
    {"cbrt", {[](auto a) { return std::cbrt(a[0]); }, 1}},
    {"sin", {[](auto a) { return std::sin(a[0]); }, 1}},
    {"cos", {[](auto a) { return std::cos(a[0]); }, 1}},
    {"tan", {[](auto a) { return std::tan(a[0]); }, 1}},
    {"sinh", {[](auto a) { return std::sinh(a[0]); }, 1}},
    {"cosh", {[](auto a) { return std::cosh(a[0]); }, 1}},
    {"tanh", {[](auto a) { return std::tanh(a[0]); }, 1}},
    {"log", {[](auto a) { return std::log10(a[0]); }, 1}},
    {"ln", {[](auto a) { return std::log(a[0]); }, 1}},
    {"abs", {[](auto a) { return std::abs(a[0]); }, 1}},
    {"ceil", {[](auto a) { return std::ceil(a[0]); }, 1}},
    {"floor", {[](auto a) { return std::floor(a[0]); }, 1}},
    {"round", {[](auto a) { return std::round(a[0]); }, 1}},
    {"min", {[](auto a) { return std::min(a[0], a[1]); }, 2}},
    {"max", {[](auto a) { return std::max(a[0], a[1]); }, 2}},
    {"pow", {[](auto a) { return std::pow(a[0], a[1]); }, 2}},
    {"factorial", {[](auto a) { return factorial_impl(a[0]); }, 1}}};

int precedence(const std::string &op) {
  if (op == "+" || op == "-")
    return 1;
  if (op == "*" || op == "/")
    return 2;
  if (op == "^")
    return 3;
  return 0;
}

double apply_op(const std::string &op, double b, double a) {
  if (op == "+")
    return a + b;
  if (op == "-")
    return a - b;
  if (op == "*")
    return a * b;
  if (op == "/")
    return b == 0 ? NAN : a / b;
  if (op == "^")
    return std::pow(a, b);
  return 0;
}

std::vector<Token> tokenize(const std::string &s) {
  std::vector<Token> tokens;
  for (size_t i = 0; i < s.size();) {
    if (std::isspace(s[i])) {
      i++;
      continue;
    }
    if (s[i] == 'e') { // Special case for constant 'e'
      if (i == s.size() - 1 || !std::isalpha(s[i + 1])) {
        tokens.push_back({TokenType::Number, std::to_string(M_E)});
        i++;
        continue;
      }
    }
    if (s.rfind("pi", i) == i) { // Special case for constant 'pi'
      tokens.push_back({TokenType::Number, std::to_string(M_PI)});
      i += 2;
      continue;
    }

    if (std::isdigit(s[i]) || s[i] == '.') {
      size_t j = i;
      while (j < s.size() && (std::isdigit(s[j]) || s[j] == '.'))
        j++;
      tokens.push_back({TokenType::Number, s.substr(i, j - i)});
      i = j;
    } else if (std::isalpha(s[i])) {
      size_t j = i;
      while (j < s.size() && std::isalpha(s[j]))
        j++;
      tokens.push_back({TokenType::Function, s.substr(i, j - i)});
      i = j;
    } else {
      char c = s[i++];
      if (c == '(')
        tokens.push_back({TokenType::LeftParen, "("});
      else if (c == ')')
        tokens.push_back({TokenType::RightParen, ")"});
      else if (c == ',')
        tokens.push_back({TokenType::Comma, ","});
      else
        tokens.push_back({TokenType::Operator, std::string(1, c)});
    }
  }
  return tokens;
}

double evaluate(const std::vector<Token> &tokens) {
  std::stack<double> values;
  std::stack<Token> ops;
  std::vector<Token> rpn;
  for (const auto &t : tokens) {
    if (t.type == TokenType::Number) {
      rpn.push_back(t);
    } else if (t.type == TokenType::Function) {
      ops.push(t);
    } else if (t.type == TokenType::Comma) {
      while (!ops.empty() && ops.top().type != TokenType::LeftParen) {
        rpn.push_back(ops.top());
        ops.pop();
      }
    } else if (t.type == TokenType::Operator) {
      while (!ops.empty() && ops.top().type == TokenType::Operator &&
             precedence(ops.top().text) >= precedence(t.text)) {
        rpn.push_back(ops.top());
        ops.pop();
      }
      ops.push(t);
    } else if (t.type == TokenType::LeftParen) {
      ops.push(t);
    } else if (t.type == TokenType::RightParen) {
      while (!ops.empty() && ops.top().type != TokenType::LeftParen) {
        rpn.push_back(ops.top());
        ops.pop();
      }
      if (!ops.empty())
        ops.pop();
      if (!ops.empty() && ops.top().type == TokenType::Function) {
        rpn.push_back(ops.top());
        ops.pop();
      }
    }
  }
  while (!ops.empty()) {
    rpn.push_back(ops.top());
    ops.pop();
  }

  for (const auto &t : rpn) {
    if (t.type == TokenType::Number) {
      values.push(std::stod(t.text));
    } else if (t.type == TokenType::Operator) {
      if (values.size() < 2)
        throw std::runtime_error("Syntax Error");
      double a = values.top();
      values.pop();
      double b = values.top();
      values.pop();
      values.push(apply_op(t.text, a, b));
    } else if (t.type == TokenType::Function) {
      auto it = functions.find(t.text);
      if (it == functions.end())
        throw std::runtime_error("Unknown function");
      int arity = it->second.arity;
      if ((int)values.size() < arity)
        throw std::runtime_error("Not enough arguments");
      std::vector<double> args(arity);
      for (int i = arity - 1; i >= 0; --i) {
        args[i] = values.top();
        values.pop();
      }
      values.push(it->second.fn(args));
    }
  }
  if (values.empty())
    return 0;
  if (values.size() > 1)
    throw std::runtime_error("Syntax Error");
  return values.top();
}

std::vector<LawnchResult> do_calc_query(const std::string &expr) {
  std::string input = expr;
  input.erase(std::remove_if(input.begin(), input.end(),
                             [](char c) {
                               return !std::isalnum(c) &&
                                      std::string("+-*/^()., ").find(c) ==
                                          std::string::npos;
                             }),
              input.end());

  if (input.empty()) {
    return {{c_strdup("Calculator"),
             c_strdup("Supports: sin, cos, sqrt, pow, log..."),
             c_strdup("accessories-calculator"), c_strdup(""), c_strdup("calc"),
             c_strdup("")}};
  }

  try {
    auto tokens = tokenize(input);
    double result = evaluate(tokens);
    if (std::isnan(result) || std::isinf(result))
      throw std::runtime_error("Math Error");

    std::stringstream ss;
    ss.precision(10);
    ss << result;
    std::string out = ss.str();

    return {{c_strdup(out.c_str()), c_strdup("Click to copy result"),
             c_strdup("accessories-calculator"),
             c_strdup(("echo -n '" + out + "' | wl-copy").c_str()),
             c_strdup("calc"), c_strdup("")}};
  } catch (const std::exception &e) {
    return {{c_strdup((std::string("Error: ") + e.what()).c_str()),
             c_strdup(input.c_str()), c_strdup("dialog-error"), c_strdup(""),
             c_strdup("calc"), c_strdup("")}};
  }
}

} // namespace

void plugin_init(const LawnchHostApi *host) { /* No state to init */ }
void plugin_destroy(void) { /* No state to destroy */ }

const char **plugin_get_triggers(void) {
  static const char *triggers[] = {":calc", "=", nullptr};
  return triggers;
}

LawnchResult *plugin_get_help(void) {
  LawnchResult *result = new LawnchResult;
  result->name = c_strdup(":calc / =");
  result->comment = c_strdup("Calculator");
  result->icon = c_strdup("accessories-calculator");
  result->command = c_strdup("");
  result->type = c_strdup("help");
  result->preview_image_path = c_strdup("");
  return result;
}

void plugin_free_results(LawnchResult *results, int num_results) {
  if (!results)
    return;
  for (int i = 0; i < num_results; ++i) {
    delete[] results[i].name;
    delete[] results[i].comment;
    delete[] results[i].icon;
    delete[] results[i].command;
    delete[] results[i].type;
    delete[] results[i].preview_image_path;
  }
  delete[] results;
}

LawnchResult *plugin_query(const char *term, int *num_results) {
  auto results_vec = do_calc_query(term);
  *num_results = results_vec.size();
  if (*num_results == 0) {
    return nullptr;
  }
  LawnchResult *result_arr = new LawnchResult[*num_results];
  std::copy(results_vec.begin(), results_vec.end(), result_arr);
  return result_arr;
}

static LawnchPluginVTable g_vtable = {.plugin_api_version =
                                          LAWNCH_PLUGIN_API_VERSION,
                                      .init = plugin_init,
                                      .destroy = plugin_destroy,
                                      .get_triggers = plugin_get_triggers,
                                      .get_help = plugin_get_help,
                                      .query = plugin_query,
                                      .free_results = plugin_free_results};

PLUGIN_API LawnchPluginVTable *lawnch_plugin_entry(void) { return &g_vtable; }
