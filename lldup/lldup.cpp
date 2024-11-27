//-------------------------------------------------------------------------------------------------
//
//  lldup      Feb-2024       Dennis Lang
//
//  Find duplicate files - specialized using AxCrypt
//
//-------------------------------------------------------------------------------------------------
//
// Author: Dennis Lang - 2024
// http://landenlabs.com/
//
// This file is part of lldup project.
//
// ----- License ----
//
// Copyright (c) 2024 Dennis Lang
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

// 4291 - No matching operator delete found
#pragma warning( disable : 4291 )
#define _CRT_SECURE_NO_WARNINGS

// Project files
#include "ll_stdhdr.hpp"
#include "directory.hpp"
#include "split.hpp"
#include "commands.hpp"
#include "dupscan.hpp"
#include "colors.hpp"

#include <stdio.h>
#include <ctype.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <regex>
#include <exception>

using namespace std;

// Helper types
typedef std::vector<std::regex> PatternList;
typedef unsigned int uint;

uint optionErrCnt = 0;
uint patternErrCnt = 0;

#if defined(_WIN32) || defined(_WIN64)
    // const char SLASH_CHAR('\\');
    #include <assert.h>
    #define strncasecmp _strnicmp
    #if !defined(S_ISREG) && defined(S_IFMT) && defined(S_IFREG)
        #define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
    #endif
#else
    // const char SLASH_CHAR('/');
#endif


// ---------------------------------------------------------------------------
// Recurse over directories, locate files.
static size_t InspectFiles(Command& command, const lstring& dirname) {
    Directory_files directory(dirname);
    lstring fullname;

    size_t fileCount = 0;

    struct stat filestat;
    try {
        if (stat(dirname, &filestat) == 0 && S_ISREG(filestat.st_mode)) {
            fileCount += command.add(dirname);
            return fileCount;
        }
    } catch (exception ex) {
        // Probably a pattern, let directory scan do its magic.
    }

    while (directory.more()) {
        directory.fullName(fullname);
        if (directory.is_directory()) {
            fileCount += InspectFiles(command, fullname);
        } else if (fullname.length() > 0) {
            fileCount += command.add(fullname);
        }
    }

    return fileCount;
}

// ---------------------------------------------------------------------------
// Return compiled regular expression from text.
std::regex getRegEx(const char* value) {
    try {
        std::string valueStr(value);
        return std::regex(valueStr);
        // return std::regex(valueStr, regex_constants::icase);
    } catch (const std::regex_error& regEx) {
        std::cerr << regEx.what() << ", Pattern=" << value << std::endl;
    }

    patternErrCnt++;
    return std::regex("");
}

// ---------------------------------------------------------------------------
// Validate option matchs and optionally report problem to user.
bool ValidOption(const char* validCmd, const char* possibleCmd, bool reportErr = true) {
    // Starts with validCmd else mark error
    size_t validLen = strlen(validCmd);
    size_t possibleLen = strlen(possibleCmd);

    if ( strncasecmp(validCmd, possibleCmd, std::min(validLen, possibleLen)) == 0)
        return true;

    if (reportErr) {
        std::cerr << "Unknown option:'" << possibleCmd << "', expect:'" << validCmd << "'\n";
        optionErrCnt++;
    }
    return false;
}
// ---------------------------------------------------------------------------
// Convert special characters from text to binary.
static std::string& ConvertSpecialChar(std::string& inOut) {
    uint len = 0;
    int x, n;
    const char* inPtr = inOut.c_str();
    char* outPtr = (char*)inPtr;
    while (*inPtr) {
        if (*inPtr == '\\') {
            inPtr++;
            switch (*inPtr) {
            case 'n': *outPtr++ = '\n'; break;
            case 't': *outPtr++ = '\t'; break;
            case 'v': *outPtr++ = '\v'; break;
            case 'b': *outPtr++ = '\b'; break;
            case 'r': *outPtr++ = '\r'; break;
            case 'f': *outPtr++ = '\f'; break;
            case 'a': *outPtr++ = '\a'; break;
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
                sscanf(inPtr, "%3o%n", &x, &n);
                inPtr += n - 1;
                *outPtr++ = (char)x;
                break;
            case 'x':                                // hexadecimal
                sscanf(inPtr + 1, "%2x%n", &x, &n);
                if (n > 0) {
                    inPtr += n;
                    *outPtr++ = (char)x;
                    break;
                }
            // seep through
            default:
                throw( "Warning: unrecognized escape sequence" );
            case '\\':
            case '\?':
            case '\'':
            case '\"':
                *outPtr++ = *inPtr;
                break;
            }
            inPtr++;
        } else
            *outPtr++ = *inPtr++;
        len++;
    }

    inOut.resize(len);
    return inOut;;
}

// ---------------------------------------------------------------------------
void showHelp(const char* arg0) {
    const char* helpMsg = "  Dennis Lang v1.8 (landenlabs.com) " __DATE__ "\n\n"
        "_p_Des: 'Find duplicate files\n"
        "_p_Use: lldup [options] directories...   or  files\n"
        "\n"
        "_p_Options (only first unique characters required, options can be repeated): \n"
        "\n"
        // "   -compareAxx       ; Find pair of encrypted path1/foo-xls.axx and path2/foo-xls.axx \n"
        // "   -duplicateAxx     ; Find pair of encrypted in raw foo-xls.axx and foo.xls \n"
        // "   -file             ; Find duplicate files by name \n"
        "\n"
        //    "   -invert           ; Invert test output "
        "   -includefile=<filePattern>\n"
        "   -excludefile=<filePattern>\n"
        "   -verbose \n"
        
        "\n"
        "_p_Options:\n"
        "   -_y_showDiff           ; Show files that differ\n"
        "   -_y_showMiss           ; Show missing files \n"
        "   -_y_sameAll            ; Compare all files for matching hash \n"
        "   -_y_hideSame           ; Don't show duplicate files \n"
        "   -_y_justName           ; Match duplicate name only, not contents \n"
        //      "   -ignoreExtn         ; With -justName, also ignore extension \n"
        "   -_y_preDup=<text>      ; Prefix before duplicates, default nothing  \n"
        "   -_y_preDiff=<text>     ; Prefix before diffences, default: \"!= \"  \n"
        "   -_y_preMiss=<text>     ; Prefix before missing, default: \"--  \" \n"
        // "   -_y_preDivider=<text>  ; Pre group divider output before groups  \n"
        "   -_y_postDivider=<text> ; Divider for dup and diff, def: \"__\\n\"  \n"
        "   -_y_separator=<text>   ; Separator  \n"
        //        "   -ignoreHardlink   ; \n"
        //        "   -ignoreSymlink    ; \n"
        "   -_y_simple             ; Only filesm no pre or separators \n"
        "   -_y_log=[1|2]          ; Only show file 1 or 2 for Dup or Diff  \n"
        "\n"
        "_p_Examples: \n"
        "  Find file matches by name and hash value (fastest with only 2 dirs) \n"
        "   lldup  dir1 dir2/subdir  \n"
        "   lldup  -_y_showDiff     dir1 dir2/subdir  \n"
        "   lldup  -_y_hideSame -_y_showMiss -_y_showDiff dir1 dir2/subdir  \n"
        "  Find file matches by mtching hash value, slower than above, 1 or more dirs \n"
        "   lldup  -_y_showAll  dir1   dir2/subdir   dir3 \n"
        "   lldup  -_y_showAll  dir1 \n"
        //       "   lldup  -inv dir1 dir2/subdir dir3 \n"
        "  Change how output appears \n"
        "   lldup  -_y_sep=, 'dir1 dir2/subdir dir3\n"
        //       "   lldup   -just -ignore . | grep png | le -I=-  '-c=lr ' \n"
        "\n"
        "\n";

    std::cerr << Colors::colorize("\n_W_") << arg0 << Colors::colorize(helpMsg);
}

//-------------------------------------------------------------------------------------------------
// Get current date/time, format is YYYY-MM-DD.HH:mm:ss
const std::string currentDateTime(time_t& now) {
    now = time(0);
    struct tm tstruct;
    char buf[80];
    tstruct = *localtime(&now);
    // Visit http://en.cppreference.com/w/cpp/chrono/c/strftime
    strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);
    return buf;
}

// ---------------------------------------------------------------------------
int main(int argc, char* argv[]) {
    DupFiles dupFiles;
    DupDecode  dupDecode;
    CompareAxxPair compareAxxPair;

    Command* commandPtr = &dupFiles;
    StringList fileDirList;

    if (argc == 1) {
        showHelp(argv[0]);
    } else {
        bool doParseCmds = true;
        string endCmds = "--";
        for (int argn = 1; argn < argc; argn++) {
            if (*argv[argn] == '-' && doParseCmds) {
                lstring argStr(argv[argn]);
                Split cmdValue(argStr, "=", 2);
                if (cmdValue.size() == 2) {
                    lstring cmd = cmdValue[0];
                    lstring value = cmdValue[1];

                    switch (cmd[(unsigned)1]) {
                    case 'e':   // excludeFile=<pat>
                        if (ValidOption("excludefile", cmd + 1)) {
                            ReplaceAll(value, "*", ".*");
                            ReplaceAll(value, "?", ".");
                            commandPtr->excludeFilePatList.push_back(getRegEx(value));
                        }
                        break;
                    case 'i':   // includeFile=<pat>
                        if (ValidOption("includefile", cmd + 1)) {
                            // includeFile=<pat>
                            ReplaceAll(value, "*", ".*");
                            ReplaceAll(value, "?", ".");
                            commandPtr->includeFilePatList.push_back(getRegEx(value));
                        }
                        break;
                    case 'l':   // log=[1|2]
                        if (ValidOption("log", cmd + 1)) {
                            commandPtr->logfile = (unsigned)strtoul(value, nullptr, 10);
                            continue;
                        }
                        break;
                    case 'p':
                        if (ValidOption("postDivider", cmd + 1, false)) {
                            commandPtr->postDivider = ConvertSpecialChar(value);
                        } else if (ValidOption("preDivider", cmd + 1)) {
                            commandPtr->preDivider = ConvertSpecialChar(value);
                        } else if (ValidOption("preDuplicate", cmd + 1)) {
                            commandPtr->preDup = ConvertSpecialChar(value);
                        } else if (ValidOption("preDiffer", cmd + 1)) {
                            commandPtr->preDiff = ConvertSpecialChar(value);
                        } else if (ValidOption("preMissing", cmd + 1)) {
                            commandPtr->preMissing = ConvertSpecialChar(value);
                        }
                        break;
                    case 's':
                        if (ValidOption("separator", cmd + 1, false)) {
                            commandPtr->separator = ConvertSpecialChar(value);
                        }
                        break;

                    default:
                        std::cerr << Colors::colorize("Use -h for help.\n_Y_Unknown option _R_") << cmd << Colors::colorize("_X_\n");
                        optionErrCnt++;
                        break;
                    }
                } else {
                    switch (argStr[1]) {
                    case 'a':
                        if (ValidOption("all", argStr + 1)) {
                            commandPtr->sameName = false;
                            continue;
                        }
                        break;
                    case 'f': // duplicated files
                        if (ValidOption("files", argStr + 1)) {
                            commandPtr = &dupFiles.share(*commandPtr);
                            continue;
                        }
                        break;
                    case '?':
                        showHelp(argv[0]);
                        continue;
                    case 'h':
                            if (ValidOption("help", argStr + 1, false)) {
                                showHelp(argv[0]);
                                continue;
                            } else if (ValidOption("hideSame", argStr + 1, false)) {
                            commandPtr->showSame = false;
                            continue;
                        }
                        break;
                    case 'i':
                        if (ValidOption("invert", argStr + 1, false)) {
                            commandPtr->invert = true;
                            continue;
                        } else if (ValidOption("ignoreExtn", argStr + 1)) {
                            commandPtr->ignoreExtn = true;
                            continue;
                        }
                        break;
                    case 'j':
                        if (ValidOption("justName", argStr + 1)) {
                            commandPtr->justName = true;
                            continue;
                        }
                        break;
                    case 's':
                        if (ValidOption("showAll", argStr + 1, false)) {
                            commandPtr->sameName = false;
                            continue;
                        } else if (ValidOption("showDiff", argStr + 1, false)) {
                            commandPtr->showDiff = true;
                            continue;
                        } else if (ValidOption("showMiss", argStr + 1, false)) {
                            commandPtr->showMiss = true;
                            continue;
                        }  else if (ValidOption("showSame", argStr + 1, false)) {
                            commandPtr->showSame = true;
                            continue;
                        }  else if (ValidOption("simple", argStr + 1, false)) {
                            commandPtr->preDup = commandPtr->preDiff = "";
                            commandPtr->postDivider = "\n";
                            continue;
                        }
                        break;
                    case 'v':
                        commandPtr->verbose = true;
                        continue;
                    }

                    if (endCmds == argv[argn]) {
                        doParseCmds = false;
                    } else {
                        std::cerr << Colors::colorize("Use -h for help.\n_Y_Unknown option _R_") << argStr << Colors::colorize("_X_\n");
                        optionErrCnt++;
                    }
                }
            } else {
                // Store file directories
                fileDirList.push_back(argv[argn]);
            }
        }

        time_t startT;
        std::cerr << "_Start " << currentDateTime(startT) << std::endl;

        if (commandPtr->begin(fileDirList)) {

            if (patternErrCnt == 0 && optionErrCnt == 0 && fileDirList.size() != 0) {
                if (fileDirList.size() == 1 && fileDirList[0] == "-") {
                    string filePath;
                    while (std::getline(std::cin, filePath)) {
                        std::cerr << "  Files Checked=" << InspectFiles(*commandPtr, filePath) << std::endl;
                    }
                } else if (commandPtr->ignoreExtn || ! commandPtr->sameName || fileDirList.size() != 2) {
                    for (auto const& filePath : fileDirList) {
                        std::cerr << "  Files Checked=" << InspectFiles(*commandPtr, filePath) << std::endl;
                    }
                } else if (fileDirList.size() == 2) {
                    DupScan dupScan(*commandPtr);
                    StringSet nextDirList;
                    nextDirList.insert("");
                    unsigned level = 0;
                    while (dupScan.findDuplicates(level, fileDirList, nextDirList)) {
                        level++;
                    }
                    std::cerr << "_Levels=" << level
                        << " Dup=" << commandPtr->sameCnt
                        << " Diff=" << commandPtr->diffCnt
                        << " Miss=" << commandPtr->missCnt
                        << " Skip=" << commandPtr->skipCnt
                        << " Files=" << commandPtr->sameCnt + commandPtr->diffCnt + commandPtr->missCnt + commandPtr->skipCnt
                        << std::endl;
                }
            }

            commandPtr->end();
        }

        time_t endT;
        std::cerr << "_End " << currentDateTime(endT) << std::endl;
        std::cerr << "_Elapsed " << std::difftime(endT, startT) << " (sec)\n";
    }

    return 0;
}
