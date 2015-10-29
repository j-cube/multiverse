/*****************************************************************************/
/*  multiverse - a next generation storage back-end for Alembic              */
/*                                                                           */
/*  Copyright 2015 J CUBE Inc. Tokyo, Japan.                                 */
/*                                                                           */
/*  Licensed under the Apache License, Version 2.0 (the "License");          */
/*  you may not use this file except in compliance with the License.         */
/*  You may obtain a copy of the License at                                  */
/*                                                                           */
/*      http://www.apache.org/licenses/LICENSE-2.0                           */
/*                                                                           */
/*  Unless required by applicable law or agreed to in writing, software      */
/*  distributed under the License is distributed on an "AS IS" BASIS,        */
/*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. */
/*  See the License for the specific language governing permissions and      */
/*  limitations under the License.                                           */
/*****************************************************************************/

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
