//-------------------------------------------------------------------------------------------------
//
//  lldup      Feb-2024       Dennis Lang
//
//  Find duplicate files - specialized using AxCrypt
//
//-------------------------------------------------------------------------------------------------
//
// Author: Dennis Lang - 2024
// https://landenlabs.com/
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
#include "signals.hpp"
#include "parseutil.hpp"
#include "commands.hpp"
#include "directory.hpp"
#include "dupscan.hpp"

#include <assert.h>
#include <iostream>
#include <exception>
#if 0
#include <stdio.h>
#include <ctype.h>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <map>
#include <set>
#include <algorithm>
#endif

// Helper types
typedef unsigned int uint;

#if defined(_WIN32) || defined(_WIN64)
    #define strncasecmp _strnicmp
#else
    // const char SLASH_CHAR('/');
#endif


// ---------------------------------------------------------------------------
// Search directories, locate files.
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

    while (!Signals::aborted && directory.more()) {
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
void showHelp(const char* arg0) {
    const char* helpMsg = "  Dennis Lang v3.4 (landenlabs.com) " __DATE__ "\n\n"
        "_p_Des: 'Find duplicate files\n"
        "_p_Use: lldup [options] directories...   or  files\n"
        "        This program has been replaced with _p_lldupdir \n"
        "\n"
        "_p_Options (only first unique characters required, options can be repeated): \n"
        "\n"
        // "   -compareAxx       ; Find pair of encrypted path1/foo-xls.axx and path2/foo-xls.axx \n"
        // "   -duplicateAxx     ; Find pair of encrypted in raw foo-xls.axx and foo.xls \n"
        // "   -file             ; Find duplicate files by name \n"
        "\n"
        //    "   -invert           ; Invert test output "
        "   -includeFile=<filePattern>\n"
        "   -excludeFile=<filePattern>\n"
        "   -verbose \n"
        "\n"
        "_p_Options:\n"
        "   -_y_showAll           ; Show files that differ\n"
        "   -_y_showDiff           ; Show files that differ\n"
        "   -_y_showMiss           ; Show missing files \n"
        "   -_y_hideDup            ; Don't show duplicate files \n"
        "\n"
        "   -_y_allFiles           ; Compare all files for matching hash \n"
        "   -_y_justName           ; Match duplicate name only, not contents \n"
        "   -_y_ignoreExtn            ; With -justName, also ignore extension \n"
        "\n"
        "   -_y_preDup=<text>      ; Prefix before duplicates, default: \"==\"  \n"
        "   -_y_preDiff=<text>     ; Prefix before differences, default: \"!= \"  \n"
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
        "   lldup  -_y_hideDup -_y_showMiss -_y_showDiff dir1 dir2/subdir  \n"
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

// ---------------------------------------------------------------------------
int main(int argc, char* argv[]) {
    Signals::init();

    ParseUtil parser;
    DupFiles dupFiles;
    DupDecode  dupDecode;
    CompareAxxPair compareAxxPair;

    Command* commandPtr = &dupFiles;
    StringList fileDirList;
    lstring timeStr;

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

                    const char* cmdName = cmd + 1;
                    if (cmd.length() > 2 && *cmdName == '-')
                        cmdName++;  // allow -- prefix on commands
                    switch (*cmdName) {
                    case 'e':   // excludeFile=<pat>
                        parser.validPattern(commandPtr->excludeFilePatList, value, "excludeFile", cmdName);
                        break;
                    case 'i':   // includeFile=<pat>
                        parser.validPattern(commandPtr->includeFilePatList, value, "includeFile", cmdName);
                        break;
                    case 'l':   // log=[1|2]
                        if (parser.validOption("log", cmdName)) {
                            commandPtr->logfile = (unsigned)strtoul(value, nullptr, 10);
                        }
                        break;
                    case 'p':
                        if (parser.validOption("postDivider", cmdName, false)) {
                            commandPtr->postDivider = ParseUtil::convertSpecialChar(value);
                        } else if (parser.validOption("preDivider", cmdName)) {
                            commandPtr->preDivider = ParseUtil::convertSpecialChar(value);
                        } else if (parser.validOption("preDuplicate", cmdName)) {
                            commandPtr->preDup = ParseUtil::convertSpecialChar(value);
                        } else if (parser.validOption("preDiffer", cmdName)) {
                            commandPtr->preDiff = ParseUtil::convertSpecialChar(value);
                        } else if (parser.validOption("preMissing", cmdName)) {
                            commandPtr->preMissing = ParseUtil::convertSpecialChar(value);
                        }
                        break;
                    case 's':
                        if (parser.validOption("separator", cmdName, false)) {
                            commandPtr->separator = ParseUtil::convertSpecialChar(value);
                        }
                        break;

                    default:
                        parser.showUnknown(argStr);
                        break;
                    }
                } else {
                    const char* cmdName = argStr + 1;
                    if (argStr.length() > 2 && *cmdName == '-')
                        cmdName++;  // allow -- prefix on commands
                    switch (*cmdName) {
                    case 'a':
                        if (parser.validOption("allFiles", cmdName)) {
                            commandPtr->sameName = false;
                        }
                        break;
                    case 'f': // duplicated files
                        if (parser.validOption("files", cmdName)) {
                            commandPtr = &dupFiles.share(*commandPtr);
                        }
                        break;
                    case '?':
                        showHelp(argv[0]);
                        continue;
                    case 'h':
                        if (parser.validOption("help", cmdName, false)) {
                            showHelp(argv[0]);
                        } else if (parser.validOption("hideDup", cmdName)) {
                            commandPtr->showSame = false;
                        }
                        break;
                    case 'i':
                        if (parser.validOption("invert", cmdName, false)) {
                            commandPtr->invert = true;
                        } else if (parser.validOption("ignoreExtn", cmdName)) {
                            commandPtr->ignoreExtn = true;
                        }
                        break;
                    case 'j':
                        if (parser.validOption("justName", cmdName)) {
                            commandPtr->justName = true;
                        }
                        break;
                    case 's':
                        if (parser.validOption("showAll", cmdName, false)) {
                            commandPtr->showSame = commandPtr->showDiff = commandPtr->showMiss = true ;
                        } else if (parser.validOption("showDiff", cmdName, false)) {
                            commandPtr->showDiff = true;
                        } else if (parser.validOption("showMiss", cmdName, false)) {
                            commandPtr->showMiss = true;
                        }  else if (parser.validOption("showSame", cmdName, false)) {
                            commandPtr->showSame = true;
                        }  else if (parser.validOption("simple", cmdName)) {
                            commandPtr->preDup = commandPtr->preDiff = "";
                            commandPtr->postDivider = "\n";
                        }
                        break;
                    case 'v':
                        commandPtr->verbose = true;
                    default:
                        parser.showUnknown(argStr);
                    }

                    if (endCmds == argv[argn]) {
                        doParseCmds = false;
                    }
                }
            } else {
                // Store file directories
                fileDirList.push_back(argv[argn]);
            }
        }

        time_t startT;
        std::cerr << Colors::colorize("_G_ +Start ") << ParseUtil::fmtDateTime(timeStr, startT) << Colors::colorize("_X_\n");

        if (commandPtr->begin(fileDirList)) {

            if (parser.patternErrCnt == 0 && parser.optionErrCnt == 0 && fileDirList.size() != 0) {
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
        ParseUtil::fmtDateTime(timeStr, endT);
        std::cerr << Colors::colorize("_G_ +End ")
            << timeStr
            << ", Elapsed "
            << std::difftime(endT, startT)
            << Colors::colorize(" (sec)_X_\n");
    }

    return 0;
}
