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

#include "IOSers.h"
#include "IP2LocCSVParser.h"
#include <cstdlib>
#include <ctime>
#include <map>
#include <sstream>


using namespace std;


int IP2LocCSVParser::main(int argc, char *argv[])
{
  if (argc != 1) {
    cerr << "No arguments expected.  Reads standard in, which should be an IP2Location(TM)" << endl;
    cerr << "file named \"IP-COUNTRY-REGION-CITY-LATITUDE-LONGITUDE.CSV\".  This program" << endl;
    cerr << "compiles this file and writes the compiled version to standard out.  Usually," << endl;
    cerr << "the compiled form of the IP-to-location database is saved as a file called" << endl;
    cerr << "\"ip2loc.bin\"." << endl;
    return EXIT_FAILURE;
  }
  vector<string> stringTable;
  vector<string> locations;
  vector<uint32_t> ipAddresses;
  vector<uint16_t> locationIDs;
  string parseError;
  if (!parseIP2LocCSV(cin, stringTable, locations, ipAddresses, locationIDs,
                      parseError)) {
    cerr << parseError << endl;
    return EXIT_FAILURE;
  }
  if (!writeIP2LocBin(cout, stringTable, locations, ipAddresses,
                      locationIDs)) {
    cerr << "Enountered error writing to output" << endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}


bool IP2LocCSVParser::parseIP2LocCSV(istream &fileIn,
                                     vector<string> &stringTable,
                                     vector<string> &locations,
                                     vector<uint32_t> &ipAddresses,
                                     vector<uint16_t> &locationIDs,
                                     string &parseError)
{
  uint32_t lineNum = 1;
  char lineBuff[1024];
  string line;
  vector<string> tokens;
  uint32_t prevIPAddress = 0xfffffffful;
  map<string, uint16_t> stringSet;
  stringstream locationKeyBuff(stringstream::out);
  string locationKey;
  map<string, uint16_t> locationSet;
  stringstream errorStringOut(stringstream::out);

  while (true) {

    //////////////////////////////////////////////////////
    // Read a line of input and detect error conditions //
    //////////////////////////////////////////////////////

    fileIn.getline(lineBuff, sizeof(lineBuff));
    streamsize lineLength = fileIn.gcount(); // Includes the '\n' read.
    if (fileIn.eof()) {
      if (lineLength == 0) { break; }
      errorStringOut << "Premature end of file line " << lineNum;
      parseError = errorStringOut.str(); return false;
    }
    if (fileIn.fail()) {
      if (lineLength == sizeof(lineBuff) - 1) {
        errorStringOut << "Line " << lineNum << " is too long";
      }
      else {
        errorStringOut << "Unknown stream problem while reading file line ";
        errorStringOut << lineNum;
      }
      parseError = errorStringOut.str(); return false;
    }
    if (lineNum > 5000000) {
      errorStringOut << "Too many lines in file (too many IP intervals)";
      parseError = errorStringOut.str(); return false;
    }
    lineLength--; // Now does not include the trailing '\n'.
    line.assign(lineBuff, lineLength);

    ////////////////////////////////////////////
    // Strip '\r' off end of line, if present //
    ////////////////////////////////////////////

    if (lineLength > 0 && line[lineLength - 1] == '\r') {
      line.erase(--lineLength);
    }

    ///////////////////
    // Tokenize line //
    ///////////////////

    if (!tokenizeLine(line, tokens)) {
      errorStringOut << "Failed to tokenize line " << lineNum;
      parseError = errorStringOut.str(); return false;
    }

    ////////////////////////
    // Parse IP addresses //
    ////////////////////////

    uint32_t beginIPAddress;
    uint32_t endIPAddress;
    if (!parseUnsignedInt(tokens[0], beginIPAddress)) {
      errorStringOut << "Failed to parse begin IP address \"";
      errorStringOut << tokens[0] << "\" on line " << lineNum;
      parseError = errorStringOut.str(); return false;
    }
    if (!parseUnsignedInt(tokens[1], endIPAddress)) {
      errorStringOut << "Failed to parse end IP address \"";
      errorStringOut << tokens[1] << "\" on line " << lineNum;
      parseError = errorStringOut.str(); return false;
    }
    if (beginIPAddress != prevIPAddress + 1) {
      errorStringOut << "Begin IP address not one greater than previous ";
      errorStringOut << "end IP address on line " << lineNum;
      parseError = errorStringOut.str(); return false;
    }
    if (endIPAddress < beginIPAddress) {
      errorStringOut << "Begin IP address is greater than end IP address ";
      errorStringOut << "on line " << lineNum;
      parseError = errorStringOut.str(); return false;
    }

    //////////////////////////////////
    // Parse latitude and longitude //
    //////////////////////////////////

    if (!isSimpleFloat(tokens[6])) {
      errorStringOut << "Failed to parse latitude \"" << tokens[6];
      errorStringOut << "\" on line " << lineNum;
      parseError = errorStringOut.str(); return false;
    }
    if (!isSimpleFloat(tokens[7])) {
      errorStringOut << "Failed to parse longitude \"" << tokens[7];
      errorStringOut << "\" on line " << lineNum;
      parseError = errorStringOut.str(); return false;
    }
    const double latitude = atof(tokens[6].c_str());
    const double longitude = atof(tokens[7].c_str());
    if (!(latitude >= -90.0 && latitude <= 90.0)) {
      errorStringOut << "Latitude " << latitude << " is out of range on line ";
      errorStringOut << lineNum;
      parseError = errorStringOut.str(); return false;
    }
    if (!(longitude >= -180.0 && longitude <= 180.0)) {
      errorStringOut << "Longitude " << longitude;
      errorStringOut << " is out of range on line " << lineNum;
      parseError = errorStringOut.str(); return false;
    }

    ////////////////////////////////////////////////////////////////////////
    // Convert country code, country, region, and city to ascii uppercase //
    ////////////////////////////////////////////////////////////////////////

    latin1ToAsciiUpper(tokens[2]);
    latin1ToAsciiUpper(tokens[3]);
    latin1ToAsciiUpper(tokens[4]);
    latin1ToAsciiUpper(tokens[5]);

    ///////////////////////////////////////////////
    // Determine string IDs for the four strings //
    ///////////////////////////////////////////////

    uint16_t strIDs[4]; // country code, country, region, city
    for (uint8_t i = 0; i < 4; i++) {
      if (stringSet.count(tokens[i + 2]) == 0) {
        if (tokens[i + 2].length() > 0xffu) {
          errorStringOut << "Max string length exceeded line " << lineNum;
          parseError = errorStringOut.str(); return false;
        }
        if (stringSet.size() > 0xffffu) {
          errorStringOut << "Too many distinct strings";
          parseError = errorStringOut.str(); return false;
        }
        strIDs[i] = (uint16_t) stringSet.size();
        stringSet[tokens[i + 2]] = strIDs[i];
        stringTable.push_back(tokens[i + 2]);
      }
      else {
        strIDs[i] = stringSet[tokens[i + 2]];
      }
    }

    ////////////////////////
    // Build location key //
    ////////////////////////

    IOSers::writeDec32_20(locationKeyBuff, latitude);
    IOSers::writeDec32_20(locationKeyBuff, longitude);
    for (uint8_t i = 0; i < 4; i++) {
      IOSers::writeUInt16(locationKeyBuff, strIDs[i]);
    }
    locationKey = locationKeyBuff.str();

    ///////////////////////////////////////
    // Find location ID for location key //
    ///////////////////////////////////////

    uint16_t locationID;
    if (locationSet.count(locationKey) == 0) {
      if (locations.size() > 0xffffu) {
        errorStringOut << "Too many distinct locations";
        parseError = errorStringOut.str(); return false;
      }
      locationID = (uint16_t) locations.size();
      locationSet[locationKey] = locationID;
      locations.push_back(locationKey);
    }
    else {
      locationID = locationSet[locationKey];
    }

    /////////////////////////////////////////////////
    // Finally, add the IP address and location ID //
    /////////////////////////////////////////////////

    ipAddresses.push_back(beginIPAddress);
    locationIDs.push_back(locationID);

    ///////////////////////////////////////////
    // Clear and increment for the next line //
    ///////////////////////////////////////////

    lineNum++;
    tokens.clear();
    prevIPAddress = endIPAddress;
    locationKeyBuff.clear();
    locationKeyBuff.str(string());
    locationKey.clear();

  }

  if (fileIn.bad()) {
    errorStringOut << "Failed to read full input";
    parseError = errorStringOut.str(); return false;
  }
  if (ipAddresses.size() == 0) {
    errorStringOut << "Input file is empty";
    parseError = errorStringOut.str(); return false;
  }
  if (prevIPAddress != 0xffffffffu) {
    errorStringOut << "Last IP interval does not end at 255.255.255.255";
    parseError = errorStringOut.str(); return false;
  }
  return true;
}


bool IP2LocCSVParser::writeIP2LocBin(ostream &fileOut,
                                     vector<string> &stringTable,
                                     vector<string> &locations,
                                     vector<uint32_t> &ipAddresses,
                                     vector<uint16_t> &locationIDs)
{
  static const uint16_t IP2LOC_BIN_VERSION = 1;
  if (!IOSers::writeUInt64(fileOut, magicFileHeader)) { return false; }
  if (!IOSers::writeUInt16(fileOut, IP2LOC_BIN_VERSION)) { return false; }
  if (!IOSers::writeUInt32(fileOut, (uint32_t) time(NULL))) { return false; }
  if (!IOSers::writeUInt32(fileOut, (uint32_t) stringTable.size())) {
    return false;
  }
  vector<string>::iterator strIter = stringTable.begin();
  while (strIter < stringTable.end()) {
    if (!IOSers::writeUInt8(fileOut, (uint8_t) (*strIter).length())) {
      return false;
    }
    fileOut.write((*strIter).data(), (streamsize) (*strIter).length());
    if (fileOut.bad()) { return false; }
    strIter++;
  }
  if (!IOSers::writeUInt32(fileOut, (uint32_t) locations.size())) {
    return false;
  }
  strIter = locations.begin();
  while (strIter < locations.end()) {
    fileOut.write((*strIter).data(), 16);
    if (fileOut.bad()) { return false; }
    strIter++;
  }
  if (!IOSers::writeUInt32(fileOut, (uint32_t) ipAddresses.size())) {
    return false;
  }
  vector<uint32_t>::iterator ipAddressesIter = ipAddresses.begin();
  vector<uint16_t>::iterator locationIDsIter = locationIDs.begin();
  while (ipAddressesIter < ipAddresses.end()) {
    if (!(IOSers::writeUInt32(fileOut, *ipAddressesIter) &&
          IOSers::writeUInt16(fileOut, *locationIDsIter))) { return false; }
    ipAddressesIter++;
    locationIDsIter++;
  }
  fileOut.flush();
  return !fileOut.bad();
}


bool IP2LocCSVParser::tokenizeLine(const string &line, vector<string> &tokens)
{
  size_t beginInx = 0;
  string token;
  for (int8_t i = 1; i <= 8; i++) {
    if (line.find('\"', beginInx) != beginInx) { return false; }
    size_t inx = line.find('\"', ++beginInx);
    if (inx == string::npos) { return false; }
    token = line.substr(beginInx, inx - beginInx);
    if (token == "-") { token = ""; }
    tokens.push_back(token);
    if (i <= 7) {
      inx++;
      if (line.find(',', inx) != inx) { return false; }
    }
    beginInx = inx + 1;
  }
  if (beginInx != line.length()) { return false; }
  return true;
}


bool IP2LocCSVParser::parseUnsignedInt(const string &strInput,
                                       uint32_t &parsedVal)
{
  static const uint32_t powBase10[] = { 1ul,
                                        10ul,
                                        100ul,
                                        1000ul,
                                        10000ul,
                                        100000ul,
                                        1000000ul,
                                        10000000ul,
                                        100000000ul,
                                        1000000000ul };
  const size_t strLen = strInput.length();
  if (!(strLen > 0 && strLen <= 10)) { return false; }
  if (strLen > 1 && strInput[0] == '0') { return false; }
  parsedVal = 0;
  char ch;
  for (int8_t i = strLen - 1; i > 0; i--) {
    ch = strInput[i];
    if (!isdigit(ch)) { return false; }
    parsedVal += powBase10[strLen - i - 1] * (ch - '0');
  }
  ch = strInput[0];
  if (!isdigit(ch)) { return false; }
  if (strLen == 10) {
    if (ch > '4') { return false; }
    if (ch == '4' && parsedVal > 294967295ul) { return false; }
  }
  parsedVal += powBase10[strLen - 1] * (ch - '0');
  return true;
}


void IP2LocCSVParser::latin1ToAsciiUpper(string &strInput)
{
  static char latin1ToAsciiUpperMap[0x100];
  static bool charMapInitialized = false;
  if (!charMapInitialized) {
    uint16_t i;
    for (i = 0x00; i <= 0x1f; i++) { latin1ToAsciiUpperMap[i] = '?'; }
    latin1ToAsciiUpperMap[0x0a] = '\n';
    for (i = 0x20; i <= 0x60; i++) { latin1ToAsciiUpperMap[i] = (char) i; }
    for (i = 0x61; i <= 0x7a; i++) {
      latin1ToAsciiUpperMap[i] = (char) (i - 0x20);
    }
    for (i = 0x7b; i <= 0x7e; i++) { latin1ToAsciiUpperMap[i] = (char) i; }
    for (i = 0x7f; i <= 0xff; i++) { latin1ToAsciiUpperMap[i] = '?'; }
    for (i = 0xc0; i <= 0xc5; i++) { latin1ToAsciiUpperMap[i] = 'A'; }
    latin1ToAsciiUpperMap[0xc7] = 'C';
    for (i = 0xc8; i <= 0xcb; i++) { latin1ToAsciiUpperMap[i] = 'E'; }
    for (i = 0xcc; i <= 0xcf; i++) { latin1ToAsciiUpperMap[i] = 'I'; }
    latin1ToAsciiUpperMap[0xd1] = 'N';
    for (i = 0xd2; i <= 0xd6; i++) { latin1ToAsciiUpperMap[i] = 'O'; }
    for (i = 0xd9; i <= 0xdc; i++) { latin1ToAsciiUpperMap[i] = 'U'; }
    latin1ToAsciiUpperMap[0xdd] = 'Y';
    for (i = 0xe0; i <= 0xe5; i++) { latin1ToAsciiUpperMap[i] = 'A'; }
    latin1ToAsciiUpperMap[0xe7] = 'C';
    for (i = 0xe8; i <= 0xeb; i++) { latin1ToAsciiUpperMap[i] = 'E'; }
    for (i = 0xec; i <= 0xef; i++) { latin1ToAsciiUpperMap[i] = 'I'; }
    latin1ToAsciiUpperMap[0xf1] = 'N';
    for (i = 0xf2; i <= 0xf6; i++) { latin1ToAsciiUpperMap[i] = 'O'; }
    for (i = 0xf9; i <= 0xfc; i++) { latin1ToAsciiUpperMap[i] = 'U'; }
    latin1ToAsciiUpperMap[0xfd] = 'Y';
    latin1ToAsciiUpperMap[0xff] = 'Y';
    charMapInitialized = true;
  }
  for (size_t j = strInput.length(); j > 0; j--) {
    strInput[j - 1] = latin1ToAsciiUpperMap[0xff & strInput[j - 1]];
  }
}


bool IP2LocCSVParser::isSimpleFloat(const string &strInput)
{
  const size_t strLen = strInput.length();
  bool digitSeen = false;
  bool periodSeen = false;
  char ch;
  for (size_t i = 0; i < strLen; i++) {
    ch = strInput[i];
    if (ch == '-') {
      if (i != 0) { return false; }
    }
    else if (ch == '.') {
      if (periodSeen) { return false; }
      periodSeen = true;
    }
    else if (isdigit(ch)) { digitSeen = true; }
    else { return false; }
  }
  return digitSeen;
}


// Private constructor should never be called.
IP2LocCSVParser::IP2LocCSVParser() {}
