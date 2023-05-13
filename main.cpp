
#define CROW_ENABLE_SSL
#include "crow.h"


#include <sstream>
#include <iostream>
#include <pqxx/pqxx>
#include <nlohmann/json.hpp>
#include <jwt-cpp/jwt.h>
#include "user.h"

using namespace std;
using namespace pqxx;

using json = nlohmann::json;

class ExampleLogHandler : public crow::ILogHandler {
    public:
        void log(std::string /*message*/, crow::LogLevel /*level*/) override {
//            cerr << "ExampleLogHandler -> " << message;
        }
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
    };

    void before_handle(crow::request& /*req*/, crow::response& /*res*/, context& /*ctx*/)
    {
        CROW_LOG_DEBUG << " - MESSAGE: " << message;
    }

    void after_handle(crow::request& /*req*/, crow::response& /*res*/, context& /*ctx*/)
    {
        // no-op
    }
};

int main()
{
    std::string sql;
    try{
        // Set up connection parameters
        std::string host = "ec2-63-34-16-201.eu-west-1.compute.amazonaws.com";
        std::string port = "5432";
        std::string dbname = "dcfr7a3le1omi";
        std::string user = "fcexicqnrtdcrl";
        std::string password = "563e7c0243b32fe9cd7b512ea8032ce87dfcb5263f902e6cd2bb299546c151e2";

        // Create connection string
        std::string conn_string = "host=" + host + " port=" + port + " dbname=" + dbname + " user=" + user + " password=" + password;
        
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
    
    
    
    crow::App<ExampleMiddleware> app;

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
        // Parse request body for credentials
        crow::json::rvalue body = crow::json::load(req.body);
        if (!body) {
            crow::response response;
            response.code = 400;
            response.body = "Invalid request body";
            response.set_header("Content-Type", "text/plain");
            return response;
        }
        std::string email = body["email"].s();
        std::string password = body["password"].s();

        // Set up connection parameters
        std::string host = "ec2-63-34-16-201.eu-west-1.compute.amazonaws.com";
        std::string port = "5432";
        std::string dbname = "dcfr7a3le1omi";
        std::string user = "fcexicqnrtdcrl";
        std::string db_password = "563e7c0243b32fe9cd7b512ea8032ce87dfcb5263f902e6cd2bb299546c151e2";

        // Create connection string
        std::string conn_string = "host=" + host + " port=" + port + " dbname=" + dbname + " user=" + user + " password=" + db_password;
        pqxx::connection conn(conn_string);

        try {
            pqxx::work txn(conn);

            // Find user by email
            pqxx::result result = txn.exec_params(
                "SELECT user_id, user_password FROM users WHERE user_email = $1",
                email
            );
            if (result.empty()) {
                txn.commit();
                crow::response response;
                response.code = 401;
                response.body = "Invalid email or password";
                response.set_header("Content-Type", "text/plain");
                return response;
            }
            std::string hashed_password = result[0]["user_password"].as<std::string>();

            // Verify password
            if (!bcrypt_checkpw(password.c_str(), hashed_password.c_str())) {
                txn.commit();
                crow::response response;
                response.code = 401;
                response.body = "Invalid email or password";
                response.set_header("Content-Type", "text/plain");
                return response;
            }
            int user_id = result[0]["user_id"].as<int>();

            // Generate JWT
            std::string jwt_secret = "my_jwt_secret";
            jwt::jwt_object jwt_payload = jwt::create()
                .set_issuer("my_app")
                .set_subject(std::to_string(user_id))
                .set_issued_at(std::chrono::system_clock::now())
                .set_expires_at(std::chrono::system_clock::now() + std::chrono::hours(24))
                .sign(jwt::algorithm::hs256(jwt_secret));

            txn.commit();

            crow::response response;
            response.code = 200;
            response.body = "Login successful";
            response.set_header("Content-Type", "text/plain");
            response.set_header("Authorization", "Bearer " + jwt_payload.signature());
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
    ([]{
        // Set up connection parameters
        std::string host = "ec2-63-34-16-201.eu-west-1.compute.amazonaws.com";
        std::string port = "5432";
        std::string dbname = "dcfr7a3le1omi";
        std::string user = "fcexicqnrtdcrl";
        std::string password = "563e7c0243b32fe9cd7b512ea8032ce87dfcb5263f902e6cd2bb299546c151e2";

        // Create connection string
        std::string conn_string = "host=" + host + " port=" + port + " dbname=" + dbname + " user=" + user + " password=" + password;
        pqxx::connection conn(conn_string);
        
        try {
            pqxx::work txn(conn);
            pqxx::result result = txn.exec("SELECT user_id, user_email, user_password FROM users");
            txn.commit();

            crow::json::wvalue json_data;
            for (const auto& row : result) {
                crow::json::wvalue user;
                user["user_id"] = row["user_id"].as<int>();
                user["user_email"] = row["user_email"].as<std::string>();
                user["user_password"] = row["user_password"].as<std::string>();
                json_data[row["user_id"].as<int>()] = std::move(user);
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
}