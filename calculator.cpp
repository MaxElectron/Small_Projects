#include <algorithm>
#include <iostream>
#include <optional>
#include <stack>
#include <sstream>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

// help message
const std::string HELPMSG = "help (h): display this message\nquit (q): quit the program\n";

// special characters as class
struct special_char {
    enum type {
        PLUS = '+',
        MINUS = '-',
        MULTIPLY = '*',
        DIVIDE = '/',
        LEFT_PARENTHESIS = '(',
        RIGHT_PARENTHESIS = ')'
    };

    type value;
};

// expression token descriptor
template <typename T, typename U = special_char>
using token = std::variant<T, U>;

// stream interaction for tokens
template <typename T>
std::ostream& operator<<(std::ostream& out, const token<T>& token) {
    if (std::holds_alternative<T>(token)) {
        out << std::get<T>(token);
    } else {
        out << (char)std::get<special_char>(token).value;
    }

    return out;
}

// expression parser
template <typename T>
std::vector<token<T>> parse(const std::optional<token<T>>& previous_result, const std::string& expression) {
    std::vector<token<T>> tokens;
    std::istringstream iss(expression);

    if (previous_result.has_value()) {
        tokens.push_back(previous_result.value());
    }

    char new_char;
    char prev_char = previous_result.has_value() ? '0' : '\0';
    std::string buffer;

    while (iss.get(new_char)) {
        switch (new_char) {
            case '-':
                // handle unary minus check
                if (!(prev_char >= '0' && prev_char <= '9') && prev_char != ')') {
                    if (buffer != "") {
                        buffer = "";
                        break;
                    }
                    buffer += new_char;
                    break;
                }
            case '+':
            case '*':
            case '/':
            case ')':
                if (buffer != "") {
                    T value;
                    std::stringstream(buffer) >> value;
                    tokens.push_back(value);
                    buffer = "";
                }

                tokens.push_back(special_char{static_cast<special_char::type>(new_char)});
                break;
            case '(':
                if (prev_char == ')') {
                    tokens.push_back(special_char{special_char::MULTIPLY});
                    prev_char = '*';
                }

                if (prev_char == '-' && buffer != "") {
                    buffer += '1';
                    T value;
                    std::stringstream(buffer) >> value;
                    tokens.push_back(value);
                    buffer = "";
                    tokens.push_back(special_char{special_char::MULTIPLY});
                    prev_char = '*';
                }

                if (buffer != "") {
                    T value;
                    std::stringstream(buffer) >> value;
                    tokens.push_back(value);
                    buffer = "";
                }

                if (prev_char != '(' && prev_char != '+' && prev_char != '-' && prev_char != '*' && prev_char != '/' && prev_char != '\0') {
                    tokens.push_back(special_char{special_char::MULTIPLY});
                    prev_char = '*';
                }
                
                tokens.push_back(special_char{static_cast<special_char::type>(new_char)});
                break;
            default:
                if (prev_char == ')') {
                    tokens.push_back(special_char{special_char::MULTIPLY});
                    prev_char = '*';
                }
                buffer += new_char;
        }

        prev_char = new_char;
    }

    if (buffer != "") {
        T value;
        std::stringstream(buffer) >> value;
        tokens.push_back(value);
        buffer = "";
    }

    return tokens;
}

// expression evaluator
template <typename T>
T evaluate(const std::vector<token<T>>& expression) {
    // convert to postfix notation using shunting yard algorithm
    std::vector<token<T>> infix_expression, postfix_expression;
    std::stack<token<T>> stack;

    infix_expression = expression;
    std::reverse(infix_expression.begin(), infix_expression.end());

    while (!infix_expression.empty()) {
        token<T> token = infix_expression.back();
        infix_expression.pop_back();

        if (std::holds_alternative<T>(token)) {
            postfix_expression.push_back(token);
            continue;
        }

        // the token is a special character
        switch (std::get<special_char>(token).value) {
            case special_char::PLUS:
                while (!stack.empty() && std::get<special_char>(stack.top()).value != special_char::LEFT_PARENTHESIS) {
                    postfix_expression.push_back(stack.top());
                    stack.pop();
                }
                stack.push(token);
                break;
            case special_char::MINUS:
                while (!stack.empty() && std::get<special_char>(stack.top()).value != special_char::LEFT_PARENTHESIS) {
                    postfix_expression.push_back(stack.top());
                    stack.pop();
                }
                stack.push(token);
                break;
            case special_char::MULTIPLY:
            case special_char::DIVIDE:
            case special_char::LEFT_PARENTHESIS:
                stack.push(token);
                break;
            case special_char::RIGHT_PARENTHESIS:
                while (!stack.empty() && std::get<special_char>(stack.top()).value != special_char::LEFT_PARENTHESIS) {
                    postfix_expression.push_back(stack.top());
                    stack.pop();
                }
                stack.pop();
                break;
        }
    }

    while (!stack.empty()) {
        postfix_expression.push_back(stack.top());
        stack.pop();
    }

    // evaluate postfix expression
    for (auto current_token : postfix_expression) {
        if (std::holds_alternative<T>(current_token)) {
            stack.push(current_token);
            continue;
        }

        T a = std::get<T>(stack.top());
        stack.pop();
        T b = std::get<T>(stack.top());
        stack.pop();

        switch (std::get<special_char>(current_token).value) {
            case special_char::PLUS:
                stack.push(b + a);
                break;
            case special_char::MINUS:
                stack.push(b - a);
                break;
            case special_char::MULTIPLY:
                stack.push(b * a);
                break;
            case special_char::DIVIDE:
                stack.push(b / a);
                break;
        }
    }

    return std::get<T>(stack.top());
}

// main loop
int main() {
    std::optional<token<float>> previous_result = std::nullopt;

    while (true) {
        std::string input;
        std::cout << "> ";
        if (previous_result.has_value()) {
            std::cout << previous_result.value() << " ";
        }

        std::getline(std::cin, input);

        input.erase(std::remove_if(input.begin(), input.end(), isspace), input.end());

        if (input == "") {
            previous_result = std::nullopt;
            continue;
        }

        if (input == "help" || input == "h") {
            std::cout << HELPMSG << "\n";
            continue;
        }

        if (input == "quit" || input == "q") {
            break;
        }

        std::vector<token<float>> tokens = parse<float>(previous_result, input);

        // print tokens
        // for (const auto& token : tokens) {
        //     std::cout << token << "\n";
        // }

        previous_result = std::make_optional(evaluate<float>(tokens));
    }
}