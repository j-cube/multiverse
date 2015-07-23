/*****************************************************************************/
/*  multiverse - a next generation storage back-end for Alembic              */
/*  Copyright (C) 2015 J CUBE Inc. Tokyo, Japan. All Rights Reserved.        */
/*                                                                           */
/*  This program is free software: you can redistribute it and/or modify     */
/*  it under the terms of the GNU General Public License as published by     */
/*  the Free Software Foundation, either version 3 of the License, or        */
/*  (at your option) any later version.                                      */
/*                                                                           */
/*  This program is distributed in the hope that it will be useful,          */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of           */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            */
/*  GNU General Public License for more details.                             */
/*                                                                           */
/*  You should have received a copy of the GNU General Public License        */
/*  along with this program.  If not, see <http://www.gnu.org/licenses/>.    */
/*                                                                           */
/*    J CUBE Inc.                                                            */
/*    6F Azabu Green Terrace                                                 */
/*    3-20-1 Minami-Azabu, Minato-ku, Tokyo                                  */
/*    info@-jcube.jp                                                         */
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
