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

#include "Location.h"
#include "IP2LocLookup.h"


using namespace std;


Location::Location() {}


double Location::getLatitude() const
{
  return latitude;
}


double Location::getLongitude() const
{
  return longitude;
}


const string *Location::getCountryCode() const
{
  return parentLookup->getString(ccInx);
}


const string *Location::getCountry() const
{
  return parentLookup->getString(countryInx);
}


const string *Location::getRegion() const
{
  return parentLookup->getString(regionInx);
}


const string *Location::getCity() const
{
  return parentLookup->getString(cityInx);
}


uint16_t Location::getCountryCodeStringID() const
{
  return ccInx;
}


uint16_t Location::getCountryStringID() const
{
  return countryInx;
}


uint16_t Location::getRegionStringID() const
{
  return regionInx;
}


uint16_t Location::getCityStringID() const
{
  return cityInx;
}


const IP2LocLookup *Location::getParentLookup() const
{
  return parentLookup;
}


uint16_t Location::getLocationID() const
{
  return locationID;
}
