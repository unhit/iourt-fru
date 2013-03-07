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

#include "IP2LocLookup.h"
#include "IOSers.h"
#include <sstream>


using namespace std;


const IP2LocLookup *IP2LocLookup::load(istream &fileIn, string &error)
{
  uint64_t magicHeader;
  if (!IOSers::readUInt64(fileIn, magicHeader)) {
    error = "Could not read 64 bit header"; return NULL;
  }
  if (magicHeader != IP2LocLookup::magicFileHeader) {
    error = "Incorrect header"; return NULL;
  }
  uint16_t version;
  if (!IOSers::readUInt16(fileIn, version)) {
    error = "Could not read 16 bit version number"; return NULL;
  }
  if (version != 1) {
    error = "Unrecognized version numer"; return NULL;
  }
  IP2LocLookup *lookup = new IP2LocLookup();
  if (!IOSers::readUInt32(fileIn, lookup->timestamp)) {
    error = "Could not read 32 bit timestamp";
    delete lookup; return NULL;
  }
  uint32_t strTblSize;
  if (!IOSers::readUInt32(fileIn, strTblSize)) {
    error = "Could not read 32 bit string table size";
    delete lookup; return NULL;
  }
  if (strTblSize > 0x10000) {
    error = "String table too large";
    delete lookup; return NULL;
  }
  lookup->strTableSize = strTblSize;
  lookup->strTable = new string[strTblSize];
  char strBuff[0xff];
  for (uint32_t strInx = 0; strInx < strTblSize; strInx++) {
    uint8_t strLen;
    if (!IOSers::readUInt8(fileIn, strLen)) {
      error = "Could not read 8 bit length of string";
      delete lookup; return NULL;
    }
    fileIn.read(strBuff, strLen);
    if (fileIn.fail()) {
      error = "Failed to read string";
      delete lookup; return NULL;
    }
    (lookup->strTable[strInx]).assign(strBuff, strLen);
  }
  uint32_t locTblSize;
  if (!IOSers::readUInt32(fileIn, locTblSize)) {
    error = "Could not read 32 bit location table size";
    delete lookup; return NULL;
  }
  if (locTblSize > 0x10000) {
    error = "Location table too large";
    delete lookup; return NULL;
  }
  lookup->locTableSize = locTblSize;
  lookup->locTable = new Location[locTblSize];
  for (uint32_t locInx = 0; locInx < locTblSize; locInx++) {
    lookup->locTable[locInx].locationID = locInx;
    lookup->locTable[locInx].parentLookup = lookup;
    long double d;
    if (!IOSers::readDec32_20(fileIn, d)) {
      error = "Failed to read latitude";
      delete lookup; return NULL;
    }
    if (!(-180.0 <= d && d <= 180.0)) {
      error = "Latitude out of range";
      delete lookup; return NULL;
    }
    lookup->locTable[locInx].latitude = d;
    if (!IOSers::readDec32_20(fileIn, d)) {
      error = "Failed to read longitude";
      delete lookup; return NULL;
    }
    if (!(-180.0 <= d && d <= 180.0)) {
      error = "Longitude out of range";
      delete lookup; return NULL;
    }
    lookup->locTable[locInx].longitude = d;
    uint16_t strInx;
    if (!IOSers::readUInt16(fileIn, strInx)) {
      error = "Failed to read country code string index";
      delete lookup; return NULL;
    }
    if (strInx >= strTblSize) {
      error = "Country code string index out of range";
      delete lookup; return NULL;
    }
    lookup->locTable[locInx].ccInx = strInx;
    if (!IOSers::readUInt16(fileIn, strInx)) {
      error = "Failed to read country string index";
      delete lookup; return NULL;
    }
    if (strInx >= strTblSize) {
      error = "Country string index out of range";
      delete lookup; return NULL;
    }
    lookup->locTable[locInx].countryInx = strInx;
    if (!IOSers::readUInt16(fileIn, strInx)) {
      error = "Failed to read region string index";
      delete lookup; return NULL;
    }
    if (strInx >= strTblSize) {
      error = "Region string index out of range";
      delete lookup; return NULL;
    }
    lookup->locTable[locInx].regionInx = strInx;
    if (!IOSers::readUInt16(fileIn, strInx)) {
      error = "Failed to read city string index";
      delete lookup; return NULL;
    }
    if (strInx >= strTblSize) {
      error = "City string index out of range";
      delete lookup; return NULL;
    }
    lookup->locTable[locInx].cityInx = strInx;
  }
  uint32_t ipTblSize;
  if (!IOSers::readUInt32(fileIn, ipTblSize)) {
    error = "Could not read 32 bit IP table size";
    delete lookup; return NULL;
  }
  if (ipTblSize > 5000000) {
    error = "IP table too large";
    delete lookup; return NULL;
  }
  lookup->ipTableSize = ipTblSize;
  lookup->ipTable = new uint32_t[ipTblSize];
  lookup->ipTableLocID = new uint16_t[ipTblSize];
  uint32_t prevIP = 0xffffffff;
  for (uint32_t ipInx = 0; ipInx < ipTblSize; ipInx++) {
    uint32_t ipAddr;
    uint16_t locID;
    if (!(IOSers::readUInt32(fileIn, ipAddr) &&
          IOSers::readUInt16(fileIn, locID))) {
      error = "Failed to read IP entry";
      delete lookup; return NULL;
    }
    if (ipInx != 0 && prevIP == 0xffffffff) {
      error = "IP address wrapped";
      delete lookup; return NULL;
    }
    if (!(ipAddr >= prevIP + 1)) {
      error = "IP address not bigger than previous";
      delete lookup; return NULL;
    }
    if (locID >= locTblSize) {
      error = "Location ID out of range";
      delete lookup; return NULL;
    }
    lookup->ipTable[ipInx] = ipAddr;
    lookup->ipTableLocID[ipInx] = locID;
    prevIP = ipAddr;
  }
  return lookup;
}


IP2LocLookup::IP2LocLookup()
{
  // For clarity.
  strTable = NULL;
  locTable = NULL;
  ipTable = NULL;
  ipTableLocID = NULL;
}


IP2LocLookup::~IP2LocLookup()
{
  if (strTable != NULL) { delete[] strTable; strTable = NULL; }
  if (locTable != NULL) { delete[] locTable; locTable = NULL; }
  if (ipTable != NULL) { delete[] ipTable; ipTable = NULL; }
  if (ipTableLocID != NULL) { delete[] ipTableLocID; ipTableLocID = NULL; }
}


const Location *IP2LocLookup::getLocationForIP(uint32_t ipAddr) const
{
  return &(locTable[ipTableLocID[getIPIndex(ipAddr)]]);
}


uint32_t IP2LocLookup::numLocations() const
{
  return locTableSize;
}


uint16_t IP2LocLookup::getLocationIDForIP(uint32_t ipAddr) const
{
  return ipTableLocID[getIPIndex(ipAddr)];
}


const Location *IP2LocLookup::getLocation(uint16_t locationID) const
{
  if (locationID >= locTableSize) { return NULL; }
  return &(locTable[locationID]);
}


void IP2LocLookup::getIPInterval(uint32_t ipAddrInput,
                                 uint32_t &ipAddrLower,
                                 uint32_t &ipAddrUpper) const
{
  uint32_t ipInx = getIPIndex(ipAddrInput);
  ipAddrLower = ipTable[ipInx];
  ipAddrUpper =
    (ipInx + 1 < ipTableSize) ? (ipTable[ipInx + 1] - 1) : 0xffffffff;
}


uint32_t IP2LocLookup::getTimestamp() const
{
  return timestamp;
}


uint32_t IP2LocLookup::numStrings() const
{
  return strTableSize;
}


const string *IP2LocLookup::getString(uint16_t stringID) const
{
  if (stringID >= strTableSize) { return NULL; }
  return &(strTable[stringID]);
}


uint32_t IP2LocLookup::getIPIndex(uint32_t ipAddr) const
{
  uint32_t low = 0;
  uint32_t high = ipTableSize;
  while (true) {
    // No overflow because ipTableSize is bounded.
    uint32_t mid = (high + low) / 2;
    uint32_t lowerIncl = ipTable[mid];
    uint32_t upperIncl =
      (mid + 1 < ipTableSize) ? (ipTable[mid + 1] - 1) : 0xffffffff;
    if (ipAddr >= lowerIncl) {
      if (ipAddr <= upperIncl) { return mid; }
      else { // ipAddr > upperIncl
        low = mid;
      }
    }
    else { // ipAddr < lowerIncl
      high = mid;
    }
  }
}
