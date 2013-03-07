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
#include <arpa/inet.h>


using namespace std;


typedef union {
  uint8_t bits;
  char bytes[];
} uint8_bytes;


typedef union {
  uint16_t bits;
  char bytes[];
} uint16_bytes;


typedef union {
  uint32_t bits;
  char bytes[];
} uint32_bytes;


bool IOSers::writeUInt8(ostream &out, const uint8_t val)
{
  uint8_bytes valb;
  valb.bits = val;
  out.write(valb.bytes, 1);
  return !out.bad();
}


bool IOSers::writeUInt16(ostream &out, const uint16_t val)
{
  uint16_bytes valb;
  valb.bits = htons(val);
  out.write(valb.bytes, 2);
  return !out.bad();
}


bool IOSers::writeUInt32(ostream &out, const uint32_t val)
{
  uint32_bytes valb;
  valb.bits = htonl(val);
  out.write(valb.bytes, 4);
  return !out.bad();
}


bool IOSers::writeUInt64(ostream &out, const uint64_t val)
{
  if (!writeUInt32(out, (val >> 32))) { return false; }
  if (!writeUInt32(out, val)) { return false; }
  return true;
}


bool IOSers::writeDec32_20(ostream &out, const long double val)
{
  return writeUInt32(out, (int32_t) (val * (1024 * 1024)));
}


bool IOSers::readUInt8(istream &in, uint8_t &val)
{
  uint8_bytes valb;
  in.read(valb.bytes, 1);
  if (in.fail()) { return false; }
  val = valb.bits;
  return true;
}


bool IOSers::readUInt16(istream &in, uint16_t &val)
{
  uint16_bytes valb;
  in.read(valb.bytes, 2);
  if (in.fail()) { return false; }
  val = ntohs(valb.bits);
  return true;
}


bool IOSers::readUInt32(istream &in, uint32_t &val)
{
  uint32_bytes valb;
  in.read(valb.bytes, 4);
  if (in.fail()) { return false; }
  val = ntohl(valb.bits);
  return true;
}


bool IOSers::readUInt64(istream &in, uint64_t &val)
{
  uint32_t temp;
  if (!readUInt32(in, temp)) { return false; }
  val = ((uint64_t) temp) << 32;
  if (!readUInt32(in, temp)) { return false; }
  val |= temp;
  return true;
}


bool IOSers::readDec32_20(istream &in, long double &val)
{
  uint32_t temp;
  if (!readUInt32(in, temp)) { return false; }
  val = ((long double) ((int32_t) temp)) / (1024 * 1024);
  return true;
}


IOSers::IOSers() { }
