/*! -*-c++-*-
  @file   IndentingOStreamBuffer.h
  @author TODO
  @brief  Declaration of automatic ostream indentation

  http://stackoverflow.com/questions/9599807/how-to-add-indention-to-the-stream-operator

  \copyright Copyright 2014-2016 Elucideye, Inc. All rights reserved.
  \license{This project is released under the 3 Clause BSD License.}

*/

#ifndef __acf_IndentingOStreamBuffer_h__
#define __acf_IndentingOStreamBuffer_h__

#include <iostream>

class IndentingOStreambuf : public std::streambuf
{
    std::streambuf* myDest;
    bool myIsAtStartOfLine;
    std::string myIndent;
    std::ostream* myOwner;

protected:
    int overflow(int ch) override
    {
        if (myIsAtStartOfLine && ch != '\n')
        {
            myDest->sputn(myIndent.data(), myIndent.size());
        }
        myIsAtStartOfLine = ch == '\n';
        return myDest->sputc(ch);
    }

public:
    explicit IndentingOStreambuf(std::streambuf* dest, int indent = 4)
        : myDest(dest)
        , myIsAtStartOfLine(true)
        , myIndent(indent, ' ')
        , myOwner(nullptr)
    {
    }

    explicit IndentingOStreambuf(std::ostream& dest, int indent = 4)
        : myDest(dest.rdbuf())
        , myIsAtStartOfLine(true)
        , myIndent(indent, ' ')
        , myOwner(&dest)
    {
        myOwner->rdbuf(this);
    }
    ~IndentingOStreambuf() override
    {
        if (myOwner != nullptr)
        {
            myOwner->rdbuf(myDest);
        }
    }
};

#endif
