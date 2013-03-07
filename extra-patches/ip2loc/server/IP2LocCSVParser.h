/*
  Copyright (c) 2010, Rambetter
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:

  1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer. 
  2. Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in the
     documentation and/or other materials provided with the distribution. 
  3. The name of the author may not be used to endorse or promote products
     derived from this software without specific prior written permission. 

  THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
  IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
  OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
  NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef IP2LOCCSVPARSER_H
#define IP2LOCCSVPARSER_H

#include <iostream>
#include <stdint.h>
#include <string>
#include <vector>


class IP2LocCSVParser
{

 public:

  /**
   * No arguments are expected.  Reads standard in, which should be the
   * contents of the IP2Location(TM) file named
   * "IP-COUNTRY-REGION-CITY-LATITUDE-LONGITUDE.CSV".  Writes to standard
   * out a compiled version of this IP-to-location database.  The compiled
   * output is usually saved to a file called "ip2loc.bin".
   */
  static int main(int argc, char *argv[]);

  /**
   * Parses an entire IP2Location(TM) file named
   * "IP-COUNTRY-REGION-CITY-LATITUDE-LONGITUDE.CSV".  A sample line of
   * this file is given in the description of tokenizeLine().
   * Returns true if and only if parsing is successful; if parsing is not
   * successful, the parseError string is set to a human-readable description
   * of the parse error.
   *
   * No input vectors are cleared at the beginning of this function.
   * It is considered an error if any of the vectors are not empty at the
   * beginning of this call; results will be unpredictable if that is the case.
   * If parsing fails, there may be things lingering in the vectors when this
   * method returns.
   *
   * The vector stringTable is populated with all unique strings that are
   * used to make up a location description.  In particular, this is a set
   * of all unique country codes, country names, regions, and cities.  The
   * index of each string in stringTable is referred to as the string ID.
   * There will be no more than 0x10000 strings in this vector.  Each string
   * will have length no greater than 0xff.
   *
   * The vector locations is populated with all unique locations that the
   * IP-to-location database references.  The index of each string
   * in the vector is referred to as the location ID of the location.  The
   * strings all have length 16 and are encoded in the following manner:
   *
   *   - The first four bytes are a serialized floating point value
   *     representing the latitude of the location.  The serialization
   *     algorithm used is the one described in IOSers::writeDec32_20().
   *
   *   - The next four bytes are a serialized floating point value
   *     representing the longitude of the location.  Same format as latitude.
   *
   *   - The next two bytes define an unsigned integer (constructed using
   *     network byte order) which is the string ID of the country code for
   *     this location.
   *
   *   - The next two bytes define the string ID for the country name.
   *
   *   - The next two bytes define the string ID for the region.
   *
   *   - The last two bytes define the string ID for the city.
   *
   * There will be no more than 0x10000 strings in this vector.
   * There will be approximately 18,000 locations.
   *
   * The vectors ipAddresses and locationIDs are populated with IP address
   * interval information; The quantity in ipAddresses at each index
   * represents the lower inclusive bound of an IP address interval, and the
   * quantity in locationIDs for the corresponding index represents
   * the location ID for that IP address interval.  There will be
   * approximately 3,000,000 entries in each of these arrays (the two arrays
   * will have the same size).
   */
  static bool parseIP2LocCSV(std::istream &fileIn,
                             std::vector<std::string> &stringTable,
                             std::vector<std::string> &locations,
                             std::vector<uint32_t> &ipAddresses,
                             std::vector<uint16_t> &locationIDs,
                             std::string &parseError);

  /**
   * These are the first 8 bytes of a compiled IP-to-location file, usually
   * called "ip2loc.bin".
   */
  static const uint64_t magicFileHeader = 0xd2f54b64dd0e09adull;

  /**
   * Writes a compiled form of the IP-to-location database to supplied ostream.
   * The vector parameters should be the results of executing the operation
   * parseIP2LocCSV().  There will be almost no data integrity checking for
   * the input parameters; however, if the supplied parameters are the results
   * of the parseIP2LocCSV() operation, the data is guaranteed to have
   * proper integrity.  The file that is saved is typically called
   * "ip2loc.bin" and is approximately 20 megabytes in size.  Returns true
   * if and only if writing the complete data was successful.
   *
   * The format of the compiled form of the IP-to-location database is as
   * follows.  All quantities described are written in network byte order.
   *
   * - The first 8 bytes (64 bits) are 0xd2f54b64dd0e09ad (magicFileHeader).
   *
   * - A 16 bit (2 byte) unsigned integer quantity specifying the version
   *   number of this file format.  The current version is 1, so 1 is written.
   *
   * - A 32 bit unsigned integer quantity specifying the number of seconds
   *   elapsed since midnight on Jan 1, 1970 (UTC) at the time of this
   *   function call.  This timestamp can be used to distinguish between
   *   one compiled version and another, to some extent.  Note: the code in
   *   the implementation will need to be re-examined when 2038 nears.
   *
   * - A 32 bit unsigned integer quantity specifying the number of strings
   *   in the string table.  This quantity is no greater than 0x10000.
   *
   * - The strings themselves.  The number of strings written will be equal
   *   to the previous 32 bit unsigned integer quantity.  Each string is
   *   preceded by an 8 bit unsigned integer quantity which specifies the
   *   length of the string that follows.  The strings are not null-terminated.
   *   The strings are composed of ASCII characters (one byte per character).
   *   An implicit string ID for each string is defined to be the zero-based
   *   index of that string in this array of strings.
   *
   * - A 32 bit unsigned integer quantity specifying the number of locations.
   *   This number is no greater than 0x10000.
   *
   * - The locations themselves.  The number of locations written will be
   *   equal to the previous 32 bit unsigned integer quantity.  Each location
   *   is 16 bytes (128 bits) in length, and those bytes correspond to
   *   latitude, longitude, country code, country, region, and city.  Please
   *   see the description of parseIP2LocCSV() for a definition of the 16 byte
   *   location format.  An implicit location ID for each location is defined
   *   to be the zero-based index of that location in this array of locations.
   *
   * - A 32 bit unsigned integer quantity specifying the number of IP address
   *   intervals.
   *
   * - A sequence of 6 byte (48 bit) quantities, where each 6 byte quantity
   *   defines an IP address interval; the number of 6 byte quantities is
   *   equal to the previous 32 bit unsigned integer quantity.  I will now
   *   describe the format for a 6 byte IP address interval.  The first 4
   *   bytes specifies an IP address, which is the [inclusive] lower bound of
   *   the IP address interval.  The last two bytes define an unsigned 16 bit
   *   integer quantity which is the the location ID of this IP address
   *   interval.
   */
  static bool writeIP2LocBin(std::ostream &fileOut,
                             std::vector<std::string> &stringTable,
                             std::vector<std::string> &locations,
                             std::vector<uint32_t> &ipAddresses,
                             std::vector<uint16_t> &locationIDs);

  /**
   * Parses the specified line of input from the IP2Location(TM) file named
   * "IP-COUNTRY-REGION-CITY-LATITUDE-LONGITUDE.CSV".
   * The line is converted into 8 string tokens which are placed into
   * the tokens vector.  (The input vector is not emptied at the beginning
   * of this call.)  Returns true if and only if the line was successfully
   * parsed, which implies that 8 tokens were inserted into the vector.
   * If false is returned, some partial tokens may be lingering inside the
   * vector when this method returns.
   *
   * Example line:
   * "67141376","67141631","CA","CANADA","QUEBEC","-","33.7357","-118.2970"
   *
   * Tokens for the above line would be the eight bits of text inside the
   * quotes (minus the quotes themselves).  The special case is that "-"
   * is converted to the empty string.
   *
   * There cannot be white space directly before or after each quoted token.
   */
  static bool tokenizeLine(const std::string &line,
                           std::vector<std::string> &tokens);


  /**
   * Parses a string into an unsigned 32 bit integer.  Every character of
   * the input string must be a digit.  The first digit of the string may be
   * zero only if the total length of the string is 1.  Numbers that cannot
   * be represented as a 32 bit unsigned integer will not be parsed.  A
   * return value of false corresponds to a failure to parse (but the
   * parameter parsedVal may still be modified even if false is returned).
   */
  static bool parseUnsignedInt(const std::string &strInput,
                               uint32_t &parsedVal);

  /**
   * Converts the input string to uppercase.  The character encoding is assumed
   * to be ISO-8859-1 (latin-1).  Where fitting, high non-ASCII latin-1
   * letters converted to similar ASCII counterparts.  Newlines ('\n') and
   * punctuation are left alone,  but control characters (even '\r') are
   * converted to '?', as are high latin-1 non-ASCII characters that cannot
   * be converted to ASCII counterparts.
   */
  static void latin1ToAsciiUpper(std::string &strInput);

  /**
   * Tests whether the input string consists of zero or more digits, followed
   * by an optional period, followed by zero or more digits.  There must be at
   * least one digit in the string.  An optional minus sign may precede the
   * string.
   */
  static bool isSimpleFloat(const std::string &strInput);


 private:

  /**
   * Constructors for this class should never be called.
   */
  IP2LocCSVParser();

};

#endif
