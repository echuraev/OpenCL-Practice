#ifndef ERROR_H
#define ERROR_H

#include <string>

class Error
{
public:
    Error(std::string msg) : m_msg(msg)
    {}
    inline std::string getMsg() { return m_msg; }
private:
    std::string m_msg;
};

#endif