#ifndef __CUSTOMTIME_H__
#define __CUSTOMTIME_H__

#include <ctime>
#include <iostream>
#include <unordered_map>

class CustomTime{
  public:
    static std::tm* convert_string_time_to_tm(std::string rawString);
};

#endif
