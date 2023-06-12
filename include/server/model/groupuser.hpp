#ifndef GROUPUSER_H
#define GROUPUSER_H

#include "user.hpp"
class GroupUser : public User
{
public:
    void setRole(string role) {this->role = role;}
    
    string getRole() {return this->role;}
protected:
private:
    string role; // creator / normal
};

#endif