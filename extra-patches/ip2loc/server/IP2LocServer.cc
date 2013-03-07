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

#include "IP2LocServer.h"
#include <arpa/inet.h>
#include <cstdlib>
#include <errno.h>
#include <fstream>
#include <pthread.h>
#include <sstream>
#include <sys/socket.h>


using namespace std;


int IP2LocServer::main(int argc, char *argv[])
{
  errno = 0;
  if (!(2 <= argc && argc <= 3)) {
    cerr << "Usage:" << endl;
    cerr << "  " << argv[0] << " <listen-port> <listen-IP>" << endl;
    cerr << "The second argument, the IP address to listen on, can be omitted, in which" << endl;
    cerr << "case the server will listen on all available interfaces.  This program reads" << endl;
    cerr << "standard input, which should be the contents of an \"ip2loc.bin\" file.  A" << endl;
    cerr << "file in the current directory named \".password\" will be read, which should" << endl;
    cerr << "consist of a single line of text specifying the password that the server will" << endl;
    cerr << "be protected by." << endl;
    return EXIT_FAILURE;
  }
  string error;
  sockaddr_in listenSockAddr;
  if (!IP2LocServer::parseListenSockAddr(argc, argv, listenSockAddr, error)) {
    cerr << error << endl; return EXIT_FAILURE;
  }
  string password;
  if (!IP2LocServer::readPassword(password, error)) {
    cerr << error << endl; return EXIT_FAILURE;
  }
  const IP2LocLookup *ip2locLookup = IP2LocLookup::load(cin, error);
  if (ip2locLookup == NULL) {
    cerr << "Error reading database from standard in:" << endl;
    cerr << error << endl; return EXIT_FAILURE;
  }
  int socket = IP2LocServer::createSocket(listenSockAddr, error);
  if (socket == -1) {
    delete ip2locLookup;
    cerr << "Could not create server socket:" << endl;
    cerr << error << endl; return EXIT_FAILURE;
  }
  pthread_attr_t workerThreadAttr;
  pthread_attr_init(&workerThreadAttr);
  pthread_attr_setdetachstate(&workerThreadAttr, PTHREAD_CREATE_JOINABLE);
  IP2LocServerComms comms;
  IP2LocServer::initComms(comms, socket, listenSockAddr.sin_addr, password,
                          ip2locLookup);
  pthread_t workerThread;
  int err;
  bool errRecv = false;
  if ((err = pthread_create(&workerThread, &workerThreadAttr,
                            IP2LocServer::workLoop, (void *) &comms)) != 0) {
    cerr << "Unable to create thread: " << strerror(err) << endl;
  }
  pthread_attr_destroy(&workerThreadAttr);
  if (err == 0) {
    IP2LocServer::receiveLoop((void *) &comms);
    if (comms.receiveError) {
      cerr << "Encountered error in receive loop, exiting" << endl;
      errRecv = true;
    }
    if ((err = pthread_join(workerThread, NULL)) != 0) {
      cerr << "Unable to join thread: " << strerror(err) << endl;
    }
  }
  IP2LocServer::teardownComms(comms);
  close(socket);
  delete ip2locLookup;
  if (err != 0 || errRecv) { return EXIT_FAILURE; }
  return EXIT_SUCCESS;
}


void IP2LocServer::initComms(IP2LocServerComms &comms, int socket,
                             in_addr localListenAddr, string &password,
                             const IP2LocLookup *ip2locLookup)
{
  comms.socket = socket;
  if (localListenAddr.s_addr == htonl(INADDR_ANY)) {
    comms.localListenAddr.s_addr = htonl(INADDR_LOOPBACK);
  }
  else {
    comms.localListenAddr = localListenAddr;
  }
  comms.password = password;
  comms.halt = 0;
  pthread_mutex_init(&comms.mutex, NULL);
  pthread_cond_init(&comms.cond, NULL);
  comms.queueHead = IP2LOCSERVER_QUEUE_LEN - 1;
  comms.queueTail = IP2LOCSERVER_QUEUE_LEN - 1;
  comms.queueUnprocessed = 0;
  comms.ip2locLookup = ip2locLookup;
  comms.receiveError = false;
  comms.receiveErrorOut = &cerr;
  comms.workErrorOut = &cerr;
}


void IP2LocServer::teardownComms(IP2LocServerComms &comms)
{
  comms.password.clear();
  pthread_mutex_destroy(&comms.mutex);
  pthread_cond_destroy(&comms.cond);
  comms.ip2locLookup = NULL;
  comms.receiveErrorOut = NULL;
  comms.workErrorOut = NULL;
}


void *IP2LocServer::receiveLoop(void *arg)
{
  IP2LocServerComms *comms = (IP2LocServerComms *) arg;
  char inBuff[IP2LOCSERVER_INCOMING_BUFF_LEN];
  sockaddr_in clSockAddr;
  while (comms->halt == 0) {
    socklen_t clSockAddrLen = sizeof(clSockAddr);
    const ssize_t readLen = recvfrom(comms->socket, inBuff, sizeof(inBuff), 0,
                                     (sockaddr *) &clSockAddr, &clSockAddrLen);
    if (readLen == -1) {
      if (errno == EAGAIN) { errno = 0; continue; } // Timed out.
      *comms->receiveErrorOut << "Problem in recvfrom(): ";
      *comms->receiveErrorOut << strerror(errno) << endl;
      comms->receiveError = true;
      errno = 0;
      break;
    }
    // BEGIN: Sanity data check; verify password etc.
    if (readLen < 4) { continue; }
    uint16_t inBuffIndex = 0;
    bool q3Request = false;
    if (memcmp(inBuff, "\xff" "\xff" "\xff" "\xff", 4) == 0) {
      q3Request = true; inBuffIndex += 4;
    }
    if (inBuffIndex + 13 > readLen) { continue; }
    if (memcmp(inBuff + inBuffIndex, "ip2locRequest", 13) != 0) { continue; }
    inBuffIndex += 13;
    if (inBuffIndex >= readLen || inBuff[inBuffIndex++] != '\n') { continue; }
    if (inBuffIndex + comms->password.length() > (unsigned) readLen) {
      continue; }
    if (memcmp(inBuff + inBuffIndex, comms->password.data(),
               comms->password.length()) != 0) { continue; }
    inBuffIndex += comms->password.length();
    if (inBuffIndex >= readLen || inBuff[inBuffIndex++] != '\n') { continue; }
    // END: Sanity data check.
    if (comms->queueUnprocessed == IP2LOCSERVER_QUEUE_LEN) { continue; }
    IP2LocQuery *query = &comms->queryQueue[comms->queueTail--];
    if (comms->queueTail < 0) {
      comms->queueTail = IP2LOCSERVER_QUEUE_LEN - 1;
    }
    memcpy(query->inBuff, inBuff + inBuffIndex, readLen - inBuffIndex);
    query->inBuffLen = readLen - inBuffIndex;
    memcpy(&query->clSockAddr, &clSockAddr, clSockAddrLen);
    query->clSockAddrLen = clSockAddrLen;
    query->q3Request = q3Request;
    pthread_mutex_lock(&comms->mutex);
    comms->queueUnprocessed++;
    pthread_cond_signal(&comms->cond);
    pthread_mutex_unlock(&comms->mutex);
  }
  pthread_mutex_lock(&comms->mutex);
  comms->halt = 1;
  pthread_cond_signal(&comms->cond);
  pthread_mutex_unlock(&comms->mutex);
  if (errno != 0) {
    *comms->receiveErrorOut << "Exiting receiveLoop(), and errno is " << errno;
    *comms->receiveErrorOut << endl;
  }
  return NULL;
}


void *IP2LocServer::workLoop(void *arg)
{
  IP2LocServerComms *comms = (IP2LocServerComms *) arg;
  char outBuff[IP2LOCSERVER_OUTGOING_BUFF_LEN];
  const size_t outBuffSize = sizeof(outBuff);
  while (true) {
    pthread_mutex_lock(&comms->mutex);
    while (comms->queueUnprocessed == 0 && comms->halt == 0) {
      pthread_cond_wait(&comms->cond, &comms->mutex);
    }
    pthread_mutex_unlock(&comms->mutex);
    if (comms->halt != 0) { break; }
    IP2LocQuery *query = &comms->queryQueue[comms->queueHead--];
    if (comms->queueHead < 0) {
      comms->queueHead = IP2LOCSERVER_QUEUE_LEN - 1;
    }
    uint16_t inBuffIndex = 0;
    int16_t challengeStartIndex = -1;
    int16_t commandLineLen = -1;
    while (true) {
      if (inBuffIndex >= query->inBuffLen) { break; }
      char ch = query->inBuff[inBuffIndex++];
      if (ch == '\n') { commandLineLen = inBuffIndex - 1; break; }
      if (challengeStartIndex < 0 && ch == ':') {
        challengeStartIndex = inBuffIndex;
      }
    }
    while (true) { // To provide a break mechanism.
      if (commandLineLen < 0) { break; }
      int16_t commandStrLen;
      if (challengeStartIndex >= 0) { // Will be -1 or strictly greater than 0.
        if (commandLineLen - challengeStartIndex != 8) { break; }
        bool valid = true;        
        for (int16_t i = challengeStartIndex; i < commandLineLen; i++) {
          char ch = query->inBuff[i];
          if (!(('0' <= ch && ch <= '9') || ('a' <= ch && ch <= 'f'))) {
            valid = false;
            break;
          }
        }
        if (!valid) { break; }
        challengeStartIndex--; // Now includes the ':'.
        commandStrLen = challengeStartIndex;
      }
      else {
        commandStrLen = commandLineLen;
      }

      ////////// BEGIN quit ///////////////////////////////////////////////////
      if (commandStrLen == 4 && memcmp(query->inBuff, "quit", 4) == 0) {
        if (query->inBuffLen != inBuffIndex || query->q3Request ||
            query->clSockAddrLen != sizeof(query->clSockAddr) ||
            query->clSockAddr.sin_addr.s_addr !=
            comms->localListenAddr.s_addr) { break; }
        comms->halt = 1;
      }
      ////////// END quit /////////////////////////////////////////////////////

      ////////// BEGIN getLocationForIP ///////////////////////////////////////
      else if (commandStrLen == 16 &&
               memcmp(query->inBuff, "getLocationForIP", 16) == 0) {
        uint16_t ipStrIndex = inBuffIndex;
        int16_t ipStrLen = -1;
        while (true) {
          if (inBuffIndex >= query->inBuffLen) { break; }
          if (query->inBuff[inBuffIndex++] == '\n') {
            ipStrLen = inBuffIndex - ipStrIndex - 1;
            break;
          }
        }
        if (ipStrLen < 0 || ipStrLen > 0x7f ||
            inBuffIndex != query->inBuffLen) { break; }
        uint32_t ipAddr;
        if (!IP2LocServer::parseIPAddr(query->inBuff + ipStrIndex,
                                       (uint8_t) ipStrLen, ipAddr)) { break; }
        const Location *location =
          comms->ip2locLookup->getLocationForIP(ipAddr);
        size_t outBuffIndex = 0;
        const string *locStrs[] =
          { location->getCountryCode(), location->getCountry(),
            location->getRegion(), location->getCity() };
        if (query->q3Request) {
          IP2LocServer::writeString(outBuff, outBuffSize, outBuffIndex,
                                    "\xff" "\xff" "\xff" "\xff", 4);
          IP2LocServer::writeString(outBuff, outBuffSize, outBuffIndex,
                                    "ip2LocResponse", 14);
          IP2LocServer::writeString(outBuff, outBuffSize, outBuffIndex,
                                    " \"", 2);
          IP2LocServer::writeString(outBuff, outBuffSize, outBuffIndex,
                                    "getLocationForIP", 16);
          if (challengeStartIndex >= 0) {
            IP2LocServer::writeString
              (outBuff, outBuffSize, outBuffIndex,
               query->inBuff + challengeStartIndex,
               commandLineLen - challengeStartIndex);
          }
          IP2LocServer::writeString(outBuff, outBuffSize, outBuffIndex,
                                    "\" \"", 3);
          IP2LocServer::writeString(outBuff, outBuffSize, outBuffIndex,
                                    query->inBuff + ipStrIndex, ipStrLen);
          IP2LocServer::writeString(outBuff, outBuffSize, outBuffIndex,
                                    "\" \"", 3);
          for (uint8_t i = 0; i < 4; i++) {
            IP2LocServer::writeString(outBuff, outBuffSize, outBuffIndex,
                                      locStrs[i]->data(),
                                      locStrs[i]->length());
            IP2LocServer::writeString(outBuff, outBuffSize, outBuffIndex,
                                      "\" \"", 3);
          }
          IP2LocServer::writeFloat(outBuff, outBuffSize, outBuffIndex,
                                   location->getLatitude());
          IP2LocServer::writeString(outBuff, outBuffSize, outBuffIndex,
                                    "\" \"", 3);
          IP2LocServer::writeFloat(outBuff, outBuffSize, outBuffIndex,
                                   location->getLongitude());
          IP2LocServer::writeString(outBuff, outBuffSize, outBuffIndex,
                                    "\"", 1);          
        }
        else {
          IP2LocServer::writeLine(outBuff, outBuffSize, outBuffIndex,
                                  "ip2locResponse", 14);
          IP2LocServer::writeString(outBuff, outBuffSize, outBuffIndex,
                                    "getLocationForIP", 16);
          if (challengeStartIndex >= 0) {
            IP2LocServer::writeString
              (outBuff, outBuffSize, outBuffIndex,
               query->inBuff + challengeStartIndex,
               commandLineLen - challengeStartIndex);
          }
          IP2LocServer::writeNewline(outBuff, outBuffSize, outBuffIndex);
          IP2LocServer::writeLine(outBuff, outBuffSize, outBuffIndex,
                                  query->inBuff + ipStrIndex, ipStrLen);
          IP2LocServer::writeNewline(outBuff, outBuffSize, outBuffIndex);
          for (uint8_t i = 0; i < 4; i++) {
            IP2LocServer::writeLine(outBuff, outBuffSize, outBuffIndex,
                                    locStrs[i]->data(), locStrs[i]->length());
          }
          IP2LocServer::writeFloatLine(outBuff, outBuffSize, outBuffIndex,
                                       location->getLatitude());
          IP2LocServer::writeFloatLine(outBuff, outBuffSize, outBuffIndex,
                                       location->getLongitude());
        }
        ssize_t sentLen =
          sendto(comms->socket, outBuff, outBuffIndex, 0,
                 (sockaddr *) &query->clSockAddr, query->clSockAddrLen);
        if (sentLen == -1) {
          if (query->clSockAddrLen == sizeof(query->clSockAddr)) {
            *comms->workErrorOut << "Problem in sendto() sending to ";
            IP2LocServer::printSockAddr(*comms->workErrorOut,
                                        query->clSockAddr);
            *comms->workErrorOut << ": " << strerror(errno) << endl;
          }
          else {
            *comms->workErrorOut << "Problem in sendto(): ";
            *comms->workErrorOut << strerror(errno) << endl;
          }
          errno = 0;
        }
        else if (sentLen != (signed) outBuffIndex) {
          if (query->clSockAddrLen == sizeof(query->clSockAddr)) {
            *comms->workErrorOut << "In sendto() sending to ";
            IP2LocServer::printSockAddr(*comms->workErrorOut,
                                        query->clSockAddr);
            *comms->workErrorOut << ", only " << sentLen << " out of ";
            *comms->workErrorOut << outBuffIndex << " bytes sent" << endl;
          }
          else {
            *comms->workErrorOut << "In sendto(), only " << sentLen;
            *comms->workErrorOut << " out of " << outBuffIndex;
            *comms->workErrorOut << " bytes sent" << endl;
          }
        }
      }
      ////////// END getLocationForIP /////////////////////////////////////////

      break;
    } // End of break mechanism.
    pthread_mutex_lock(&comms->mutex);
    comms->queueUnprocessed--;
    pthread_mutex_unlock(&comms->mutex);
  }
  comms->halt = 1;
  if (errno != 0) {
    *comms->workErrorOut << "Exiting workLoop(), and errno is " << errno;
    *comms->workErrorOut << endl;
  }
  return NULL;
}


bool IP2LocServer::parseIPAddr(const char *ipStr, uint8_t strLen,
                               uint32_t &ipAddr)
{
  static const uint16_t powBase10[] = { 1u, 10u, 100u };
  int8_t octetInx = 3;
  uint8_t digitCount = 0;
  uint16_t sum = 0;
  char character = 0;
  bool lastZero = false;
  ipAddr = 0;
  for (int16_t i = strLen; i >= 0;) {
    if (--i < 0 || (character = ipStr[i]) == '.') {
      if (sum == 0) {
        if (digitCount != 1) { return false; }
      }
      else { // sum is not zero.
        if (lastZero) { return false; }
        if (sum > 0x00ffu) { return false; }
      }
      ipAddr |= ((uint32_t) sum) << ((3 - octetInx--) << 3);
      if (i >= 0 && octetInx < 0) { return false; }
      digitCount = 0;
      sum = 0;
    }
    else if (isdigit(character)) {
      if (digitCount == 3) { return false; }
      if ('0' == character) { lastZero = true; }
      else { lastZero = false; }
      sum += (powBase10[digitCount++] * (character - '0'));
    }
    else { return false; }
  }
  if (octetInx >= 0) { return false; }
  return true;
}


bool IP2LocServer::parseListenSockAddr(int argc, char *argv[],
                                       sockaddr_in &sa, string &error)
{
  bzero(&sa, sizeof(sa));
  sa.sin_family = AF_INET;
  string portStr(argv[1]);
  const int port = atoi(portStr.c_str());
  stringstream sstream(ios_base::out);
  sstream << port;
  string portStr2(sstream.str());
  if (!(portStr == portStr2)) {
    error = "Could not parse port"; return false;
  }
  if (port < 1 || port > 0xffff) {
    error = "Port out of range"; return false;
  }
  sa.sin_port = htons((uint16_t) port);
  if (argc > 2) {
    if (inet_pton(AF_INET, argv[2], &sa.sin_addr) != 1) {
      error = "Could not parse listen IP address"; return false;
    }
  }
  else {
    sa.sin_addr.s_addr = htonl(INADDR_ANY);
  }
  return true;
}


bool IP2LocServer::readPassword(string &password, string &error)
{
  ifstream fileIn;
  fileIn.open(".password", ifstream::in);
  if (!fileIn.good()) {
    if (fileIn.is_open()) { fileIn.close(); }
    error = "Could not open .password file"; return false;
  }
  char fileContents[128];
  fileIn.read(fileContents, sizeof(fileContents));
  if (fileIn.bad()) {
    if (fileIn.is_open()) { fileIn.close(); }
    error = "Failed to read .password file"; return false;
  }
  if (!fileIn.eof()) {
    if (fileIn.is_open()) { fileIn.close(); }
    error = ".password file is too large"; return false;
  }
  uint8_t fileSize = fileIn.gcount();
  if (fileIn.is_open()) { fileIn.close(); }
  stringstream sstream(ios_base::in | ios_base::out);
  sstream.write(fileContents, fileSize);
  if (getline(sstream, password) == NULL) {
    error = ".password file is empty"; return false;
  }
  string line;
  if (getline(sstream, line) != NULL) {
    error = ".password file contains multiple lines of text"; return false;
  }
  if (password.length() < 4) {
    error = "Password is too short"; return false;
  }
  return true;
}


int IP2LocServer::createSocket(sockaddr_in &sa, string &error)
{
  int sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (sock == -1) {
    error = "Failed in socket(): ";
    error.append(strerror(errno));
    return -1;
  }
  timeval timeo;
  bzero(&timeo, sizeof(timeo));
  timeo.tv_usec = 250000;
  if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeo, sizeof(timeo)) != 0) {
    close(sock);
    error = "Failed to setsockopt() SO_RCVTIMEO: ";
    error.append(strerror(errno));
    return -1;
  }
  if (bind(sock, (sockaddr *) &sa, sizeof(sa)) != 0) {
    close(sock);
    error = "Failed in bind(): ";
    error.append(strerror(errno));
    return -1;
  }
  return sock;
}


void IP2LocServer::printSockAddr(ostream &out, const sockaddr_in &sa)
{
  in_addr_t addr = sa.sin_addr.s_addr;
  out << ((addr >> 24) & 0xff) << "." << ((addr >> 16) & 0xff);
  out << "." << ((addr >> 8) & 0xff) << "." << (addr & 0xff);
  out << ":" << sa.sin_port;
}
