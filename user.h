#ifndef USER_H
#define USER_H

// Define a struct to hold user data
struct User {
    int id;
    std::string email;
    std::string password;
};

//Define a function to generate JWT tokens
std::string generateJwtToken(const User& user);

#endif 
