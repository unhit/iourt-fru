<?php
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


class ip2loc
{

  // $last_error gets set every time get_locations() is called.  If an
  // exception is thrown by get_locations(), $last_error will be set to NULL.
  // Otherwise, $last_error will be an array with keys being the full set
  // of IP address strings passed in to get_locations().  A NULL value means
  // there was no error.  Otherwise the value is a string describing the
  // nature of the problem.  If $locations is the value returned by
  // get_locations(), then exactly one of $locations[$ip_string] and
  // $last_error[$ip_string] will be NULL for each $ip_string passed in
  // to get_locations().
  public $last_error;

  private $serv_password;
  private $serv_address;
  private $serv_port;
  private $timeout; // Seconds, float.
  private $retries;
  private $socket;
  private $closed;

  // If construction fails, an exception is thrown.  $timeout is a value
  // specifying the number of milliseconds to wait for all return packets
  // on every try in get_locations().  The total number of tries made in
  // get_locations() is $retries + 1.  A "try" is an attempt to send request
  // packets for all outstanding IP addresses and wait for all corresponding
  // responses.  A request packet sent in an earlier try may result in
  // a response packet read in a later try, and this data will be accepted.
  // Therefore, the $timeout does not necessarily need to be greater than
  // the ping time to the server, as long as an appropriate amount of
  // retries are made to prolong the time between writing request packets
  // on the first try and reading response packets on the last try.  However,
  // it's recommended to set the $timeout higher than the ping time to the
  // server to avoid extra unnecessary data being sent and to account for
  // lost packets.
  public function __construct($ip2loc_serv_password,
                              $ip2loc_serv_address = "localhost",
                              $ip2loc_serv_port = 10020,
                              $timeout = 500, // milliseconds
                              $retries = 2)
  {
    $this->serv_password = $ip2loc_serv_password;
    if (!is_string($this->serv_password)) {
      throw new Exception("Password is not a string"); }
    // In IP2LocServer.cc, password must have length 4 or more.
    // The buffer for reading the password file has size 128.
    if (strlen($this->serv_password) < 4) {
      throw new Exception("Password is too short"); }
    if (strlen($this->serv_password) > 128) {
      throw new Exception("Password is too long"); }
    if (!(strstr($this->serv_password, "\n") === FALSE)) {
      throw new Exception("Password contains newline character"); }

    $this->serv_address = $ip2loc_serv_address;
    if (!is_string($this->serv_address)) {
      throw new Exception("Server address is not a string"); }

    $this->serv_port = $ip2loc_serv_port;
    if (!is_int($this->serv_port)) {
      throw new Exception("Server port is not an int, try casting"); }
    if (!(1 <= $this->serv_port && $this->serv_port <= 0x0000ffff)) {
      throw new Exception("Server port out of range"); }

    $this->timeout = $timeout;
    if (!is_numeric($this->timeout)) {
      throw new Exception("Timeout is not numeric"); }
    $this->timeout = max(0.1, min(10000.0, floatval($this->timeout)));
    $this->timeout = $this->timeout / 1000.0;

    $this->retries = $retries;
    if (!is_int($this->retries)) {
      throw new Exception("Retries is not an int, try casting"); }
    $this->retries = max(0, min(20, intval($this->retries)));

    $this->socket = @socket_create(AF_INET, SOCK_DGRAM, SOL_UDP);
    if (!$this->socket) {
      $error = "socket_create() failed, reason: " .
        socket_strerror(socket_last_error());
      throw new Exception($error); }

    if (!@socket_connect($this->socket, $this->serv_address,
                         $this->serv_port)) {
      $error = "socket_connect() failed, reason: " .
        socket_strerror(socket_last_error($this->socket));
      @socket_close($this->socket);
      throw new Exception($error); }
    $this->closed = FALSE;

    $this->last_error = NULL;
  }

  private function is_digit($ch)
  {
    $ch_ord = ord($ch);
    return ord("0") <= $ch_ord && $ch_ord <= ord("9");
  }

  // This is as strict as IP2LocServer::parseIPAddr() in the server code.
  private function is_ip($ip_string)
  {
    static $pow_base_10 = array(1, 10, 100);
    $octet_inx = 3;
    $digit_count = 0;
    $sum = 0;
    $character;
    $last_zero = FALSE;
    for ($i = strlen($ip_string); $i >= 0;) {
      if (--$i < 0 || ($character = $ip_string[$i]) == ".") {
        if ($sum == 0) {
          if ($digit_count != 1) { return FALSE; }
        }
        else { // Sum is not zero.
          if ($last_zero) { return FALSE; }
          if ($sum > 0xff) { return FALSE; }
        }
        $octet_inx--;
        if ($i >= 0 && $octet_inx < 0) { return FALSE; }
        $digit_count = 0;
        $sum = 0;
      }
      else if ($this->is_digit($character)) {
        if ($digit_count == 3) { return FALSE; }
        if ("0" == $character) { $last_zero = TRUE; }
        else { $last_zero = FALSE; }
        $sum += $pow_base_10[$digit_count++] * (ord($character) - ord("0"));
      }
      else { return FALSE; }
    }
    if ($octet_inx >= 0) { return FALSE; }
    return TRUE;
  }

  // Throws an exception if the socket has already been closed, if the
  // input value isn't an array, if the input array has length greater
  // than 64, or if not all values in the array are strings.  Returns an
  // array of ip_location objects (see below for the class definition).
  // The returned [associative] array has keys that are IP addresses in the
  // input array, and values that are the ip_location objects corresponding
  // the the IP address keys.  If an error condition arises for an IP
  // address, the value for that key will be NULL (the IP address key will
  // still exists in the array even if that IP address caused an error).
  // Errors are reported in the $last_error array, which also has the full
  // set of IP addresses as keys.  If $locations is the value returned by
  // this function, then exactly one of $locations[$ip_string] and
  // $last_error[$ip_string] will be NULL for each $ip_string passed to
  // this function.
  public function get_locations($ip_string_array)
  {
    $this->last_error = NULL;
    if ($this->closed) {
      throw new Exception("Socket has been closed"); }
    if (!is_array($ip_string_array)) {
      throw new Exception("Input to get_locations() is not an array"); }
    if (count($ip_string_array) > 64) {
      throw new Exception("Input array is too large, more than 64 values"); }

    $this->last_error = array();
    $sent_challenges = array();
    $locations = array(); // Return value.
    $bad_ip_count = 0;
    foreach ($ip_string_array as $key => $ip_string) {
      if (!is_string($ip_string)) {
        $this->last_error = NULL;
        throw new Exception("Value in input array to get_locations() " .
                            "at index $key is not a string"); }
      if (!$this->is_ip($ip_string)) {
        $this->last_error[$ip_string] = "Not an IP address";
        $sent_challenges[$ip_string] = NULL;
        $bad_ip_count++; }
      else {
        $this->last_error[$ip_string] = NULL;
        $sent_challenges[$ip_string] = array();
      }
      $locations[$ip_string] = NULL;
    }

    $locations_remaining = count($locations) - $bad_ip_count;
    $recv_buff = "";
    $last_error = NULL;
    $incorrect_challenges = array();
    $socket_write_errors = array();

    for ($try = 0; $try <= $this->retries; $try++) {
      if ($locations_remaining == 0) { break; }

      $microtime = (int) (microtime(TRUE) * 1000000); // For challenge.
      foreach ($sent_challenges as $ip_string => &$challenges_for_ip) {
        if ($this->last_error[$ip_string] !== NULL ||
            $locations[$ip_string] !== NULL) { continue; }
        $challenge = 0;
        while ($challenge == 0) {
          $challenge = 0xffffffff &
            ((mt_rand() << 16) ^ (0x0000ffff & mt_rand()) ^ $microtime); }
        if (@socket_write($this->socket,
                          sprintf("ip2locRequest\n%s\n" .
                                  "getLocationForIP:%08x\n%s\n",
                                  $this->serv_password,
                                  $challenge, $ip_string)) === FALSE) {
          $socket_write_errors[$ip_string] =
            "socket_write() failed, reason: " .
            socket_strerror(socket_last_error($this->socket));
          continue; // Try to write more packets even though probably fail.
        }
        $challenges_for_ip[] = $challenge;
      }
      unset($challenges_for_ip); // Because of the reference in "foreach".

      $time_after_sends = microtime(TRUE);
      $timeval = array();
      $fatal_error = FALSE;

      $bad_packets_after_timeout = 0;
      while (TRUE) {
        if ($locations_remaining == 0) { break; }
        if ($bad_packets_after_timeout > 1024) {
          $last_error = "Packet flood attack"; // Not fatal.
          break;
        }
        $microseconds_to_block =
          (int) (($this->timeout - (microtime(TRUE) -
                                    $time_after_sends)) * 1000000);
        $recv_flags = 0x00;
        if ($microseconds_to_block > 0) {
          $timeval["sec"] = (int) ($microseconds_to_block / 1000000);
          $timeval["usec"] = $microseconds_to_block - ($timeval["sec"] *
                                                       1000000);
          if (!@socket_set_option($this->socket,
                                  SOL_SOCKET, SO_RCVTIMEO, $timeval)) {
            $last_error =
              "socket_set_option() on SO_RCVTIMEO failed, reason: " .
              socket_strerror(socket_last_error($this->socket));
            $fatal_error = TRUE;
            break;
          }
        }
        else {
          $recv_flags = MSG_DONTWAIT;
        }
        $recv_len = @socket_recv($this->socket, $recv_buff, 512, $recv_flags);
        if ($recv_len === FALSE) {
          $errno = socket_last_error($this->socket);
          if ($errno === SOCKET_EAGAIN) {
            if ($last_error === NULL) {
              $last_error = "socket_recv() failed, reason: " .
                socket_strerror($errno); }
          }
          else {
            $last_error = "socket_recv() failed, reason: " .
              socket_strerror($errno);
            $fatal_error = TRUE; }
          break;
        }

        $raw_response = substr($recv_buff, 0, $recv_len);
        $response_lines = explode("\n", $raw_response, 11);
        $ip_string = NULL;
        if (count($response_lines) != 11 ||
            strcmp($response_lines[0], "ip2locResponse") ||
            strlen($response_lines[1]) != 25 ||
            strncmp($response_lines[1], "getLocationForIP:", 17) ||
            !$this->is_ip($ip_string = $response_lines[2]) ||
            strcmp($response_lines[3], "") ||
            strcmp($response_lines[10], "")) {
          $last_error = "Malformed ip2loc response";
          if ($microseconds_to_block <= 0) { $bad_packets_after_timeout++; }
          continue;
        }
        if (!array_key_exists($ip_string, $sent_challenges)) {
          $last_error = "Incorrect IP address in response";
          if ($microseconds_to_block <= 0) { $bad_packets_after_timeout++; }
          continue;
        }
        if ($locations[$ip_string] !== NULL) { // Probably dup.
          // Count dups as bad to prevent a dup flood attack.
          if ($microseconds_to_block <= 0) { $bad_packets_after_timeout++; }
          continue;
        }
        $response_challenge = (int) hexdec(substr($response_lines[1], 17));
        $challenge_matched = FALSE;
        if ($response_challenge != 0) { // We never send challenge zero.
          $challenges_for_ip = $sent_challenges[$ip_string];
          for ($i = count($challenges_for_ip) - 1; $i >= 0; $i--) {
            if ($challenges_for_ip[$i] == $response_challenge) {
              $challenge_matched = TRUE;
              break;
            }
          }
        }
        if (!$challenge_matched) {
          $incorrect_challenges[$ip_string] = TRUE;
          if ($microseconds_to_block <= 0) { $bad_packets_after_timeout++; }
          continue;
        }

        $locations[$ip_string] =
          new ip_location($response_lines[4], $response_lines[5],
                          $response_lines[6], $response_lines[7],
                          $response_lines[8], $response_lines[9],
                          $ip_string);
        $locations_remaining--;

      }

      if ($fatal_error) { break; }
    }

    if ($last_error === NULL) { $last_error = "Unknown error"; }
    foreach ($locations as $ip_string => $location) {
      if ($location === NULL && $this->last_error[$ip_string] === NULL) {
        if (array_key_exists($ip_string, $incorrect_challenges)) {
          $this->last_error[$ip_string] = "Incorrect challenge in response"; }
        else if (array_key_exists($ip_string, $socket_write_errors)) {
          // It may be that on the last try a packet was sent successfully.
          // However, report the first write fail as an error because it is
          // more specific.  Also, if there was a timeout problem it could be
          // because only the last try was successful in sending, and not
          // enough time was given to receive the response.  Just an example.
          $this->last_error[$ip_string] = $socket_write_errors[$ip_string]; }
        else {
          // The last error wasn't specific to a particular IP address
          // because we didn't get as far as receiving a response for
          // a specific requested IP address.  Therefore give all unanswered
          // requests the last error, because the last error could have
          // been for any of the unanswered packets.
          $this->last_error[$ip_string] = $last_error; }
        $locations_remaining--;
      }
      else if ($location !== NULL && $this->last_error[$ip_string] !== NULL) {
        $this->last_error = NULL;
        throw new Exception("\$location not NULL and \$last_error not NULL, " .
                            "programming error"); }
    }
    if ($locations_remaining != 0) {
      $this->last_error = NULL;
      throw new Exception("\$locations_remaining not zero, programming error");
    }
    return $locations;
  }

  // Closes the underlying socket.
  public function close()
  {
    $this->closed = TRUE;
    @socket_close($this->socket);
  }

}


class ip_location
{

  private $location;

  /* private */ function __construct($country_code,
                                     $country,
                                     $region,
                                     $city,
                                     $latitude,
                                     $longitude,
                                     $ip_addr)
  {
    $this->location = array();
    $this->location[0] = $country_code;
    $this->location[1] = $country;
    $this->location[2] = $region;
    $this->location[3] = $city;
    $this->location[4] = $latitude;
    $this->location[5] = $longitude;
    $this->location[6] = $ip_addr;
  }

  public function country_code() { return $this->location[0]; }
  public function country() { return $this->location[1]; }
  public function region() { return $this->location[2]; }
  public function city() { return $this->location[3]; }
  public function latitude() { return floatval($this->location[4]); }
  public function longitude() { return floatval($this->location[5]); }
  public function ip_addr() { return $this->location[6]; }

}

?>
