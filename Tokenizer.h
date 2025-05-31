#pragma once
#include <4dm.h>
#include "CommandExceptions.h"
#include "Parsing.h"

using namespace commandExceptions;
using namespace parsing;

namespace tokenizer {

    static void parseCommandName(
        std::stringstream& commandStream,
        std::string& command
    ) {
        commandStream >> command;
        command = command.substr(1);
        if (command.empty()) {
            throw CommandSyntaxException(command, "expected command name after /");
        }
    }

    static void parseModifiers(
        std::stringstream& commandStream,
        std::unordered_set<std::string>& modifiers
    ) {
        std::string token;
        while (true) {
            std::streampos pos = commandStream.tellg();
            if (!(commandStream >> token)) {
                return;
            }
            if (token.size() >= 3 && token[0] == '-' && token[1] == '-') {
                if (token.find('"') != std::string::npos) {
                    throw CommandSyntaxException(token, "no quotes are allowed in modifiers");
                }
                modifiers.insert(token.substr(2));
            }
            else {
                commandStream.clear();
                commandStream.seekg(pos);
                return;
            }
        }
    }
    
    inline static void parseParameters(
        std::stringstream& commandStream,
        std::vector<std::string>& parameters
    ) {
        std::string tok;
        while (commandStream >> tok) {
            if (tok.rfind("--", 0) == 0)
                throw CommandSyntaxException(tok, "modifier after parameter");
            int qc = 0;
            for (char c : tok) if (c == '"') ++qc;
            if (qc > 2) throw CommandSyntaxException(tok, "invalid quote position");
            if (qc == 1) {
                size_t p = tok.find('"');
                if (p != 0 && p != tok.size() - 1)
                    throw CommandSyntaxException(tok, "invalid quote position");
            }
            if (qc == 2) {
                if (!(tok.front() == '"' && tok.back() == '"'))
                    throw CommandSyntaxException(tok, "invalid quote position");
            }
            parameters.push_back(tok);
        }

        for (size_t i = 0; i < parameters.size(); ) {
            std::string& s = parameters[i];
            if (s.size() > 1 && s.front() == '"' && s.back() == '"') {
                s = s.substr(1, s.size() - 2);
                ++i;
                continue;
            }
            if (s.front() == '"' && (s.size() == 1 || s.back() != '"')) {
                std::string accum = (s.size() > 1 ? s.substr(1) : std::string());
                bool closed = false;
                size_t j = i + 1;
                for (; j < parameters.size(); ++j) {
                    std::string& t2 = parameters[j];
                    if (!t2.empty() && t2.back() == '"') {
                        if (t2.front() == '"' && t2.size() > 1)
                            throw CommandSyntaxException(t2, "invalid quote position");
                        if (t2.size() > 1)
                            accum += " " + t2.substr(0, t2.size() - 1);
                        parameters[i] = accum;
                        parameters.erase(parameters.begin() + (i + 1),
                            parameters.begin() + (j + 1));
                        closed = true;
                        break;
                    }
                    if (t2.find('"') != std::string::npos)
                        throw CommandSyntaxException(t2, "invalid quote position");
                    accum += " " + t2;
                }
                if (!closed) throw CommandSyntaxException(accum, "mismatched quote");
                continue;
            }
            if (!s.empty() && s.back() == '"' && s.front() != '"')
                throw CommandSyntaxException(s, "invalid quote position");
            ++i;
        }

        for (size_t i = 0; i < parameters.size(); ) {
            std::string& s = parameters[i];
            bool merged = false;
            if (!s.empty()) {
                char first = s.front();
                char last = s.back();
                if (last == '(' && i + 1 < parameters.size()) {
                    s += parameters[i + 1];
                    parameters.erase(parameters.begin() + (i + 1));
                    merged = true;
                }
                else if (first == ')' && i > 0) {
                    parameters[i - 1] += s;
                    parameters.erase(parameters.begin() + i);
                    i = (i > 0 ? i - 1 : 0);
                    merged = true;
                }
                else if ((first == '+' || first == '-' || first == '*' || first == '/')) {
                    if (i > 0) {
                        parameters[i - 1] += s;
                        parameters.erase(parameters.begin() + i);
                        i = (i > 0 ? i - 1 : 0);
                    }
                    else if (i + 1 < parameters.size()) {
                        s += parameters[i + 1];
                        parameters.erase(parameters.begin() + (i + 1));
                    }
                    merged = true;
                }
                else if ((last == '+' || last == '-' || last == '*' || last == '/') && i + 1 < parameters.size()) {
                    s += parameters[i + 1];
                    parameters.erase(parameters.begin() + (i + 1));
                    merged = true;
                }
            }
            if (!merged) ++i;
        }

        int bal = 0;
        for (const auto& s : parameters) {
            for (char c : s) {
                if (c == '(') ++bal;
                else if (c == ')') --bal;
                if (bal < 0) throw CommandSyntaxException(s, "parentheses mismatch");
            }
        }
        if (bal != 0) throw CommandSyntaxException("", "parentheses mismatch");
    }

    void tokenize(
        std::stringstream& commandStream,
        std::string& command,
        std::unordered_set<std::string>& modifiers,
        std::vector<std::string>& parameters
    ) {
        parseCommandName(commandStream, command);

        parseModifiers(commandStream, modifiers);

        parseParameters(commandStream, parameters);

        
        Console::printLine("command = ", command);
        for (const auto& mod : modifiers) {
            Console::printLine("modifier = ", mod);
        }
        for (unsigned i = 0; i < parameters.size(); ++i) {
            Console::printLine(std::format("parameter[{}] = {}",i,parameters[i]));
        }
    }
}