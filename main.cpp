
#define CROW_ENABLE_SSL
#include "crow.h"

#include <crow/logging.h>

#include <chrono>
#include <iomanip>
#include <fstream>
#include <signal.h>
#include <unistd.h>

#include <sstream>
#include <iostream>
#include <pqxx/pqxx>
#include <nlohmann/json.hpp>
#include <ctime>
#include <string>
using namespace std;
using namespace pqxx;


std::string conn_string ="";
using json = nlohmann::json;

std::ofstream log_file;

class FileLogHandler : public crow::ILogHandler {
public:
  FileLogHandler(std::ofstream& file) : log_file(file) {}

  void log(std::string message, crow::LogLevel level) override {
    auto now = std::chrono::system_clock::now();
    std::time_t current_time = std::chrono::system_clock::to_time_t(now);
    std::string time_str = std::ctime(&current_time);
    // Remove the trailing newline character from the time string
    time_str.erase(std::remove(time_str.begin(), time_str.end(), '\n'), time_str.end());

    std::string level_str;
  switch (level) {
    case crow::LogLevel::Debug:
      level_str = "[DEBUG]";
      break;
    case crow::LogLevel::Info:
      level_str = "[INFO]";
      break;
    case crow::LogLevel::Warning:
      level_str = "[WARNING]";
      break;
    case crow::LogLevel::Error:
      level_str = "[ERROR]";
      break;
    case crow::LogLevel::CRITICAL:
      level_str = "CRITICAL";
      break;
    default:
      level_str = "UNKNOWN";
      break;
  }
    log_file << time_str << " " << level_str << " " << message << std::endl;
  }

private:
  std::ofstream& log_file;
};

void doSomething(int i) {
    CROW_LOG_INFO << "Thread " << i << " is doing something..." ;
    std::this_thread::sleep_for(std::chrono::seconds(1)); // simulate some work
    CROW_LOG_INFO << "Thread " << i << " has finished doing something." ;
}

struct User {
    int user_id;
    std::string user_name;
    std::string user_password;
    std::string user_email;
    std::string user_gps;
    int user_role_id;
};

struct Completed {
    int completed_id;
    int completed_user_id;
    int completed_checkpoints_id;
    int completed_trail_id;
    int completed_challenged_id;
};


struct Challenge {
    int challenge_id;
    std::string challenge_name;
    std::string challenge_gps; // As a string instead of POINT data type
    int challenge_points;
    int challenge_trail_id;
    int challenge_area_id;
};

struct Trail {
    int trail_id;
    std::string trail_name;
    std::string trail_start;
    std::string trail_end;

};

struct Area {
    int area_id;
    std::string area_gps;
    std::string area_name;
};

struct Checkpoint {
    int checkpoint_id;
    std::string checkpoint_gps;
};

struct ExampleMiddleware  {
    std::string message;
    ExampleMiddleware() {
        message = "foo";
    }
    struct context {
        // Store the start time of the request processing
        std::chrono::system_clock::time_point start_time;
        // Store the IP address of the client making the request
        std::string client_ip;
    };

    

    void setMessage(std::string newMsg) {
        message = newMsg;
    }

    void before_handle(crow::request& req, crow::response& res, context& ctx){
    // Log the start time of the request processing
    ctx.start_time = std::chrono::system_clock::now();
    // Log the IP address of the client making the request
    ctx.client_ip = req.remote_ip_address;



    CROW_LOG_INFO << "[REQUEST] " << crow::method_name(req.method) << " " << req.url << " FROM " << std::string(ctx.client_ip)<< " ON THREAD " << std::this_thread::get_id();;

}


    void after_handle(crow::request& req, crow::response& res, context& ctx) {
        // Log the duration of the request processing
        auto end_time = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsed_seconds = end_time - ctx.start_time;

        CROW_LOG_INFO << "[RESPONSE] " << std::to_string(res.code) << " " << crow::method_name(req.method) << " " << req.url
            << " FROM " << std::string(ctx.client_ip) << " IN " << elapsed_seconds.count() << " SECONDS";
    }
};
int main()
{
    std::string sql;

    // Get the current date and time
    auto now1 = std::chrono::system_clock::now();
    std::time_t current_time = std::chrono::system_clock::to_time_t(now1);

    // Create a formatted string with the current date and time
    std::stringstream filename_ss;
    filename_ss << "logs/CROW-BUILD-" << std::put_time(std::localtime(&current_time), "%d-%m-%Y, %H:%M:%S") << ".txt";
    std::string filename = filename_ss.str();

    // Open the log file for writing
    log_file.open(filename);

    if (!log_file.is_open()) {
        std::cerr << "Failed to open log file." << std::endl;
        return -1;
    }

    // Create the custom log handler
    FileLogHandler file_log_handler(log_file);

    // Configure Crow to use the custom log handler
    crow::logger::setHandler(&file_log_handler);
    std::signal(SIGINT, [](int) {
    if (log_file.is_open()) {
        log_file.close();
    }
    std::exit(0);
    });
    try{
        // Set up connection parameters
        std::string host = "ec2-54-76-132-202.eu-west-1.compute.amazonaws.com";
        std::string port = "5432";
        std::string dbname = "d7mqnaojamm7jr";
        std::string user = "yqbjkzhuzzezfs";
        std::string password = "5433343d3797e83769200119eda5399511d00ef9df9c7d682868e59b1d729c7e";

        // Create connection string
        conn_string = "host=" + host + " port=" + port + " dbname=" + dbname + " user=" + user + " password=" + password;
        
        // Create a connection to the database
        pqxx::connection conn(conn_string);

        if (conn.is_open())
        {
            std::cout << "Connected to database successfully!" << std::endl;
            
        }else{
            std::cout << "Connection to database failed" << std::endl;
        }

        sql = "SELECT user_id, user_email, user_password FROM users";
        pqxx::work w(conn);
       pqxx::result rows = w.exec(sql);
        /* List down all the records */
     for (int i = 0 ; i < rows.size(); i++){
        int id = rows[i].at("user_id").as<int>();
        string email = rows[i].at("user_email").as<string>();
        string password = rows[i].at("user_password").as<string>();
        cout << id << ", " << email << ", " << password << endl;
     }
      cout << "Operation done successfully" << endl;
      conn.disconnect ();
   
    } catch (const std::exception &e) {
        cerr << e.what() << std::endl;
        return 1;
   }
    
    // Create the logger object
    //Logger logger("crow");
    const size_t num_threads = 10;
    asio::thread_pool thread_pool(num_threads);
    crow::App<ExampleMiddleware> app(&thread_pool);

    
    

    app.get_middleware<ExampleMiddleware>().setMessage("hello");
    
    // added for timing
    auto start = std::chrono::system_clock::now();

    // added for timestamp
    std::time_t now = std::chrono::system_clock::to_time_t(start);

    CROW_LOG_INFO << "Starting threads at " << std::put_time(std::localtime(&now), "%Y-%m-%d %H:%M:%S");

    for (int i = 0; i < num_threads; i++)
    {
        asio::post(thread_pool, [i]()
                   {
                auto start = std::chrono::high_resolution_clock::now();
                doSomething(i);
                auto end = std::chrono::high_resolution_clock::now();
                CROW_LOG_INFO << "Thread " << i << " finished in "
                    << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
                    << " milliseconds"; });
    }

    // added for timing
    auto end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - start;

    // added for timestamp
    now = std::chrono::system_clock::to_time_t(end);

    CROW_LOG_INFO << "All threads finished at " << std::put_time(std::localtime(&now), "%Y-%m-%d %H:%M:%S");
    CROW_LOG_INFO << "Elapsed time: " << elapsed_seconds.count() << " seconds";

    CROW_ROUTE(app, "/")
        .name("hello")
    ([]{
        return "Hello World!";
    });

    CROW_ROUTE(app, "/about")
    ([](){
        return "About Crow example.";
    });

    // a request to /path should be forwarded to /path/
    CROW_ROUTE(app, "/path/")
    ([](){
        return "Trailing slash test case..";
    });
    
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
            pqxx::connection conn(conn_string);

            // Query the database for the user with the given email and password
            pqxx::work txn(conn);
            pqxx::result result = txn.exec_params("SELECT user_id, user_email, user_password FROM users WHERE user_email = $1 AND user_password = $2", email, password);
            txn.commit();

            // If the user is found, generate a JWT token and return it in the response along with the user object
            if (!result.empty()) {
                int user_id = result[0]["user_id"].as<int>();
                
                std::string user_email = result[0]["user_email"].as<std::string>();
                // Generate JWT token
                //std::string jwt_token = generate_jwt(user_id);

                // Create JSON object with user and token
                crow::json::wvalue json_response;
                json_response["user_id"] = user_id;
                
                //json_response["jwt"] = jwt_token;

                crow::response response(json_response);
                response.code = 200;
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

    CROW_ROUTE(app, "/users")
    ([]{
        pqxx::connection conn(conn_string);
        
        try {
            pqxx::work txn(conn);
            pqxx::result result = txn.exec("SELECT user_id, user_name, user_password, user_email, user_gps, user_role_id FROM users");
            txn.commit();

            crow::json::wvalue json_data = crow::json::wvalue(crow::json::type::Object);
            for (const auto& row : result) {
                User user;
                user.user_id = row["user_id"].as<int>();
                user.user_name = row["user_name"].as<std::string>();
                user.user_password = row["user_password"].as<std::string>();
                user.user_email = row["user_email"].as<std::string>();
                user.user_gps = row["user_gps"].as<std::string>();
                user.user_role_id = row["user_role_id"].as<int>();


                crow::json::wvalue user_json;
                user_json["user_id"] = user.user_id;
                user_json["user_name"] = user.user_name;
                user_json["user_password"] = user.user_password;
                user_json["user_email"] = user.user_email;
                user_json["user_gps"] = user.user_gps;
                user_json["user_role_id"] = user.user_role_id;
                if (!row["user_id"].is_null()) {
                    json_data[std::to_string(user.user_id)] = std::move(user_json);
                }
            }

            crow::response response{json_data};
        
            response.set_header("Content-Type", "application/json");
            return response;
        } catch (const std::exception& e) {
            crow::response response;
            response.code = 500;
            response.body = e.what();
            
            response.set_header("Content-Type", "text/plain");
            return response;
        }
    });
    CROW_ROUTE(app, "/users/<int>")
    .methods("GET"_method)
    ([&](const crow::request& req, int user_id) {
        pqxx::connection conn(conn_string);
        try {
            // Check if user exists
            pqxx::work txn(conn);
            pqxx::result result = txn.exec_params("SELECT user_id, user_name, user_email, user_gps, user_role_id FROM users WHERE user_id = $1", user_id);
            txn.commit();

            // If user doesn't exist, return 404 response
            if (result.empty()) {
                crow::response response;
                response.code = 404;
                response.body = "User not found";
                response.set_header("Content-Type", "text/plain");
                return response;
            }

            // Extract user data and construct response
            int id = result[0]["user_id"].as<int>();
            std::string name = result[0]["user_name"].as<std::string>();
            std::string email = result[0]["user_email"].as<std::string>();
            std::string gps = result[0]["user_gps"].as<std::string>();
            int role_id = result[0]["user_role_id"].as<int>();

            crow::json::wvalue json_data;
            json_data["user_id"] = id;
            json_data["user_name"] = name;
            json_data["user_email"] = email;
            json_data["user_gps"] = gps;
            json_data["user_role_id"] = role_id;

            crow::response response{json_data};
            response.code = 200;
            response.set_header("Content-Type", "application/json");
            return response;
        } catch (const std::exception& e) {
            crow::response response;
            response.code = 500;
            response.body = e.what();
            response.set_header("Content-Type", "text/plain");
            return response;
        }
    });

    CROW_ROUTE(app, "/users")
    .methods("PUT"_method)
    ([&](const crow::request& req){
        pqxx::connection conn(conn_string);
        
        try {
            // Parse JSON payload
            auto json_payload = crow::json::load(req.body);
            if (!json_payload) {
                throw std::runtime_error("Invalid JSON payload");
            }

            // Extract user ID from payload
            int user_id = json_payload["user_id"].i();

            // Start outer transaction
            pqxx::work txn(conn);

            // Check if user exists
            pqxx::result check_result = txn.exec_params("SELECT COUNT(*) FROM users WHERE user_id = $1", user_id);
            int count = check_result[0]["count"].as<int>();
            if (count == 0) {
                throw std::runtime_error("User not found");
            }

            // Start inner transaction for update
            pqxx::subtransaction update_txn(txn, "update");

            // Update user
            pqxx::result result = update_txn.exec_params("UPDATE users SET user_name = $2, user_password = $3, user_email = $4, user_gps = $5, user_role_id = $6 WHERE user_id = $1 RETURNING user_id", 
                                                   user_id,
                                                   std::string(json_payload["user_name"].s()),
                                                   std::string(json_payload["user_password"].s()),
                                                   std::string(json_payload["user_email"].s()),
                                                   std::string(json_payload["user_gps"].s()),
                                                   json_payload["user_role_id"].i());
            update_txn.commit();

            // Commit outer transaction
            txn.commit();

            crow::json::wvalue json_data;
            json_data["user_id"] = result[0]["user_id"].as<int>();

            crow::response response{json_data};
            response.code = 200;
            response.set_header("Content-Type", "application/json");
            return response;
        } catch (const std::exception& e) {
            conn.disconnect();
            crow::response response;
            response.code = 500;
            response.body = e.what();
            response.set_header("Content-Type", "text/plain");
            return response;
        }
    });


    CROW_ROUTE(app, "/user")
    .methods("POST"_method)
    ([](const crow::request& req){
        pqxx::connection conn(conn_string);
        
        try {
            // Parse JSON payload
            auto json_payload = crow::json::load(req.body);
            if (!json_payload) {
                throw std::runtime_error("Invalid JSON payload");
            }

            // Start outer transaction
            pqxx::work txn(conn);

            // Insert new user
            pqxx::result result = txn.exec_params("INSERT INTO users (user_name, user_password, user_email, user_gps, user_role_id) VALUES ($1, $2, $3, $4, $5) RETURNING user_id",
                                                std::string(json_payload["user_name"].s()),
                                                std::string(json_payload["user_password"].s()),
                                                std::string(json_payload["user_email"].s()),
                                                std::string(json_payload["user_gps"].s()),
                                                json_payload["user_role_id"].i());

            // Commit transaction
            txn.commit();

            // Create response JSON
            crow::json::wvalue json_data;
            json_data["user_id"] = result[0]["user_id"].as<int>();
            
            crow::response response{json_data};
            response.code = 201;
            response.set_header("Content-Type", "application/json");

            return response;
        } catch (const std::exception& e) {
            conn.disconnect();
            crow::response response;
            response.code = 500;
            response.body = e.what();
            response.set_header("Content-Type", "text/plain");
            return response;
        }
        
        
    });


    CROW_ROUTE(app, "/users/<int>")
    .methods("DELETE"_method)
    ([&](int user_id){
        pqxx::connection conn(conn_string);
        
        try {
            pqxx::work txn(conn);
            pqxx::result result = txn.exec_params("WITH deleted_completed AS (DELETE FROM completed WHERE completed_user_id = $1 RETURNING completed_user_id) DELETE FROM users WHERE user_id = $1 RETURNING user_id", user_id);
            txn.commit();

            if (result.empty()) {
                crow::response response;
                response.code = 404;
                response.body = "User not found";
                response.set_header("Content-Type", "text/plain");
                return response;
            }

            crow::json::wvalue json_data;
            json_data["user_id"] = result[0]["user_id"].as<int>();

            crow::response response{json_data};
            response.code = 200;
            response.set_header("Content-Type", "application/json");
            return response;
        } catch (const std::exception& e) {
            crow::response response;
            response.code = 500;
            response.body = e.what();
            
            response.set_header("Content-Type", "text/plain");
            return response;
        }
    });

/*                                                         COMPLETED ROUTES                                     */
    CROW_ROUTE(app, "/completed")
    ([]{
        pqxx::connection conn(conn_string);
       
        try {
            pqxx::work txn(conn);
            pqxx::result result = txn.exec("SELECT completed_id, completed_user_id, completed_checkpoints_id, completed_trail_id, completed_challenged_id  FROM completed");
            txn.commit();

            crow::json::wvalue json_data;
            for (const auto& row : result) {
                Completed completed;
                completed.completed_id = row["completed_id"].as<int>();
                completed.completed_user_id = row["completed_user_id"].as<int>();
                completed.completed_checkpoints_id = row["completed_checkpoints_id"].as<int>();
                completed.completed_trail_id = row["completed_trail_id"].as<int>();
                completed.completed_challenged_id = row["completed_challenged_id"].as<int>();

                crow::json::wvalue completed_json;
                completed_json["completed_id"] = completed.completed_id;
                completed_json["completed_user_id"] = completed.completed_user_id;
                completed_json["completed_checkpoints_id"] = completed.completed_checkpoints_id;
                completed_json["completed_trail_id"] = completed.completed_trail_id;
                completed_json["completed_challenged_id"] = completed.completed_challenged_id;
                json_data[completed.completed_id] = std::move(completed_json);
            }

            crow::response response{json_data};
       
            response.set_header("Content-Type", "application/json");
            return response;
        } catch (const std::exception& e) {
            crow::response response;
            response.code = 500;
            response.body = e.what();
           
            response.set_header("Content-Type", "text/plain");
            return response;
        }
    });
    

    CROW_ROUTE(app, "/completed/<int>")
    .methods("GET"_method)
    ([&](const crow::request& req, int completed_id) {
        pqxx::connection conn(conn_string);
        try {
            // Check if completed entry exists
            pqxx::work txn(conn);
            pqxx::result result = txn.exec_params("SELECT completed_id, completed_user_id, completed_checkpoints_id, completed_trail_id, completed_challenged_id FROM completed WHERE completed_id = $1", completed_id);
            txn.commit();

            // If completed entry doesn't exist, return 404 response
            if (result.empty()) {
                crow::response response;
                response.code = 404;
                response.body = "Completed entry not found";
                response.set_header("Content-Type", "text/plain");
                return response;
            }

            // Extract completed entry data and construct response
            int id = result[0]["completed_id"].as<int>();
            int user_id = result[0]["completed_user_id"].as<int>();
            int checkpoints_id = result[0]["completed_checkpoints_id"].as<int>();
            int trail_id = result[0]["completed_trail_id"].as<int>();
            int challenged_id = result[0]["completed_challenged_id"].as<int>();

            crow::json::wvalue json_data;
            json_data["completed_id"] = id;
            json_data["completed_user_id"] = user_id;
            json_data["completed_checkpoints_id"] = checkpoints_id;
            json_data["completed_trail_id"] = trail_id;
            json_data["completed_challenged_id"] = challenged_id;

            crow::response response{json_data};
            response.code = 200;
            response.set_header("Content-Type", "application/json");
            return response;
        } catch (const std::exception& e) {
            crow::response response;
            response.code = 500;
            response.body = e.what();
            response.set_header("Content-Type", "text/plain");
            return response;
        }
    });

    CROW_ROUTE(app, "/completed")
    .methods("PUT"_method)
    ([&](const crow::request& req){
        pqxx::connection conn(conn_string);

        try {
            // Parse JSON payload
            auto json_payload = crow::json::load(req.body);
            if (!json_payload) {
                throw std::runtime_error("Invalid JSON payload");
            }

            // Extract completed ID from payload
            int completed_id = json_payload["completed_id"].i();

            // Start outer transaction
            pqxx::work txn(conn);

            // Check if completed record exists
            pqxx::result check_result = txn.exec_params("SELECT COUNT(*) FROM completed WHERE completed_id = $1", completed_id);
            int count = check_result[0]["count"].as<int>();
            if (count == 0) {
                throw std::runtime_error("Completed record not found");
            }

            // Start inner transaction for update
            pqxx::subtransaction update_txn(txn, "update");

            // Update completed record
            pqxx::result result = update_txn.exec_params("UPDATE completed SET completed_user_id = $2, completed_checkpoints_id = $3, completed_trail_id = $4, completed_challenged_id = $5 WHERE completed_id = $1 RETURNING completed_id",
                                                   completed_id,
                                                   json_payload["completed_user_id"].i(),
                                                   json_payload["completed_checkpoints_id"].i(),
                                                   json_payload["completed_trail_id"].i(),
                                                   json_payload["completed_challenged_id"].i());
            update_txn.commit();

            // Commit outer transaction
            txn.commit();

            crow::json::wvalue json_data;
            json_data["completed_id"] = result[0]["completed_id"].as<int>();

            crow::response response{json_data};
            response.code = 200;
            response.set_header("Content-Type", "application/json");
            return response;
        } catch (const std::exception& e) {
            conn.disconnect();
            crow::response response;
            response.code = 500;
            response.body = e.what();
            response.set_header("Content-Type", "text/plain");
            return response;
        }
    });

    CROW_ROUTE(app, "/completed/<int>")
    .methods("DELETE"_method)
    ([&](int completed_id){
        pqxx::connection conn(conn_string);
       
        try {
            pqxx::work txn(conn);
            pqxx::result result = txn.exec_params("DELETE FROM completed WHERE completed_id = $1 RETURNING completed_id", completed_id);
            txn.commit();

            if (result.empty()) {
                crow::response response;
                response.code = 404;
                response.body = "Completed record not found";
                response.set_header("Content-Type", "text/plain");
                return response;
            }

            crow::json::wvalue json_data;
            json_data["completed_id"] = result[0]["completed_id"].as<int>();

            crow::response response{json_data};
            response.code = 200;
            response.set_header("Content-Type", "application/json");
            return response;
        } catch (const std::exception& e) {
            crow::response response;
            response.code = 500;
            response.body = e.what();
           
            response.set_header("Content-Type", "text/plain");
            return response;
        }
    });
    CROW_ROUTE(app, "/completed")
    .methods("POST"_method)
    ([](const crow::request& req){
        pqxx::connection conn(conn_string);

        try {
            // Parse JSON payload
            auto json_payload = crow::json::load(req.body);
            if (!json_payload) {
                throw std::runtime_error("Invalid JSON payload");
            }

            // Start outer transaction
            pqxx::work txn(conn);

            // Insert new completed
            pqxx::result result = txn.exec_params("INSERT INTO completed (completed_user_id, completed_checkpoints_id, completed_trail_id, completed_challenged_id) VALUES ($1, $2, $3, $4) RETURNING completed_id",
                                                json_payload["completed_user_id"].i(),
                                                json_payload["completed_checkpoints_id"].i(),
                                                json_payload["completed_trail_id"].i(),
                                                json_payload["completed_challenged_id"].i());

            // Commit transaction
            txn.commit();

            // Create response JSON
            crow::json::wvalue json_data;
            json_data["completed_id"] = result[0]["completed_id"].as<int>();

            crow::response response{json_data};
            response.code = 201;
            response.set_header("Content-Type", "application/json");

            return response;
        } catch (const std::exception& e) {
            conn.disconnect();
            crow::response response;
            response.code = 500;
            response.body = e.what();
            response.set_header("Content-Type", "text/plain");
            return response;
        }
    });
    
/*                                                         TRAILS ROUTES                                     */

    CROW_ROUTE(app, "/trails")
    ([]{
        pqxx::connection conn(conn_string);

        try {
            pqxx::work txn(conn);
            pqxx::result result = txn.exec("SELECT trail_id, trail_name, trail_start, trail_end FROM trails");
            txn.commit();

            crow::json::wvalue json_data;
            for (const auto& row : result) {
                Trail trail;
                trail.trail_id = row["trail_id"].as<int>();
                trail.trail_name = row["trail_name"].as<std::string>();
                trail.trail_start = row["trail_start"].as<std::string>();
                trail.trail_end = row["trail_end"].as<std::string>();
            
                crow::json::wvalue trail_json;
                trail_json["trail_id"] = trail.trail_id;
                trail_json["trail_name"] = trail.trail_name;
                trail_json["trail_start"] = trail.trail_start;
                trail_json["trail_end"] = trail.trail_end;
        
                json_data[trail.trail_id] = std::move(trail_json);
            }

            crow::response response{json_data};

            response.set_header("Content-Type", "application/json");
            return response;
        } catch (const std::exception& e) {
            crow::response response;
            response.code = 500;
            response.body = e.what();

            response.set_header("Content-Type", "text/plain");
            return response;
        }
    });


   

    CROW_ROUTE(app, "/trails/<int>")
    .methods("GET"_method)
    ([&](const crow::request& req, int trail_id) {
        pqxx::connection conn(conn_string);
        try {
            pqxx::work txn(conn);
            pqxx::result result = txn.exec_params("SELECT trail_id, trail_name, trail_start, trail_end FROM trails WHERE trail_id = $1", trail_id);
            txn.commit();

            if (result.empty()) {
                crow::response response;
                response.code = 404;
                response.body = "Trail not found";
                response.set_header("Content-Type", "text/plain");
                return response;
            }
            
            Trail trail;
            trail.trail_id = result[0]["trail_id"].as<int>();
            trail.trail_name = result[0]["trail_name"].as<std::string>();
            trail.trail_start = result[0]["trail_start"].as<std::string>();
            trail.trail_end = result[0]["trail_end"].as<std::string>();
            
            crow::json::wvalue trail_json;
            trail_json["trail_id"] = trail.trail_id;
            trail_json["trail_name"] = trail.trail_name;
            trail_json["trail_start"] = trail.trail_start;
            trail_json["trail_end"] = trail.trail_end;
            
            

            crow::response response{trail_json};
            response.code = 200;
            response.set_header("Content-Type", "application/json");
            return response;
        } catch (const std::exception& e) {
            crow::response response;
            response.code = 500;
            response.body = e.what();
            response.set_header("Content-Type", "text/plain");
            return response;
        }
    });


    CROW_ROUTE(app, "/trails")
    .methods("PUT"_method)
    ([&](const crow::request& req){
        pqxx::connection conn(conn_string);
       
        try {
            // Parse JSON payload
            auto json_payload = crow::json::load(req.body);
            if (!json_payload) {
                throw std::runtime_error("Invalid JSON payload");
            }

            // Extract trail data from payload
            Trail trail;
            trail.trail_id = json_payload["trail_id"].i();
            trail.trail_name = std::string(json_payload["trail_name"].s());
            trail.trail_start = std::string(json_payload["trail_start"].s());
            trail.trail_end = std::string(json_payload["trail_end"].s());
            

            // Start outer transaction
            pqxx::work txn(conn);

            // Check if trail exists
            pqxx::result check_result = txn.exec_params("SELECT COUNT(*) FROM trails WHERE trail_id = $1", trail.trail_id);
            int count = check_result[0]["count"].as<int>();
            if (count == 0) {
                throw std::runtime_error("Trail not found");
            }

            // Start inner transaction for update
            pqxx::subtransaction update_txn(txn, "update");

            // Update trail
            pqxx::result result = update_txn.exec_params("UPDATE trails SET trail_name = $2, trail_start = $3, trail_end = $4 WHERE trail_id = $1 RETURNING trail_id",
                                                   trail.trail_id,
                                                   trail.trail_name,
                                                   trail.trail_start,
                                                   trail.trail_end
                                                   );
            update_txn.commit();

            // Commit outer transaction
            txn.commit();

            crow::json::wvalue json_data;
            json_data["trail_id"] = result[0]["trail_id"].as<int>();

            crow::response response{json_data};
            response.code = 200;
            response.set_header("Content-Type", "application/json");
            return response;
        } catch (const std::exception& e) {
            conn.disconnect();
            crow::response response;
            response.code = 500;
            response.body = e.what();
            response.set_header("Content-Type", "text/plain");
            return response;
        }
    });



    CROW_ROUTE(app, "/trails/<int>")
    .methods("DELETE"_method)
    ([&](int trail_id){
        pqxx::connection conn(conn_string);
       
        try {
            pqxx::work txn(conn);
            pqxx::result result = txn.exec_params("WITH deleted_completed AS (DELETE FROM completed WHERE completed_trail_id = $1 RETURNING completed_trail_id) DELETE FROM trails WHERE trail_id = $1 RETURNING trail_id, trail_name, trail_start, trail_end", trail_id);
            txn.commit();

            if (result.empty()) {
                crow::response response;
                response.code = 404;
                response.body = "Trail not found";
                response.set_header("Content-Type", "text/plain");
                return response;
            }

            Trail trail;
            trail.trail_id = result[0]["trail_id"].as<int>();
            trail.trail_name = result[0]["trail_name"].as<std::string>();
            trail.trail_start = result[0]["trail_start"].as<std::string>();
            trail.trail_end = result[0]["trail_end"].as<std::string>();
            

            crow::json::wvalue json_data;
            json_data["trail_id"] = result[0]["trail_id"].as<int>();

            crow::response response{json_data};
            response.code = 200;
            response.set_header("Content-Type", "application/json");

            return response;
        } catch (const std::exception& e) {
            crow::response response;
            response.code = 500;
            response.body = e.what();
           
            response.set_header("Content-Type", "text/plain");
            return response;
        } 
    });

/*                                                         AREAS ROUTES                                     */

    CROW_ROUTE(app, "/areas")
    ([]{
        pqxx::connection conn(conn_string);
       
        try {
            pqxx::work txn(conn);
            pqxx::result result = txn.exec("SELECT area_id, area_name, area_gps FROM area");
            txn.commit();

            crow::json::wvalue json_data;
            for (const auto& row : result) {
                Area area;
                area.area_id = row["area_id"].as<int>();
                area.area_name = row["area_name"].as<std::string>();
                area.area_gps = row["area_gps"].as<std::string>();

                crow::json::wvalue area_json;
                area_json["area_id"] = area.area_id;
                area_json["area_name"] = area.area_name;
                area_json["area_gps"] = area.area_gps;
                json_data[area.area_id] = std::move(area_json);
            }

            crow::response response{json_data};
       
            response.set_header("Content-Type", "application/json");
            return response;
        } catch (const std::exception& e) {
            crow::response response;
            response.code = 500;
            response.body = e.what();
           
            response.set_header("Content-Type", "text/plain");
            return response;
        }
    });

    CROW_ROUTE(app, "/areas/<int>")
    .methods("GET"_method)
    ([&](const crow::request& req, int area_id) {
        pqxx::connection conn(conn_string);
        try {
            // Check if area exists
            pqxx::work txn(conn);
            pqxx::result result = txn.exec_params("SELECT area_id, area_name, area_gps FROM area WHERE area_id = $1", area_id);
            txn.commit();

            // If area doesn't exist, return 404 response
            if (result.empty()) {
                crow::response response;
                response.code = 404;
                response.body = "Area not found";
                response.set_header("Content-Type", "text/plain");
                return response;
            }

            // Extract area data and construct response
            int id = result[0]["area_id"].as<int>();
            std::string name = result[0]["area_name"].as<std::string>();
            std::string gps = result[0]["area_gps"].as<std::string>();

            crow::json::wvalue json_data;
            json_data["area_id"] = id;
            json_data["area_name"] = name;
            json_data["area_gps"] = gps;

            crow::response response{json_data};
            response.code = 200;
            response.set_header("Content-Type", "application/json");
            return response;
        } catch (const std::exception& e) {
            crow::response response;
            response.code = 500;
            response.body = e.what();
            response.set_header("Content-Type", "text/plain");
            return response;
        }
    });

    CROW_ROUTE(app, "/areas")
    .methods("PUT"_method)
    ([&](const crow::request& req){
        pqxx::connection conn(conn_string);
       
        try {
            // Parse JSON payload
            auto json_payload = crow::json::load(req.body);
            if (!json_payload) {
                throw std::runtime_error("Invalid JSON payload");
            }

            // Extract area ID from payload
            int area_id = json_payload["area_id"].i();

            // Start outer transaction
            pqxx::work txn(conn);

            // Check if area exists
            pqxx::result check_result = txn.exec_params("SELECT COUNT(*) FROM area WHERE area_id = $1", area_id);
            int count = check_result[0]["count"].as<int>();
            if (count == 0) {
                throw std::runtime_error("Area not found");
            }

            // Start inner transaction for update
            pqxx::subtransaction update_txn(txn, "update");

            // Update area
            pqxx::result result = update_txn.exec_params("UPDATE area SET area_name = $2, area_type = $3, area_location = $4 WHERE area_id = $1 RETURNING area_id",
                                                   area_id,
                                                   std::string(json_payload["area_name"].s()),
                                                   std::string(json_payload["area_type"].s()),
                                                   std::string(json_payload["area_location"].s()));
            update_txn.commit();

            // Commit outer transaction
            txn.commit();

            crow::json::wvalue json_data;
            json_data["area_id"] = result[0]["area_id"].as<int>();

            crow::response response{json_data};
            response.code = 200;
            response.set_header("Content-Type", "application/json");
            return response;
        } catch (const std::exception& e) {
            conn.disconnect();
            crow::response response;
            response.code = 500;
            response.body = e.what();
            response.set_header("Content-Type", "text/plain");
            return response;
        }
    });

    CROW_ROUTE(app, "/areas/<int>")
    .methods("DELETE"_method)
    ([&](int area_id){
        pqxx::connection conn(conn_string);
       
        try {
            pqxx::work txn(conn);
            pqxx::result result = txn.exec_params("DELETE FROM area WHERE area_id = $1 RETURNING area_id", area_id);
            txn.commit();

            if (result.empty()) {
                crow::response response;
                response.code = 404;
                response.body = "Area not found";
                response.set_header("Content-Type", "text/plain");
                return response;
            }

            crow::json::wvalue json_data;
            json_data["area_id"] = result[0]["area_id"].as<int>();

            crow::response response{json_data};
            response.code = 200;
            response.set_header("Content-Type", "application/json");
            return response;
        } catch (const std::exception& e) {
            crow::response response;
            response.code = 500;
            response.body = e.what();
           
            response.set_header("Content-Type", "text/plain");
            return response;
        }
    });

    

/*                                                         CHECKPOINTS ROUTES                                     */

CROW_ROUTE(app, "/checkpoints")
    ([]{
        pqxx::connection conn(conn_string);
       
        try {
            pqxx::work txn(conn);
            pqxx::result result = txn.exec("SELECT checkpoint_id, ST_AsText(checkpoint_gps) FROM checkpoints");
            txn.commit();

            crow::json::wvalue json_data;
            for (const auto& row : result) {
                Checkpoint checkpoint;
                checkpoint.checkpoint_id = row["checkpoint_id"].as<int>();
                checkpoint.checkpoint_gps = row["checkpoint_id"].as<std::string>();
                crow::json::wvalue checkpoint_json;
                checkpoint_json["checkpoint_id"] = checkpoint.checkpoint_id;
                checkpoint_json["checkpoint_gps"] = checkpoint.checkpoint_gps;
               
                json_data[checkpoint.checkpoint_id] = std::move(checkpoint_json);
            }

            crow::response response{json_data};
       
            response.set_header("Content-Type", "application/json");
            return response;
        } catch (const std::exception& e) {
            crow::response response;
            response.code = 500;
            response.body = e.what();
           
            response.set_header("Content-Type", "text/plain");
            return response;
        }
    });

CROW_ROUTE(app, "/checkpoints/<int>")
    .methods("GET"_method)
    ([&](const crow::request& req, int checkpoint_id) {
        pqxx::connection conn(conn_string);
        try {
            // Check if checkpoint exists
            pqxx::work txn(conn);
            pqxx::result result = txn.exec_params("SELECT checkpoint_id, checkpoint_gps FROM checkpoints WHERE checkpoint_id = $1", checkpoint_id);
            txn.commit();

            // If checkpoint doesn't exist, return 404 response
            if (result.empty()) {
                crow::response response;
                response.code = 404;
                response.body = "Checkpoint not found";
                response.set_header("Content-Type", "text/plain");
                return response;
            }

            // Extract checkpoint data and construct response
            int id = result[0]["checkpoint_id"].as<int>();
            std::string gps = result[0]["checkpoint_gps"].as<std::string>();

            crow::json::wvalue json_data;
            json_data["checkpoint_id"] = id;
            json_data["checkpoint_gps"] = gps;

            crow::response response{json_data};
            response.code = 200;
            response.set_header("Content-Type", "application/json");
            return response;
        } catch (const std::exception& e) {
            crow::response response;
            response.code = 500;
            response.body = e.what();
            response.set_header("Content-Type", "text/plain");
            return response;
        }
    });

CROW_ROUTE(app, "/checkpoints")
    .methods("PUT"_method)
    ([&](const crow::request& req){
        pqxx::connection conn(conn_string);

        try {
            // Parse JSON payload
            auto json_payload = crow::json::load(req.body);
            if (!json_payload) {
                throw std::runtime_error("Invalid JSON payload");
            }

            // Extract checkpoint ID from payload
            int checkpoint_id = json_payload["checkpoint_id"].i();

            // Start outer transaction
            pqxx::work txn(conn);

            // Check if checkpoint exists
            pqxx::result check_result = txn.exec_params("SELECT COUNT(*) FROM checkpoints WHERE checkpoint_id = $1", checkpoint_id);
            int count = check_result[0]["count"].as<int>();
            if (count == 0) {
                throw std::runtime_error("Checkpoint not found");
            }

            // Start inner transaction for update
            pqxx::subtransaction update_txn(txn, "update");

            // Update checkpoint
            pqxx::result result = update_txn.exec_params("UPDATE checkpoints SET c, checkpoint_gps = $2 WHERE checkpoint_id = $1 RETURNING checkpoint_id",
                                                   checkpoint_id,
                                                   std::string(json_payload["checkpoint_gps"].s())
                                                   );
            update_txn.commit();

            // Commit outer transaction
            txn.commit();

            crow::json::wvalue json_data;
            json_data["checkpoint_id"] = result[0]["checkpoint_id"].as<int>();

            crow::response response{json_data};
            response.code = 200;
            response.set_header("Content-Type", "application/json");
            return response;
        } catch (const std::exception& e) {
            conn.disconnect();
            crow::response response;
            response.code = 500;
            response.body = e.what();
            response.set_header("Content-Type", "text/plain");
            return response;
        }
    });


CROW_ROUTE(app, "/checkpoints/<int>")
    .methods("DELETE"_method)
    ([&](int checkpoint_id){
        pqxx::connection conn(conn_string);
       
        try {
            pqxx::work txn(conn);
            pqxx::result result = txn.exec_params("DELETE FROM checkpoints WHERE checkpoint_id = $1 RETURNING checkpoint_id", checkpoint_id);
            txn.commit();

            if (result.empty()) {
                crow::response response;
                response.code = 404;
                response.body = "Checkpoint not found";
                response.set_header("Content-Type", "text/plain");
                return response;
            }

            crow::json::wvalue json_data;
            json_data["checkpoint_id"] = result[0]["checkpoint_id"].as<int>();

            crow::response response{json_data};
            response.code = 200;
            response.set_header("Content-Type", "application/json");
            return response;
        } catch (const std::exception& e) {
            crow::response response;
            response.code = 500;
            response.body = e.what();
           
            response.set_header("Content-Type", "text/plain");
            return response;
        }
    });

/*                                                         Challenge ROUTES                                     */
    CROW_ROUTE(app, "/challenges")
    ([]{
        pqxx::connection conn(conn_string);

        try {
            pqxx::work txn(conn);
            pqxx::result result = txn.exec("SELECT challenge_id, challenge_name, challenge_gps, challenge_points, challenge_trail_id, challenge_area_id FROM challenges");
            txn.commit();

            crow::json::wvalue json_data;
            for (const auto& row : result) {
                Challenge challenge;
                challenge.challenge_id = row["challenge_id"].as<int>();
                challenge.challenge_name = row["challenge_name"].as<std::string>();
                challenge.challenge_gps = row["challenge_gps"].as<std::string>();
                challenge.challenge_points = row["challenge_points"].as<int>();
                challenge.challenge_trail_id = row["challenge_trail_id"].as<int>();
                challenge.challenge_area_id = row["challenge_area_id"].as<int>();

                crow::json::wvalue challenge_json;
                challenge_json["challenge_id"] = challenge.challenge_id;
                challenge_json["challenge_name"] = challenge.challenge_name;
                challenge_json["challenge_gps"] = challenge.challenge_gps;
                challenge_json["challenge_points"] = challenge.challenge_points;
                challenge_json["challenge_trail_id"] = challenge.challenge_trail_id;
                challenge_json["challenge_area_id"] = challenge.challenge_area_id;

                json_data[challenge.challenge_id] = std::move(challenge_json);
            }

            crow::response response{json_data};
            response.set_header("Content-Type", "application/json");
            return response;
        } catch (const std::exception& e) {
            crow::response response;
            response.code = 500;
            response.body = e.what();
            response.set_header("Content-Type", "text/plain");
            return response;
        }
    });

    
    CROW_ROUTE(app, "/challenges/<int>")
    .methods("GET"_method)
    ([&](const crow::request& req, int challenge_id) {
        pqxx::connection conn(conn_string);
        try {
            // Check if challenge exists
            pqxx::work txn(conn);
            pqxx::result result = txn.exec_params("SELECT challenge_id, challenge_name, challenge_gps, challenge_points, challenge_trail_id, challenge_area_id FROM challenges WHERE challenge_id = $1", challenge_id);
            txn.commit();

            // If challenge doesn't exist, return 404 response
            if (result.empty()) {
                crow::response response;
                response.code = 404;
                response.body = "Challenge not found";
                response.set_header("Content-Type", "text/plain");
                return response;
            }

            // Extract challenge data and construct response
            int id = result[0]["challenge_id"].as<int>();
            std::string name = result[0]["challenge_name"].as<std::string>();
            std::string gps = result[0]["challenge_gps"].as<std::string>();
            int points = result[0]["challenge_points"].as<int>();
            int trail_id = result[0]["challenge_trail_id"].as<int>();
            int area_id = result[0]["challenge_area_id"].as<int>();

            crow::json::wvalue json_data;
            json_data["challenge_id"] = id;
            json_data["challenge_name"] = name;
            json_data["challenge_gps"] = gps;
            json_data["challenge_points"] = points;
            json_data["challenge_trail_id"] = trail_id;
            json_data["challenge_area_id"] = area_id;

            crow::response response{json_data};
            response.code = 200;
            response.set_header("Content-Type", "application/json");
            return response;
        } catch (const std::exception& e) {
            crow::response response;
            response.code = 500;
            response.body = e.what();
            response.set_header("Content-Type", "text/plain");
            return response;
        }
    });

    

    CROW_ROUTE(app, "/challenges/<int>")
    .methods("DELETE"_method)
    ([&](int challenge_id){
        pqxx::connection conn(conn_string);
       
        try {
            pqxx::work txn(conn);
            pqxx::result result = txn.exec_params("DELETE FROM challenges WHERE challenge_id = $1 RETURNING challenge_id", challenge_id);
            txn.commit();

            if (result.empty()) {
                crow::response response;
                response.code = 404;
                response.body = "Challenge not found";
                response.set_header("Content-Type", "text/plain");
                return response;
            }

            crow::json::wvalue json_data;
            json_data["challenge_id"] = result[0]["challenge_id"].as<int>();

            crow::response response{json_data};
            response.code = 200;
            response.set_header("Content-Type", "application/json");
            return response;
        } catch (const std::exception& e) {
            crow::response response;
            response.code = 500;
            response.body = e.what();
           
            response.set_header("Content-Type", "text/plain");
            return response;
        }
    });
    // simple json response
    // To see it in action enter {ip}:18080/json
    CROW_ROUTE(app, "/json")
    ([]{
        crow::json::wvalue x;
        x["message"] = "Hello, World!";
        return x;
    });

    // To see it in action enter {ip}:18080/hello/{integer_between -2^32 and 100} and you should receive
    // {integer_between -2^31 and 100} bottles of beer!
    CROW_ROUTE(app,"/hello/<int>")
    ([](int count){
        if (count > 100)
            return crow::response(400);
        std::ostringstream os;
        os << count << " bottles of beer!";
        return crow::response(os.str());
    });

    // To see it in action submit {ip}:18080/add/1/2 and you should receive 3 (exciting, isn't it)
    CROW_ROUTE(app,"/add/<int>/<int>")
    ([](const crow::request& /*req*/, crow::response& res, int a, int b){
        std::ostringstream os;
        os << a+b;
        res.write(os.str());
        res.end();
    });

    // Compile error with message "Handler type is mismatched with URL paramters"
    //CROW_ROUTE(app,"/another/<int>")
    //([](int a, int b){
        //return crow::response(500);
    //});

    // more json example

    // To see it in action, I recommend to use the Postman Chrome extension:
    //      * Set the address to {ip}:18080/add_json
    //      * Set the method to post
    //      * Select 'raw' and then JSON
    //      * Add {"a": 1, "b": 1}
    //      * Send and you should receive 2

    // A simpler way for json example:
    //      * curl -d '{"a":1,"b":2}' {ip}:18080/add_json
    CROW_ROUTE(app, "/add_json")
        .methods("POST"_method)
    ([](const crow::request& req){
        auto x = crow::json::load(req.body);
        if (!x)
            return crow::response(400);
        int sum = x["a"].i()+x["b"].i();
        std::ostringstream os;
        os << sum;
        return crow::response{os.str()};
    });

    // Example of a request taking URL parameters
    // If you want to activate all the functions just query
    // {ip}:18080/params?foo='blabla'&pew=32&count[]=a&count[]=b
    CROW_ROUTE(app, "/params")
    ([](const crow::request& req){
        std::ostringstream os;

        // To get a simple string from the url params
        // To see it in action /params?foo='blabla'
        os << "Params: " << req.url_params << "\n\n"; 
        os << "The key 'foo' was " << (req.url_params.get("foo") == nullptr ? "not " : "") << "found.\n";

        // To get a double from the request
        // To see in action submit something like '/params?pew=42'
        
        // To get a list from the request
        // You have to submit something like '/params?count[]=a&count[]=b' to have a list with two values (a and b)
        auto count = req.url_params.get_list("count");
        os << "The key 'count' contains " << count.size() << " value(s).\n";
        for(const auto& countVal : count) {
            os << " - " << countVal << '\n';
        }

        // To get a dictionary from the request
        // You have to submit something like '/params?mydict[a]=b&mydict[abcd]=42' to have a list of pairs ((a, b) and (abcd, 42))
        auto mydict = req.url_params.get_dict("mydict");
        os << "The key 'dict' contains " << mydict.size() << " value(s).\n";
        for(const auto& mydictVal : mydict) {
            os << " - " << mydictVal.first << " -> " << mydictVal.second << '\n';
        }

        return crow::response{os.str()};
    });    

    CROW_ROUTE(app, "/large")
    ([]{
        return std::string(512*1024, ' ');
    });
    
    // enables all log
    //ExampleLogHandler logHandler;
    //ExampleLogHandler::logHandlerPtr_ = &logHandler;
    
    
    app.loglevel(crow::LogLevel::DEBUG);
    //crow::logger::setHandler(std::make_shared<ExampleLogHandler>());

    app.port(18080)
        .concurrency(num_threads)
        //.ssl_file("cert.crt", "private.key")
        .run();

    //logHandler.flush();
    //logHandler.close();

    

    
}


