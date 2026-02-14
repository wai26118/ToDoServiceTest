#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/json.hpp>
#include <boost/algorithm/string.hpp>

#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <vector>
#include <memory>
#include <random>
#include <sstream>
#include <iomanip>

#include "Utility.hpp"
#include "DbAccess.hpp"
#include "ToDoService.hpp"

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace json = boost::json;
using tcp = net::ip::tcp;
using namespace std;

PgPool pg_pool("host=localhost dbname=todolist user=postgres password=12345", 5);

// This function produces an HTTP response for the given
//
void handle_request(http::request<http::string_body>&& req, beast::tcp_stream& stream) 
{
    http::response<http::string_body> res{http::status::ok, req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "application/json");
    res.keep_alive(req.keep_alive());

    ToDoService service(pg_pool);

    try 
    {
        string target = string(req.target());
        auto method = req.method();

        json::value body_val;
        if (!req.body().empty()) {
            body_val = json::parse(req.body());
        }

        string error_msg = "";

        if (method == http::verb::post && target == "/todos") 
        {
            string new_id;
            if (service.CreateToDo(body_val, new_id, error_msg)) 
            {
                json::object resp{{"id", new_id}};
                res.body() = json::serialize(resp);
            } 
            else 
            {
                res.result(http::status::bad_request);
                json::object err{{"error", error_msg}};
                res.body() = json::serialize(err);
            }
        }
        else if (method == http::verb::get && target.rfind("/todos/", 0) == 0) 
        {
            string id = target.substr(7);
            json::object item;
            if (service.GetToDoById(id, item, error_msg))
            {
                res.body() = json::serialize(item);
            }
            else
            {
                res.result(http::status::bad_request);
                json::object err{{"error", error_msg}};
                res.body() = json::serialize(err);
            }
        }
        else if (method == http::verb::get && target.find("/todos") == 0) 
        {
            // Parse query string if present
            string query_string;
            size_t qpos = target.find('?');
            if (qpos != string::npos) {
                query_string = target.substr(qpos + 1);
            }

            // Simple query param map
            map<string, string> params;
            if (!query_string.empty()) {
                istringstream iss(query_string);
                string token;
                while (getline(iss, token, '&')) {
                    size_t eq = token.find('=');
                    if (eq != string::npos) {
                        string key = token.substr(0, eq);
                        string val = token.substr(eq + 1);
                        boost::algorithm::replace_all(val, "%20", " ");
                        params[key] = val;
                    }
                }
            }

            json::array out_items;
            if (service.GetAllToDos(params, out_items, error_msg))
            {
                json::object resp{{"todos", move(out_items)}};
                res.body() = json::serialize(resp);
            }
            else
            {
                res.result(http::status::bad_request);
                json::object err{{"error", error_msg}};
                res.body() = json::serialize(err);
            }
        }
        else if (method == http::verb::patch && target.rfind("/todos/", 0) == 0) 
        {
            string id = target.substr(7);

            if (!body_val.is_object()) 
            {
                throw runtime_error("Request body must be a JSON object");
            }
            if (service.UpdateToDo(id, body_val.as_object(), error_msg)) 
            {
                json::object resp{{"success", true}};
                res.body() = json::serialize(resp);
            } 
            else 
            {
                res.result(http::status::bad_request);
                json::object err{{"error", error_msg}};
                res.body() = json::serialize(err);
            }
        }
        else if (method == http::verb::delete_ && target.rfind("/todos/", 0) == 0) 
        {
            string id = target.substr(7);
            if (service.DeleteToDo(id, error_msg)) 
            {
                json::object resp{{"success", true}};
                res.body() = json::serialize(resp);
            }
            else
            {
                res.result(http::status::bad_request);
                json::object err{{"error", error_msg}};
                res.body() = json::serialize(err);
            }
        }
        else 
        {
            res.result(http::status::not_found);
            json::object err{{"error", "Not Found"}};
            res.body() = json::serialize(err);
        }
    }
    catch (const json::system_error& je) 
    {
        res.result(http::status::bad_request);
        json::object err{{"error", string("Invalid JSON: ") + je.what()}};
        res.body() = json::serialize(err);
    }
    catch (const pqxx::sql_error& se)
    {
        res.result(http::status::internal_server_error);
        json::object err{{"error", string("Database error: ") + se.what()}};
        res.body() = json::serialize(err);
    }
    catch (const exception& e) 
    {
        res.result(http::status::bad_request);
        json::object err{{"error", e.what()}};
        res.body() = json::serialize(err);
    }

    res.prepare_payload();

    beast::error_code ec;
    http::write(stream, res, ec);
    if (ec) 
    {
        cerr << "Write failed: " << ec.message() << "\n";
    }
    cout << "Responded with status " << res.result_int() << "\n";
}


// This function handles an HTTP server connection
//
void do_session(beast::tcp_stream stream) 
{
    beast::error_code ec;
    beast::flat_buffer buffer;
    http::request<http::string_body> req;
    http::read(stream, buffer, req, ec);
    if (ec) 
    {
        cerr << "Read error: " << ec.message() << "\n";
        return;
    }
    cout << "Received request: " << req.method_string() << " " << req.target() << "\n";
    handle_request(move(req), stream);
    stream.close();
}

// Accepts incoming connections and launches the sessions
//
void do_accept(net::io_context& ioc, tcp::acceptor& acceptor) 
{
    for (;;) 
    {
        tcp::socket socket(ioc);
        beast::error_code ec;
        acceptor.accept(socket, ec);
        if (ec) 
        {
            cerr << "Accept error: " << ec.message() << "\n";
            continue;
        }
        cout << "Accepted connection from " << socket.remote_endpoint() << "\n";
        thread([s = beast::tcp_stream(move(socket))]() mutable 
        {
            do_session(move(s));
        }).detach();
    }
}

// Main function: setup server and run
//
int main() 
{
    try 
    {
        cout << "Starting ToDoService...\n";
        net::io_context ioc{1};
        tcp::acceptor acceptor(ioc, tcp::endpoint(tcp::v4(), 8080));

        cout << "ToDoService listening on http://localhost:8080\n";

        thread t([&] { do_accept(ioc, acceptor); });
        ioc.run();
        t.join();
    }
    catch (const exception& e) 
    {
        cerr << "Fatal: " << e.what() << "\n";
        return 1;
    }
    return 0;
}