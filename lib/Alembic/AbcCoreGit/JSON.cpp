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

#include <Alembic/AbcCoreGit/JSON.h>

namespace Alembic {
namespace AbcCoreGit {
namespace ALEMBIC_VERSION_NS {

JSONParser::JSONParser() :
    ok(false)
{
}

JSONParser::JSONParser(const std::string& jsonPathname, const std::string& jsonContents) :
    ok(false)
{
    parseString(jsonPathname, jsonContents);
}

JSONParser::JSONParser(const std::string& jsonPathname) :
    ok(false)
{
    parseFile(jsonPathname);
}

bool JSONParser::parseFile(const std::string& jsonPathname)
{
    std::ifstream jsonFile(jsonPathname.c_str());
    std::stringstream jsonBuffer;
    jsonBuffer << jsonFile.rdbuf();
    jsonFile.close();

    jsonBuffer.seekg(0, std::ios::end);
    size_t jsonSize = jsonBuffer.tellg();
    jsonBuffer.seekg(0, std::ios::beg);

    m_jsonCharBufferPtr.reset( new char[jsonSize + 1] );
    jsonBuffer.read( m_jsonCharBufferPtr.get(), jsonSize );
    m_jsonCharBufferPtr.get()[jsonSize] = '\0';

#ifdef JSON_IN_SITU_PARSING
    // In-situ parsing, decode strings directly in the source string. Source must be string.
    if (document.ParseInsitu(m_jsonCharBufferPtr.get()).HasParseError())
    {
        ABCA_THROW( "format error while parsing '" << jsonPathname << "': " << GetParseError_En(document.GetParseError()) );
        ok = false;
    }
#else
    // "normal" parsing, decode strings to new buffers. Can use other input stream via ParseStream().
    if (document.Parse(jsonBuffer.str().c_str()).HasParseError())
    {
        ABCA_THROW( "format error while parsing '" << jsonPathname << "': " << GetParseError_En(document.GetParseError()) );
        ok = false;
    }
#endif
    ok = true;

    return ok;
}

bool JSONParser::parseString(const std::string& jsonPathname, const std::string& jsonContents)
{
    std::stringstream jsonBuffer;

    jsonBuffer.str(jsonContents);

    size_t jsonSize = jsonContents.length();

    jsonBuffer.seekg(0, std::ios::beg);

    m_jsonCharBufferPtr.reset( new char[jsonSize + 1] );
    jsonBuffer.read( m_jsonCharBufferPtr.get(), jsonSize );
    m_jsonCharBufferPtr.get()[jsonSize] = '\0';

#ifdef JSON_IN_SITU_PARSING
    // In-situ parsing, decode strings directly in the source string. Source must be string.
    if (document.ParseInsitu(m_jsonCharBufferPtr.get()).HasParseError())
    {
        ABCA_THROW( "format error while parsing '" << jsonPathname << "': " << GetParseError_En(document.GetParseError()) );
        ok = false;
    }
#else
    // "normal" parsing, decode strings to new buffers. Can use other input stream via ParseStream().
    if (document.Parse(jsonContents.c_str()).HasParseError())
    {
        ABCA_THROW( "format error while parsing '" << jsonPathname << "': " << GetParseError_En(document.GetParseError()) );
        ok = false;
    }
#endif
    ok = true;

    return ok;
}

} // End namespace ALEMBIC_VERSION_NS
} // End namespace AbcCoreGit
} // End namespace Alembic
