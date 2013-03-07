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

#ifndef LOCATION_H
#define LOCATION_H

#include <stdint.h>
#include <string>


class IP2LocLookup;


class Location
{

  friend class IP2LocLookup;


 public:

  double getLatitude() const;

  double getLongitude() const;

  const std::string *getCountryCode() const;

  const std::string *getCountry() const;

  const std::string *getRegion() const;

  const std::string *getCity() const;

  /**
   * Returns the string ID of the country code string in the parent
   * IP2LocLookup object's string table.
   */
  uint16_t getCountryCodeStringID() const;

  /**
   * Returns the string ID of the country string in the parent IP2LocLookup
   * object's string table.
   */
  uint16_t getCountryStringID() const;

  /**
   * Returns the string ID of the region string in the parent IP2LocLookup
   * object's string table.
   */
  uint16_t getRegionStringID() const;

  /**
   * Returns the string ID of the city string in the parent IP2LocLookup
   * object's string table.
   */
  uint16_t getCityStringID() const;

  /**
   * Returns the parent IP2LocLookup object from which this Location came.
   */
  const IP2LocLookup *getParentLookup() const;

  uint16_t getLocationID() const;


 private:

  /**
   * A default constructor exists so that an array of Location can be
   * created.
   */
  Location();

  uint16_t locationID;
  const IP2LocLookup *parentLookup;
  double latitude;
  double longitude;
  uint16_t ccInx;
  uint16_t countryInx;
  uint16_t regionInx;
  uint16_t cityInx;

};

#endif
