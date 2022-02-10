#include <exception>

/**
 * @brief custom exception class
 * 
 */
class CustomException: public std::exception{
  private:
    const char* errMsg;

  public:
    CustomException(): errMsg("error: something\'s wrong"){}
    CustomException(const char* _errMsg): errMsg(_errMsg){}
    virtual const char* what() const throw() {
      return this->errMsg;
    }
};
