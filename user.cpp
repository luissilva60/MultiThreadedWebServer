#include <crow.h>
#include <jwt.h>
#include <pqxx/pqxx>
// Define a struct to hold user data
struct User {
    int id;
    std::string email;
    std::string password;
};
std::string generateJwtToken(int user_id);

// Define a secret key for JWT token signing and verification
const std::string secret = "my_secret_key";

// Define a middleware to authenticate requests using JWT tokens
struct AuthMiddleware {
    void before_handle(crow::request& req, crow::response& res, std::function<void()> next) {
        // Get the JWT token from the "Authorization" header
        auto authHeader = req.get_header_value("Authorization");
        if (authHeader.empty() || !jwt::token::is_valid(authHeader, jwt::default_clock{}, jwt::params{secret})) {
            res.code = 401;
            res.body = "Unauthorized";
            res.end();
            return;
        }

        // Token is valid, call the next middleware
        next();
    }
};

// Define a function to generate JWT tokens
std::string generateJwtToken(const User& user) {
    return jwt::create()
        .set_issuer("my_issuer")
        .set_subject(std::to_string(user.id))
        .set_payload_claim("email", user.email)
        .sign(jwt::algorithm::hs256{secret});
}


CROW_ROUTE(app, "/login")
    .methods("POST"_method)
    ([](const crow::request& req) {
        try {
            // Parse the JSON body of the request
            crow::json::rvalue json_data = crow::json::load(req.body);
            if (!json_data) {
                throw std::invalid_argument("Invalid JSON body");
            }

            // Get the email and password from the JSON data
            std::string email = json_data["email"].s();
            std::string password = json_data["password"].s();

            // Set up connection parameters
            std::string host = "ec2-54-76-132-202.eu-west-1.compute.amazonaws.com";
            std::string port = "5432";
            std::string dbname = "d7mqnaojamm7jr";
            std::string user = "yqbjkzhuzzezfs";
            std::string password = "5433343d3797e83769200119eda5399511d00ef9df9c7d682868e59b1d729c7e";

            // Create connection string
            std::string conn_string = "host=" + host + " port=" + port + " dbname=" + dbname + " user=" + user + " password=" + db_password;
            pqxx::connection conn(conn_string);

            // Query the database for the user with the given email and password
            pqxx::work txn(conn);
            pqxx::result result = txn.exec_params("SELECT user_id FROM users WHERE user_email = $1 AND user_password = $2", email, password);
            txn.commit();

            // If the user is found, generate a JWT token and return it in the response
            if (!result.empty()) {
                crow::json::wvalue json_token;
                json_token["token"] = generate_jwt_token(result[0]["user_id"].as<int>());
                crow::response response{json_token};
                response.set_header("Content-Type", "application/json");
                return response;
            } else {
                throw std::invalid_argument("Invalid email or password");
            }
        } catch (const std::exception& e) {
            crow::response response;
            response.code = 401;
            response.body = e.what();

            response.set_header("Content-Type", "text/plain");
            return response;
        }
    });
    
    