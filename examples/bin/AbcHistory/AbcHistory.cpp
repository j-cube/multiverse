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
    printf ("Usage: abchistory [-r | --read] FILE...\n");
    printf ("Used to list modification history of one or more Alembic files.\n\n");
    printf ("Specified files must be using the Git backend.\n");
    printf ("If -r is not provided the Alembic file is not parsed.\n");
    printf ("Options:\n\n");
    printf ("  -r | --read   Read and process the archive.\n");

    exit(EXIT_FAILURE);
}

static bool hasOption(const std::vector<std::string>& options, const std::string& option)
{
    std::vector<std::string>::const_iterator it;
    for (it = options.begin(); it != options.end(); ++it)
    {
        const std::string& it_option = *it;
        if (it_option == option)
            return true;
    }
    return false;
}

int main(int argc, char *argv[])
{
    if (argc < 2)
        usage();

    // parse args
    std::vector<std::string> arguments(argv, argv + argc);
    std::vector<std::string> options;
    std::vector<std::string> files;

    // separate file args from option args
    for (std::size_t i = 1; i < arguments.size(); i++)
    {
        if (arguments[i].substr(0, 1) == "-")
            options.push_back(arguments[i]);
        else
            files.push_back(arguments[i]);
    }

    if (files.size() < 1)
        usage();

    bool opt_read_archive = hasOption(options, "-r") || hasOption(options, "--read");

    std::vector<std::string>::const_iterator it;
    for (it = files.begin(); it != files.end(); ++it)
    {
        const std::string& file = *it;

        std::string jsonHistory;

        if (opt_read_archive)
        {
            Alembic::AbcCoreFactory::IOptions rOptions;

            Alembic::AbcCoreFactory::IFactory factory;
            Alembic::AbcCoreFactory::IFactory::CoreType coreType;
            Alembic::Abc::IArchive archive = factory.getArchive(file, coreType, rOptions);
            if (!archive.valid())
            {
                std::cerr << "ERROR: Invalid Alembic file specified: " << file << std::endl;
                exit(EXIT_FAILURE);
            }

            if (coreType != Alembic::AbcCoreFactory::IFactory::kGit)
            {
                std::cerr << "ERROR: Alembic file specified: " << file << " not of the required type (Git)." << std::endl;
                exit(EXIT_FAILURE);
            }

            bool error = false;
            jsonHistory = Alembic::AbcCoreGit::getHistoryJSON(archive, error);
        } else
        {
            bool error = false;
            jsonHistory = Alembic::AbcCoreGit::getHistoryJSON(file, error);
        }

        std::cout << "Alembic file '" << file << "' (JSON) history:" << std::endl;
        std::cout << jsonHistory << std::endl;
    }

    exit(EXIT_SUCCESS);
}
