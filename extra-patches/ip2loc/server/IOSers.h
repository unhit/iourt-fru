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

#ifndef IOSERS_H
#define IOSERS_H

#include <iostream>
#include <stdint.h>


class IOSers
{

 public:

  /**
   * Writes an 8 bit unsigned integer quantity to the specified ostream.
   * This function is the inverse of readUInt8().  Returns true if and only if
   * writing the byte was successful.
   */
  static bool writeUInt8(std::ostream &out, const uint8_t val);

  /**
   * Writes a 16 bit unsigned integer quantity to the specified ostream.
   * The 2 bytes from the 16 bit quantity are written in network byte order,
   * meaning that the most significant 8 bits are output first.  This
   * function is the inverse of readUInt16().  Returns true if and only if
   * writing the 2 bytes was successful.
   */
  static bool writeUInt16(std::ostream &out, const uint16_t val);

  /**
   * Writes a 32 bit unsigned integer quantity to the specified ostream.
   * The 4 bytes from the 32 bit quantity are written in network byte order,
   * meaning that the most significant 8 bits are output first.  This
   * function is the inverse of readUInt32().  Returns true if and only if
   * writing the 4 bytes was successful.
   */
  static bool writeUInt32(std::ostream &out, const uint32_t val);

  /**
   * Writes a 64 bit unsigned integer quantity to the specified ostream.
   * The 8 bytes from the 64 bit quantity are written in network byte order,
   * meaning that the most significant 8 bits are output first.  This
   * function is the inverse of readUInt64().  Returns true if and only if
   * writing the 8 bytes was successful.
   */
  static bool writeUInt64(std::ostream &out, const uint64_t val);

  /**
   * Writes a floating point number to the specified ostream as a 4 byte
   * quantity.  The input value should be strictly less than 2048.0 and
   * greater than or equal to -2048.  The algorithm for converting the input
   * floating point value to a 4 byte quantity is as follows.  First, the
   * input value is multiplied by 2^20.  Then, that resulting value
   * is cast to a 32 bit signed integer quantity.  The 32 bit signed integer
   * is written as a 4 byte sequence in network byte order (the most
   * significant 8 bits come first).  This function is the inverse of
   * readDec32_20(); however, the accuracy of data is not preserved
   * completely because of the algorithm used.  Returns true if and only if
   * writing the 4 bytes was successful.  Bytes are written even if the input
   * value is not in the required range.
   */
  static bool writeDec32_20(std::ostream &out, const long double val);

  /**
   * Reads an 8 bit unsigned integer quantity from the specified istream.
   * This function is the inverse of writeUInt8().  Returns true if and only
   * if reading the byte was successful.
   */
  static bool readUInt8(std::istream &in, uint8_t &val);

  /**
   * Reads a 16 bit unsigned integer quantity from the specified istream.
   * The 2 bytes making up the 16 bit quantity are assumed to be in network
   * byte order, meaning that the most significant 8 bits will be read first.
   * This function is the inverse of writeUInt16().  Returns true if and only
   * if reading 2 bytes was successful.
   */
  static bool readUInt16(std::istream &in, uint16_t &val);

  /**
   * Reads a 32 bit unsigned integer quantity from the specified istream.
   * The 4 bytes making up the 32 bit quantity are assumed to be in network
   * byte order, meaning that the most significant 8 bits will be read first.
   * This function is the inverse of writeUInt32().  Returns true if and only
   * if reading 4 bytes was successful.
   */
  static bool readUInt32(std::istream &in, uint32_t &val);

  /**
   * Reads a 64 bit unsigned integer quantity from the specified istream.
   * The 8 bytes making up the 64 bit quantity are assumed to be in network
   * byte order, meaning that the most significant 8 bits will be read first.
   * This function is the inverse of writeUInt64().  Returns true if and only
   * if reading 8 bytes was successful.
   */
  static bool readUInt64(std::istream &in, uint64_t &val);

  /**
   * Assembles a floating point number from the specified istream by reading
   * 4 bytes.  The algorithm for converting 4 bytes into a floating point
   * value is as follows.  First, a 32 bit signed integer quantity is assembled
   * from the 4 bytes by using network byte order (first byte read corresponds
   * to the most significant 8 bits).  This integer value is cast to a floating
   * point, and is then divided by 2^20.  Therefore, values assembled by this
   * function are always greater than or equal to -2048.0 and strictly less
   * than 2048.0.  This function is the inverse of writeDec32_20().  Returns
   * true if and only if reading 4 bytes was successful.
   */
  static bool readDec32_20(std::istream &in, long double &val);


 private:

  /**
   * Objects of this class should never be instantiated.
   */
  IOSers();

};

#endif
