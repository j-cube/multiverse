//-*****************************************************************************
//
// Copyright (c) 2013,
//  Sony Pictures Imageworks, Inc. and
//  Industrial Light & Magic, a division of Lucasfilm Entertainment Company Ltd.
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// *       Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// *       Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
// *       Neither the name of Sony Pictures Imageworks, nor
// Industrial Light & Magic nor the names of their contributors may be used
// to endorse or promote products derived from this software without specific
// prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//-*****************************************************************************

#include <Alembic/Abc/All.h>
#include <Alembic/AbcCoreFactory/All.h>
#include <Alembic/AbcCoreGit/All.h>
#include <Alembic/AbcCoreGit/ArImpl.h>
#include <Alembic/AbcCoreAbstract/All.h>

#include <boost/algorithm/string/predicate.hpp>

#include <iostream>

static void usage()
{
    printf ("Usage: abchistory file\n");
    printf ("Used to list modification history of an Alembic file (using the Git backend)\n\n");

    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
    std::string inFile;

    int arg_i = 1;

    if (argc != 2)
        usage();

    inFile = argv[arg_i++];

    Alembic::AbcCoreFactory::IOptions rOptions;

    Alembic::AbcCoreFactory::IFactory factory;
    Alembic::AbcCoreFactory::IFactory::CoreType coreType;
    Alembic::Abc::IArchive archive = factory.getArchive(inFile, coreType, rOptions);
    if (!archive.valid())
    {
        printf("Error: Invalid Alembic file specified: %s\n",
               inFile.c_str());
        return 1;
    }

    if (coreType != Alembic::AbcCoreFactory::IFactory::kGit)
    {
        printf("Warning: Alembic file specified: %s\n", inFile.c_str());
        printf("is not of the required Git type.\n");
        return 1;
    }

    bool error = false;
    std::string jsonHistory = Alembic::AbcCoreGit::getHistoryJSON(archive, error);

    std::cout << jsonHistory << std::endl;

    return 0;
}
