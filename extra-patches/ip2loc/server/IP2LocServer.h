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

#ifndef IP2LOCSERVER_H
#define IP2LOCSERVER_H

#include "IP2LocLookup.h"
#include <cstring>
#include <netinet/in.h>
#include <signal.h>
#include <sstream>
#include <string>

#define IP2LOCSERVER_INCOMING_BUFF_LEN 256
#define IP2LOCSERVER_OUTGOING_BUFF_LEN 512

// This cannot be greater than 127.
#define IP2LOCSERVER_QUEUE_LEN 64


struct IP2LocQuery {
  char inBuff[IP2LOCSERVER_INCOMING_BUFF_LEN];
  uint16_t inBuffLen;
  sockaddr_in clSockAddr;
  socklen_t clSockAddrLen;
  bool q3Request;
};


struct IP2LocServerComms {
  int socket;
  in_addr localListenAddr;
  std::string password;
  sig_atomic_t halt; // Non-zero means halt.
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  IP2LocQuery queryQueue[IP2LOCSERVER_QUEUE_LEN];
  int8_t queueHead;
  int8_t queueTail;
  sig_atomic_t queueUnprocessed;
  const IP2LocLookup *ip2locLookup;
  bool receiveError;
  std::ostream *receiveErrorOut;
  std::ostream *workErrorOut;
};


class IP2LocServer
{

 public:

  static int main(int argc, char *argv[]);

  static void initComms(IP2LocServerComms &comms, int socket,
                        in_addr localListenAddr, std::string &password,
                        const IP2LocLookup *ip2locLookup);

  static void teardownComms(IP2LocServerComms &comms);

  /**
   * This is designed to be called by a new pthread.  The input arg is
   * a reference to IP2LocServerComms.  NULL is returned.
   */
  static void *receiveLoop(void *arg);

  /**
   * This is designed to be called by a new pthread.  The input arg is
   * a reference to IP2LocServerComms.  NULL is returned.
   */
  static void *workLoop(void *arg);

  static bool parseIPAddr(const char *ipStr, uint8_t strLen, uint32_t &ipAddr);

  static bool parseListenSockAddr(int argc, char *argv[],
                                  sockaddr_in &sa, std::string &error);

  static bool readPassword(std::string &password, std::string &error);

  static int createSocket(sockaddr_in &sa, std::string &error);

  static void printSockAddr(std::ostream &out, const sockaddr_in &sa);

  /**
   * There is a danger of overflow if the size_t input parameters are too big.
   * Error checks for this condition are not made.  This will return false
   * if and only if not the complete line could not be written or if
   * outBuffIndex is greater than outBuffSize (regardless of whether or not
   * lineLen is 0).
   */
  inline
    static bool writeString(char *outBuff, size_t outBuffSize,
                            size_t &outBuffIndex,
                            const char *line, size_t lineLen)
  {
    bool rtnVal = true;
    ssize_t sLineLen = lineLen;
    if (outBuffIndex + sLineLen > outBuffSize) {
      sLineLen = outBuffSize - (ssize_t) outBuffIndex;
      rtnVal = false;
    }
    if (sLineLen > 0) {
      memcpy(outBuff + outBuffIndex, line, sLineLen);
      outBuffIndex += sLineLen;
    }
    return rtnVal;
  }


  inline
    static bool writeNewline(char *outBuff, size_t outBuffSize,
                             size_t &outBuffIndex)
  {
    if (outBuffIndex < outBuffSize) {
      outBuff[outBuffIndex++] = '\n';
      return true;
    }
    return false;
  }


  /**
   * This writes an additional '\n' after the line.  There is a danger of
   * overflow if the size_t input parameters are too big.  Error checks for
   * this condition are not made.
   */
  inline
    static bool writeLine(char *outBuff, size_t outBuffSize,
                          size_t &outBuffIndex,
                          const char *line, size_t lineLen)
  {
    IP2LocServer::writeString(outBuff, outBuffSize, outBuffIndex,
                              line, lineLen);
    return IP2LocServer::writeNewline(outBuff, outBuffSize, outBuffIndex);
  }


  inline
    static bool writeFloat(char *outBuff, size_t outBuffSize,
                           size_t &outBuffIndex, double value)
  {
    if (outBuffIndex >= outBuffSize) { return false; }
    // Using sstream is not the most efficient way to do this, but it is
    // the simplest conceptually and the safest.
    std::stringstream sstream(std::ios_base::out);
    sstream << value;
    std::string str = sstream.str();
    return IP2LocServer::writeString(outBuff, outBuffSize, outBuffIndex,
                                     str.data(), str.length());
  }


  /**
   * Writes a '\n' after the floating point value.
   */
  inline
    static bool writeFloatLine(char *outBuff, size_t outBuffSize,
                               size_t &outBuffIndex, double value)
  {
    IP2LocServer::writeFloat(outBuff, outBuffSize, outBuffIndex, value);
    return IP2LocServer::writeNewline(outBuff, outBuffSize, outBuffIndex);
  }


 private:

};

#endif
