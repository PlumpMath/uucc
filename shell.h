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
    virtual Line pull() = 0;
    virtual ~Process() = default;
};

class Cat : public Process {

public:
    Cat(const char *fname);
    Cat(const std::string &fname) : Cat(fname.c_str()) {};
    Line pull() override;

private:

    std::vector<std::string> contents;
    size_t offset=0;
};
