#ifndef __CUSTOMTIME_H__
#define __CUSTOMTIME_H__

#include <ctime>
#include <iostream>
#include <unordered_map>

class CustomTime{
  public:

    /**
     * @brief convert time from string to std::tm
     * 
     * @param rawString raw time string
     * @return std::tm* converted std::tm pointer
     */
    static std::tm* convert_string_time_to_tm(std::string rawString);

    /**
     * @brief convert time from time_t to string
     * 
     * @param time 
     * @return std::string 
     */
    static std::string convert_time_t_to_string(std::time_t time);

    /**
     * @brief Get the current time in string format
     * 
     * @return std::string 
     */
    static std::string get_current_time_in_str();

    /**
     * @brief Get the current time in time object
     * 
     * @return std::time_t 
     */
    static std::time_t get_current_time_in_time();
};

#endif
