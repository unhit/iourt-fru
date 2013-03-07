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

#ifndef IP2LOCLOOKUP_H
#define IP2LOCLOOKUP_H

#include "Location.h"
#include <iostream>
#include <stdint.h>


class IP2LocLookup
{

 public:

  /**
   * These are the first 8 bytes of a compiled IP-to-location file, usually
   * called "ip2loc.bin".
   */
  static const uint64_t magicFileHeader = 0xd2f54b64dd0e09adull;

  /**
   * Loads an IP2LocLookup from a compiled IP-to-location database, usually
   * a file named "ip2loc.bin".  Returns NULL if loading was unsuccessful.
   * You need to call the destructor on the returned object when you are
   * finished using it.
   * If NULL is returned, the error string is set with an error message.
   * This function is safe in mission-critical environments.  There is no
   * possible way to hand-craft a malicious istream that causes a
   * buffer overrun or a huge memory allocation.
   */
  static const IP2LocLookup *load(std::istream &fileIn, std::string &error);

  /**
   * Returns the location for the input IP address.  This never returns NULL.
   */
  const Location *getLocationForIP(uint32_t ipAddr) const;

  /**
   * Returns the total number of Location objects.  Each Location has
   * an ID that is between 0 and numLocations() - 1, inclusive.
   */
  uint32_t numLocations() const;

  /**
   * This is similar to getLocationForIP(), except that only the ID
   * of the Location object is returned.  A Location object can later be
   * fetched based on this ID via a call to getLocation().  Having just
   * location IDs for IP addresses can be useful for bucketing locations,
   * for example.
   */
  uint16_t getLocationIDForIP(uint32_t ipAddr) const;

  /**
   * Returns a location for a given location ID.  The location ID
   * should be between 0 and numLocations() - 1, inclusive.  NULL is returned
   * if the location ID is out of this range.
   */
  const Location *getLocation(uint16_t locationID) const;

  /**
   * Fetches the IP address interval in which the input IP address belongs.
   * All IP addresses within that interval will map to the same Location.
   * The values ipAddrLower and ipAddrUpper are set to the lower and upper
   * inclusive bounds of the interval, respectively.
   */
  void getIPInterval(uint32_t ipAddrInput,
                     uint32_t &ipAddrLower, uint32_t &ipAddrUpper) const;

  /**
   * Returns the time (in seconds elapsed since midnight Jan 1, 1970 UTC)
   * when this compiled IP-to-location database was generated.
   */
  uint32_t getTimestamp() const;

  /**
   * Returns the number of strings in the string table.  The string table
   * stores all of the strings for the Location objects.  These are the
   * country codes, countries, regions, and cities.  Each string has a
   * string ID.  A string ID is between 0 and numStrings() - 1, inclusive.
   * The string table is only necessary if you are interested in compressing
   * Location objects down to a small number of bytes by sharing the string
   * table ahead of time (or caching it).
   */
  uint32_t numStrings() const;

  /**
   * Returns the string from the string table for the given string ID.  The
   * string ID must be between 0 and numStrings() - 1, inclusive.  If the
   * string ID is out of this range, NULL is returned.  See description of
   * numStrings() for an explanation of the string table.
   */
  const std::string *getString(uint16_t stringID) const;

  /**
   * Destructor.  All Location objects that were returned by this IP2LocLookup
   * will be deleted as well.
   */
  ~IP2LocLookup();


 private:

  /**
   * Constructor used by load().
   */
  IP2LocLookup();

  /**
   * Returns the index (in ipTable) of the IP address interval where
   * specified IP address belongs.
   */
  uint32_t getIPIndex(uint32_t ipAddr) const;

  uint32_t timestamp;
  uint32_t strTableSize;
  std::string *strTable;
  uint32_t locTableSize;
  Location *locTable;
  uint32_t ipTableSize;
  uint32_t *ipTable;
  uint16_t *ipTableLocID;

};

#endif
