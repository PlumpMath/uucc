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
#include<memory>
#include<iostream>
#include<fstream>
#include<stdexcept>


Cat::Cat(const char *fname) {
   std::ifstream ifile(fname, std::ios::in);
   for(std::string l; std::getline(ifile, l);) {
       contents.emplace_back(std::move(l));
   }
}

Line Cat::pull() {
    if(offset >= contents.size()) {
        return Line{EOF_PIPE, ""};
    }
    return Line{STDOUT_PIPE, contents[offset++]};
}

std::string get_cwd() {
    const size_t ARRSIZE = 512;
    char arr[ARRSIZE];
    getcwd(arr, ARRSIZE);
    return std::string(arr);
}

std::vector<std::vector<std::string>> parse(const std::string &line) {
    std::vector<std::vector<std::string>> result;
    std::vector<std::string> one{"cat", "build.ninja"};
    result.push_back(one);
    return result;
}

std::vector<std::unique_ptr<Process>> build_pipeline(const std::vector<std::vector<std::string>> &commands) {
    std::vector<std::unique_ptr<Process>> result;
    for(const auto &s : commands) {
        if(s.at(0) == "cat") {
            result.emplace_back(std::unique_ptr<Process>(new Cat(s.at(1))));
        } else {
            throw std::logic_error("Unknown command: " + s.at(0));
        }
    }
    return result;
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
    auto cmd_arr = parse(curline);
    auto pipeline = build_pipeline(cmd_arr);
    run_pipeline(pipeline);
}


int main(int argc, char **argv) {
    try {
        while(true) {
            eval_loop();
        }
    } catch(const std::exception &e) {
        printf("%s\n", e.what());
    }
    return 0;
}
