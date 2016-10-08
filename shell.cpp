/*
 * Copyright (C) 2016 Jussi Pakkanen.
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of version 3, or (at your option) any later version,
 * of the GNU General Public License as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include"shell.h"
#include<unistd.h>
#include<dirent.h>
#include<memory>
#include<iostream>
#include<fstream>
#include<stdexcept>
#include<algorithm>
#include<regex>

class ExitShell : public std::exception {
};

Cat::Cat(const std::vector<std::string> &args, Process *previous) : Process(previous) {
    auto i = args.begin();
    ++i;
    for(;i != args.end(); ++i) {
        std::ifstream ifile(*i, std::ios::in);
        for(std::string l; std::getline(ifile, l);) {
            contents.emplace_back(std::move(l));
        }
    }
}

Line Cat::pull() {
    if(offset >= contents.size()) {
        return Line{EOF_PIPE, ""};
    }
    return Line{STDOUT_PIPE, contents[offset++]};
}

Line Exit::pull() {
    throw ExitShell();
}

Cd::Cd(const std::vector<std::string> &args, Process *previous) : Process(previous) {
    if(args.size() != 2) {
        throw std::logic_error("Cd takes one argument");
    }
    dirname = args[1];
}

Line Cd::pull() {
    if(!dirname.empty()) {
        if(chdir(dirname.c_str()) != 0) {
            throw std::runtime_error("Could not cd.");
        }
        dirname.clear();
    }
    return Line{EOF_PIPE, ""};
}

Ls::Ls(const std::vector<std::string> &args, Process *previous) : Process(previous) {
    std::string dirname;
    if(args.size() == 1) {
        dirname = ".";
    } else if(args.size() == 2) {
        dirname = args[1];
    } else {
        throw std::logic_error("Ls takes one or zero arguments.");
    }
    readdir(dirname);
}

void Ls::readdir(const std::string &dirname) {
    std::unique_ptr<DIR, int(*)(DIR*)> dirholder(opendir(dirname.c_str()), closedir);
    if(!dirholder) {
        throw std::runtime_error("Could not open dir for reading.");
    }
    auto dir = dirholder.get();
    std::array<char, sizeof(dirent) + NAME_MAX + 1> buf;
    struct dirent *cur = reinterpret_cast<struct dirent*>(buf.data());
    struct dirent *de;
    std::string basename;
    while(readdir_r(dir, cur, &de) == 0 && de) {
        basename = cur->d_name;
        files.push_back(basename);
    }
    std::sort(files.begin(), files.end());
}

Line Ls::pull() {
    if(offset >= files.size()) {
        return Line{EOF_PIPE, ""};
    }
    return Line{STDOUT_PIPE, files[offset++]};
}

std::string get_cwd() {
    const size_t ARRSIZE = 512;
    char arr[ARRSIZE];
    getcwd(arr, ARRSIZE);
    return std::string(arr);
}


Grep::Grep(const std::vector<std::string> &args, Process *previous) : Process(previous) {
    if(args.size() != 2) {
        throw std::logic_error("Grep takes exactly one argument.");
    }
    pattern = args.back();
}

Line Grep::pull() {
    std::regex p(pattern); // inefficient
    while(true) {
        auto l = previous->pull();
        if(l.type == EOF_PIPE || l.type == STDERR_PIPE) {
            return l;
        }
        if(std::regex_search(l.text, p)) {
            return l;
        }
    }
}

// The world's most simple parser.
std::vector<std::vector<std::string>> parse(const std::string &line) {
    std::vector<std::vector<std::string>> result;
    std::vector<std::string> current_command;
    std::string current_word;
    for(const auto &c : line) {
        if(c == '|') {
            if(!current_word.empty()) {
                current_command.emplace_back(std::move(current_word));
                current_word.clear();
            }
            result.emplace_back(std::move(current_command));
            current_command.clear();
        } else if(c == ' ' || c == '\t') {
            if(!current_word.empty()) {
                current_command.emplace_back(current_word);
                current_word.clear();
            }
        } else {
            current_word.push_back(c);
        }
    }
    if(!current_word.empty()) {
        current_command.emplace_back(std::move(current_word));
    }
    if(!current_command.empty()) {
        result.emplace_back(std::move(current_command));
    }
    return result;
}

Sort::Sort(const std::vector<std::string> &args, Process *previous) : Process(previous) {
    if(args.size() != 1) {
        throw std::logic_error("Sort takes no arguments.");
    }
}

Line Sort::pull() {
    while(!depleted) {
        auto l = previous->pull();
        switch(l.type) {
        case STDERR_PIPE : return l;
        case STDOUT_PIPE : lines.emplace_back(std::move(l.text)); break;
        case EOF_PIPE : depleted = true; std::sort(lines.begin(), lines.end()); break;
        }
    }
    if(i >= lines.size()) {
        return Line{EOF_PIPE, ""};
    }
    return Line{STDOUT_PIPE, lines[i++]};
}

std::vector<std::unique_ptr<Process>> build_pipeline(const std::vector<std::vector<std::string>> &commands) {
    std::vector<std::unique_ptr<Process>> result;
    result.reserve(commands.size()+1);
    result.emplace_back(std::unique_ptr<Process>(new Empty()));
    for(const auto &s : commands) {
        if(s.empty()) {
            continue;
        }
        Process *previous = result.back().get();
        if(s[0] == "cat") {
            result.emplace_back(std::unique_ptr<Process>(new Cat(s, previous)));
        } else if(s[0] == "exit") {
            result.emplace_back(std::unique_ptr<Process>(new Exit(s, previous)));
        } else if(s[0] == "cd") {
            result.emplace_back(std::unique_ptr<Process>(new Cd(s, previous)));
        } else if(s[0] == "ls") {
            result.emplace_back(std::unique_ptr<Process>(new Ls(s, previous)));
        } else if(s[0] == "grep") {
            result.emplace_back(std::unique_ptr<Process>(new Grep(s, previous)));
        } else if(s[0] == "sort") {
            result.emplace_back(std::unique_ptr<Process>(new Sort(s, previous)));
        } else if(s[0] == "uniq") {
            result.emplace_back(std::unique_ptr<Process>(new Uniq(s, previous)));
        } else {
            throw std::logic_error("Unknown command: " + s[0]);
        }
    }
    return result;
}

Uniq::Uniq(const std::vector<std::string> &args, Process *previous) : Process(previous), prev_line("\n" ) {
    if(args.size() != 1) {
        throw std::logic_error("Uniq takes no arguments.");
    }
}

Line Uniq::pull() {
    while(true) {
        auto l = previous->pull();
        switch(l.type) {
        case STDERR_PIPE : return l;
        case STDOUT_PIPE : if(l.text != prev_line) { prev_line = l.text; return l;}; break;
        case EOF_PIPE : return l;
        }
    }
}

void run_pipeline(std::vector<std::unique_ptr<Process>> &pipeline) {
    if(pipeline.empty()) {
        return;
    }
    while(true) {
        auto l = pipeline.back()->pull();
        if(l.type == EOF_PIPE) {
            return;
        }
        printf("%s\n", l.text.c_str());
    }
}

void eval_loop()  {
    auto cwd = get_cwd();
    printf("%s\n$ ", cwd.c_str());
    fflush(stdout);
    std::string curline;
    std::getline(std::cin, curline);
    try {
        auto cmd_arr = parse(curline);
        auto pipeline = build_pipeline(cmd_arr);
        run_pipeline(pipeline);
    } catch(const ExitShell &) {
        throw;
    } catch(const std::exception &e) {
        printf("Error: %s\n", e.what());
    }
}

int main(int argc, char **argv) {
    printf(R"(Welcome to the experimental toy shell. It is just like a regular Unix shell but"
a lot more limited. You can only use a few commands (ls, grep etc) and they
have no options. Other things that don't work:

- shell or environment variables
- stream redirection other than '|'
- tab completion
- string quoting (only spaces count)
- most shell usability things apart from a working backspace

Run the "exit" command or hit ctrl-c to exit.

)");
    try {
        while(true) {
            eval_loop();
        }
    } catch(const ExitShell &) {
    }
    return 0;
}
