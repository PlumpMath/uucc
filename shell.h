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

#pragma once

#include<string>
#include<vector>

enum OutputType {
    STDOUT_PIPE,
    STDERR_PIPE,
    EOF_PIPE,
};

struct Line {
    OutputType type;
    std::string text;
};

class Process {
public:
    explicit Process(Process *previous) : previous(previous) {};
    virtual Line pull() = 0;
    virtual ~Process() = default;
protected:
    Process *previous;
};

class Empty : public Process {
public:
    Empty() : Process(nullptr) {};
    Line pull() override {
        return Line{EOF_PIPE, ""};
    }
};

class Cat : public Process {

public:
    Cat(const std::vector<std::string> &args, Process *previous);
    Line pull() override;

private:

    std::vector<std::string> contents;
    size_t offset=0;
};

class Exit : public Process {
public:
    Exit(const std::vector<std::string> &args, Process *previous) : Process(previous) {}
    Line pull() override;
};

class Cd : public Process {
public:
    Cd(const std::vector<std::string> &args, Process *previous);
    Line pull() override;
private:
    std::string dirname;
};

class Ls : public Process {
public:
    Ls(const std::vector<std::string> &args, Process *previous);
    Line pull() override;

private:
    void readdir(const std::string &dirname);
    std::vector<std::string> files;
    size_t offset = 0;
};

class Grep : public Process {
public:
    Grep(const std::vector<std::string> &args, Process *previous);
    Line pull() override;

private:
    std::string pattern;
};

class Sort : public Process {
public:
    Sort(const std::vector<std::string> &args, Process *previous);
    Line pull() override;

private:
    std::vector<std::string> lines;
    bool depleted = false;
    size_t i = 0;
};


class Uniq : public Process {
public:
    Uniq(const std::vector<std::string> &args, Process *previous);
    Line pull() override;

private:
    std::string prev_line;
};
