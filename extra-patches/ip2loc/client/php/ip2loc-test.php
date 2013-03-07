<?php

require_once "ip2loc.php";

$ip = "99.50.206.241"; 
$ip2loc = NULL;
try { $ip2loc = new ip2loc("pass", "localhost", 10020, 300, 2); }
catch (Exception $e) {
  echo "Error in constructor: " . $e->getMessage() . "\n";
  exit(1);
}
$locations = NULL;
try { $locations = $ip2loc->get_locations(array($ip)); }
catch (Exception $e) {
  echo "Error in get_locations(): " . $e->getMessage() . "\n";
  exit(1);
}
$ip2loc->close();
$location = $locations[$ip];
if (!$location) {
  echo "Error in get_locations(): " . $ip2loc->last_error[$ip] . "\n";
  exit(1);
}
echo "ip        : " . $location->ip_addr() . "\n";
echo "cc        : " . $location->country_code() . "\n";
echo "country   : " . $location->country() . "\n";
echo "region    : " . $location->region() . "\n";
echo "city      : " . $location->city() . "\n";
echo "latitude  : " . $location->latitude() . "\n";
echo "longitude : " . $location->longitude() . "\n";
exit(0);

?>
