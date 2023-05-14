
#define CROW_ENABLE_SSL
#include "crow.h"




#include <sstream>
#include <iostream>
#include <pqxx/pqxx>
#include <nlohmann/json.hpp>

using namespace std;
using namespace pqxx;


std::string conn_string ="";
using json = nlohmann::json;

void doSomething(int i) {
    std::cout << "Thread " << i << " is doing something..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1)); // simulate some work
    std::cout << "Thread " << i << " has finished doing something." << std::endl;
}

class ExampleLogHandler : public crow::ILogHandler {
    public:
        void log(std::string /*message*/, crow::LogLevel /*level*/) override {
//            cerr << "ExampleLogHandler -> " << message;
        }
};
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

struct ExampleMiddleware 
{
    std::string message;

    ExampleMiddleware() 
    {
        message = "foo";
    }

    void setMessage(std::string newMsg)
    {
        message = newMsg;
    }

    struct context
    {
        // Store the start time of the request processing
        std::chrono::system_clock::time_point start_time;
        // Store the IP address of the client making the request
        std::string client_ip;
    };

    void before_handle(crow::request& req, crow::response& res, context& ctx)
    {
        // Log the start time of the request processing
        ctx.start_time = std::chrono::system_clock::now();
        // Log the IP address of the client making the request
        ctx.client_ip = req.get_header_value("X-Real-IP");

        CROW_LOG_INFO << "[REQUEST] " << req.method << " " << req.raw_url << " FROM " << ctx.client_ip;
    }

    void after_handle(crow::request& req, crow::response& res, context& ctx)
    {
        // Log the duration of the request processing
        auto end_time = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsed_seconds = end_time - ctx.start_time;

        CROW_LOG_INFO << "[RESPONSE] " << res.code << " " << req.method << " " << req.raw_url
                      << " FROM " << ctx.client_ip << " IN " << elapsed_seconds.count() << " SECONDS";
    }
};
int main()
{
    std::string sql;
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
    
    
    const size_t num_threads = 10;
    asio::thread_pool thread_pool(num_threads);
    crow::App<ExampleMiddleware> app(&thread_pool);
    

    app.get_middleware<ExampleMiddleware>().setMessage("hello");

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

            crow::json::wvalue json_data;
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
                json_data[user.user_id] = std::move(user_json);
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
    app.loglevel(crow::LogLevel::DEBUG);
    //crow::logger::setHandler(std::make_shared<ExampleLogHandler>());

    app.port(18080)
        .multithreaded()
        //.ssl_file("cert.crt", "private.key")
        .run();

    // added for timing
    auto start = std::chrono::system_clock::now();

    // added for timestamp
    std::time_t now = std::chrono::system_clock::to_time_t(start);

    std::cout << "Starting threads at " << std::put_time(std::localtime(&now), "%Y-%m-%d %H:%M:%S") << std::endl;

    for (int i = 0; i < num_threads; i++) {
        asio::post(thread_pool, [i]() {
        auto start = std::chrono::high_resolution_clock::now();
        doSomething(i);
        auto end = std::chrono::high_resolution_clock::now();
        std::cout << "Thread " << i << " finished in "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << " milliseconds" << std::endl;
});

    }

    // added for timing
    auto end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = end-start;

    // added for timestamp
    now = std::chrono::system_clock::to_time_t(end);

    std::cout << "All threads finished at " << std::put_time(std::localtime(&now), "%Y-%m-%d %H:%M:%S") << std::endl;
    std::cout << "Elapsed time: " << elapsed_seconds.count() << " seconds" << std::endl;
}


