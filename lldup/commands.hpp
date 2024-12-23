//-------------------------------------------------------------------------------------------------
// File: Directory.h    Author: Dennis Lang
//
// Desc: This class is used to obtain the names of files in a directory.
//
// Usage::
//      Create a Directory_files object by providing the name of the directory
//      to use.  'next_file_name()' returns the next file name found in the
//      directory, if any.  You MUST check for the existance of more files
//      by using 'more_files()' between each call to "next_file_name()",
//      it tells you if there are more files AND sequences you to the next
//      file in the directory.
//
//      The normal usage will be something like this:
//          Directory_files dirfiles( dirName);
//          while (dirfiles.more_files())
//          {   ...
//              lstring filename = dirfiles.name();
//              ...
//          }
//-------------------------------------------------------------------------------------------------
//
// Author: Dennis Lang - 2024
// https://landenlabs.com
//
// This file is part of lldup project.
//
// ----- License ----
//
// Copyright (c) 2024  Dennis Lang
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is furnished to do
// so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#pragma once
#include "ll_stdhdr.hpp"

#include <vector>
#include <regex>
#include "lstring.hpp"

// Helper types
typedef std::vector<lstring> StringList;
typedef std::vector<std::regex> PatternList;
typedef unsigned int uint;
typedef std::vector<unsigned> IntList;

// ---------------------------------------------------------------------------
class Command {
public:
    // Runtime options
    PatternList includeFilePatList;
    PatternList excludeFilePatList;
    lstring DECRYPT_KEY;

    bool showFile = false;
    bool verbose = false;
    bool invert  = false;
    bool sameName = true;
    bool justName = false;
    bool ignoreExtn = false;

    // -- Duplicate file
    bool showSame = true;
    bool showDiff = false;
    bool showMiss = false;

    unsigned logfile = 0;   // 0=default show both, else only show file 1 or 2
    unsigned sameCnt = 0;
    unsigned diffCnt = 0;
    unsigned missCnt = 0;
    unsigned skipCnt = 0; // exludue and include filters rejected file.

    lstring separator = "\n";
    lstring preDivider = "";
    lstring postDivider = "\n__\n";
    
    lstring preDup = "==";
    lstring preMissing = "-- ";
    lstring preDiff = "!= ";

private:
    lstring none;
    char code;

public:
    Command(char c) : code(c) {
    }

    virtual  bool begin(StringList& fileDirList)  {
        return fileDirList.size() > 0;
    }

    virtual size_t add(const lstring& file) = 0;

    virtual bool end() {
        return true;
    }

    bool validFile(const lstring& name);

    Command& share(const Command& other) {
        includeFilePatList = other.includeFilePatList;
        excludeFilePatList = other.excludeFilePatList;
        DECRYPT_KEY = other.DECRYPT_KEY;
        showFile = other.showFile;
        verbose = other.verbose;
        invert = other.invert;
        sameName = other.sameName;
        justName = other.justName;
        ignoreExtn = other.ignoreExtn;
        separator = other.separator;
        preDivider = other.preDivider;
        postDivider = other.postDivider;
        return *this;
    }

    /*
    typedef size_t(*InspectFileFunc)(Command& command, const lstring& dirname);
    InspectFileFunc inspectFileFunc;

    size_t InspectFile(const lstring& dirName) {
        return (*inspectFileFunc)(*this, dirName, CmdState::run);
    }
    */

};

// ---------------------------------------------------------------------------
class DupDecode : public Command {
public:

    DupDecode() : Command('d') {}
    virtual size_t add(const lstring& file);
};

class DupFiles : public Command {
public:
    DupFiles() : Command('f') {}
    virtual  bool begin(StringList& fileDirList);
    virtual size_t add(const lstring& file);
    virtual bool end();

    void printPaths(const IntList& pathListIdx, const std::string& name);
};

class CompareAxxPair : public Command {
    lstring ref1Path, ref2Path;
public:
    CompareAxxPair() : Command('c') {}
    virtual  bool begin(StringList& fileDirList);
    virtual size_t add(const lstring& file);
};

