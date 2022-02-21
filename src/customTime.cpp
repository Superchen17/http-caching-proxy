#include "customTime.h"

#include <sstream>
#include <cstring>

std::tm* CustomTime::convert_string_time_to_tm(std::string rawString){
  // example input: Tue, 15 Feb 2022 03:05:27 GMT

  std::unordered_map<std::string, int> monthMap = {
    {"Jan", 1}, {"Feb", 2}, {"Mar", 3}, {"Apr", 4},
    {"May", 5}, {"Jun", 6}, {"Jul", 7}, {"Aug", 8},
    {"Sep", 9}, {"Oct", 10}, {"Nov", 11}, {"Dec", 12}
  };

  std::tm* timestamp = new std::tm(); 
  timestamp->tm_year = std::stoi(rawString.substr(12)) - 1900;
  timestamp->tm_mon = monthMap[rawString.substr(8, 3)] - 1;
  timestamp->tm_mday = std::stoi(rawString.substr(5));
  timestamp->tm_hour = std::stoi(rawString.substr(17));
  timestamp->tm_min = std::stoi(rawString.substr(20));
  timestamp->tm_sec = std::stoi(rawString.substr(23));
  timestamp->tm_isdst = -1;

  return timestamp;
} 

std::string CustomTime::convert_time_t_to_string(std::time_t time){
  std::stringstream ss;
  ss << std::strtok(std::ctime(&time), "\n");
  return ss.str();
}

std::string CustomTime::get_current_time_in_str(){
  std::time_t timeNow = std::time(0);
  return convert_time_t_to_string(timeNow);
}

std::time_t get_current_time_in_time(){
  std::time_t timeNow = std::time(0);
  return timeNow;
}
