#ifndef DBACCESS_HPP
#define DBACCESS_HPP

#include <boost/json.hpp>

#include <vector>
#include <string>
#include <iostream>
#include <pqxx/pqxx>
#include <string>
#include <memory>
#include <mutex>

#include "Utility.hpp"

namespace json = boost::json;
using namespace std;

class ToDoItem
{
public:
    string id;
    string name;
    string description;
    string due_date;
    string status;   // "Completed", "In Progress", "Not Started"
};



class PgPool {
public:
    PgPool(const string& conn_str, size_t size = 5) : conn_str_(conn_str), size_(size) 
    {
        for (size_t i = 0; i < size_; ++i) {
            conns_.emplace_back(make_shared<pqxx::connection>(conn_str_));
        }
    }

    shared_ptr<pqxx::connection> get() 
    {
        lock_guard<mutex> lock(mtx_);
        if (conns_.empty()) return nullptr;
        auto conn = conns_.back();
        conns_.pop_back();
        return conn;
    }

    void release(shared_ptr<pqxx::connection> conn) 
    {
        lock_guard<mutex> lock(mtx_);
        conns_.push_back(conn);
    }

    virtual bool CreateToDoItem(ToDoItem item)
    {
        try
        {   
            auto conn_ptr = this->get();
            if (!conn_ptr) 
            {
                throw runtime_error("No available database connection");
            }
            pqxx::work txn(*conn_ptr);            
            string new_id = generate_id();
            auto result = txn.exec_params(
                "INSERT INTO ToDoItems (id, name, description, due_date, status) "
                "VALUES ($1, $2, $3, $4, $5::todo_item_status)",
                new_id, item.name, item.description.empty() ? nullopt : optional<string>{item.description},
                item.due_date.empty() ? nullopt : optional<string>{item.due_date},
                item.status
            );
            txn.commit();
            this->release(conn_ptr);
        }
        catch (const pqxx::sql_error& se) 
        {
            cerr << "Database error: " << se.what() << "\n";
            return false;
        }
        catch (const exception& e) 
        {
            cerr << "Error: " << e.what() << "\n";
            return false;
        }
        return true;
    }
    virtual bool GetAllToDoItems(
        boost::json::array& out_items,
        std::optional<std::string> status_filter      = std::nullopt,
        std::optional<std::string> due_date_after     = std::nullopt,
        std::optional<std::string> due_date_before    = std::nullopt,
        std::optional<std::string> sort_by            = "due_date",
        std::optional<std::string> sort_order         = "asc")   
    {
        out_items.clear();

        try {
            // Get connection (using your existing pattern)
            auto conn_ptr = this->get();
            if (!conn_ptr) 
            {
                std::cerr << "No available connection in pool" << std::endl;
                return false;
            }

            pqxx::work txn(*conn_ptr);

            // Build WHERE clause
            std::string where_clause;
            std::vector<std::string> params;

            if (status_filter.has_value()) {
                where_clause += (where_clause.empty() ? "WHERE " : " AND ");
                where_clause += "status = $" + std::to_string(params.size() + 1);
                params.push_back(*status_filter);
            }

            if (due_date_after.has_value()) {
                where_clause += (where_clause.empty() ? "WHERE " : " AND ");
                where_clause += "due_date > $" + std::to_string(params.size() + 1);
                params.push_back(*due_date_after);
            }

            if (due_date_before.has_value()) {
                where_clause += (where_clause.empty() ? "WHERE " : " AND ");
                where_clause += "due_date < $" + std::to_string(params.size() + 1);
                params.push_back(*due_date_before);
            }

            // Build ORDER BY clause
            std::string order_clause = " ORDER BY ";

            std::string field = sort_by.value_or("due_date");
            std::string direction = (sort_order.value_or("asc") == "desc") ? "DESC" : "ASC";

            if (field == "due_date") {
                order_clause += "due_date " + direction + " NULLS LAST";
            }
            else if (field == "name") {
                order_clause += "name " + direction;
            }
            else if (field == "status") {
                order_clause += "status " + direction;
            }
            else if (field == "id") {
                order_clause += "id " + direction;
            }
            else {
                // Fallback to default
                order_clause += "due_date ASC NULLS LAST";
            }

            // Final query
            std::string sql = "SELECT id, name, description, due_date, status FROM ToDoItems "
                + where_clause
                + order_clause;

            // Execute with parameters
            pqxx::result rows;
            if (params.empty()) {
                rows = txn.exec(sql);
            } else if (params.size() == 1) {
                rows = txn.exec_params(sql, params[0]);
            } else if (params.size() == 2) {
                rows = txn.exec_params(sql, params[0], params[1]);
            } else if (params.size() == 3) {
                rows = txn.exec_params(sql, params[0], params[1], params[2]);
            } else if (params.size() == 4) {
                rows = txn.exec_params(sql, params[0], params[1], params[2], params[3]);
            } else {
                throw std::runtime_error("Too many parameters");
            }

            // Build JSON array
            for (auto row : rows) {
                boost::json::object item;

                item["id"]          = row["id"].as<std::string>();
                item["name"]        = row["name"].as<std::string>();
                item["description"] = row["description"].is_null() ? "" : row["description"].as<std::string>();
                item["due_date"]    = row["due_date"].is_null() ? "" : row["due_date"].as<std::string>();
                item["status"]      = row["status"].as<std::string>();

                out_items.emplace_back(std::move(item));

                // print out for debugging
                std::cout << "Fetched item: " << row["status"].as<std::string>() << std::endl;
            }

            // Return connection
            release(conn_ptr);

            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "GetAllToDoItems failed: " << e.what() << std::endl;
            return false;
        }
    }

    virtual bool GetToDoItemById(const string& id, json::object& item)
    {
        try
        {   
            auto conn_ptr = this->get();
            if (!conn_ptr) 
            {
                throw runtime_error("No available database connection");
            }
            pqxx::work txn(*conn_ptr);            

            auto row = txn.exec_params1("SELECT name, description, due_date, status "
                                        "FROM ToDoItems WHERE id = $1", id);
            
            json::object foundItem
            {
                {"id",          id},
                {"name",        row["name"].as<string>()},
                {"description", row["description"].is_null() ? "" : row["description"].as<string>()},
                {"due_date",    row["due_date"].is_null() ? "" : row["due_date"].as<string>()},
                {"status",      row["status"].as<string>()}
            };
            item = move(foundItem);
            
            txn.commit();
            this->release(conn_ptr);
        }
        catch (const pqxx::sql_error& se) 
        {
            cerr << "Database error: " << se.what() << "\n";
            return false;
        }
        catch (const exception& e) 
        {
            cerr << "Error: " << e.what() << "\n";
            return false;
        }
        return true;
    }

    virtual bool UpdateToDoItem(const string& id, const map<string, string>& updates)
    {
        try
        {   
            auto conn_ptr = this->get();
            if (!conn_ptr) 
            {
                throw runtime_error("No available database connection");
            }
            pqxx::work txn(*conn_ptr);            

            string set_clause;
            vector<string> params;
            int idx = 1;
            for (const auto& [k, v] : updates) {
                if (!set_clause.empty()) set_clause += ", ";
                set_clause += k + " = $" + to_string(idx++);
                params.push_back(v);
            }
            params.push_back(id);  // last param = id

            string query = "UPDATE ToDoItems SET " + set_clause + " WHERE id = $" + to_string(idx);            
            if (params.size() == 1) 
            {
                txn.exec_params(query, params[0]);
            } else if (params.size() == 2) 
            {
                txn.exec_params(query, params[0], params[1]);
            } else if (params.size() == 3) 
            {
                txn.exec_params(query, params[0], params[1], params[2]);
            } else if (params.size() == 4) 
            {
                txn.exec_params(query, params[0], params[1], params[2], params[3]);
            } else if (params.size() == 5) 
            {
                txn.exec_params(query, params[0], params[1], params[2], params[3], params[4]);
            }
            txn.commit();
            this->release(conn_ptr);
        }
        catch (const pqxx::sql_error& se) 
        {
            cerr << "Database error: " << se.what() << "\n";
            return false;
        }
        catch (const exception& e) 
        {
            cerr << "Error: " << e.what() << "\n";
            return false;
        }
        return true;
    }

    virtual bool DeleteToDoItem(const string& id)
    {
        try
        {   
            auto conn_ptr = this->get();
            if (!conn_ptr) 
            {
                throw runtime_error("No available database connection");
            }
            pqxx::work txn(*conn_ptr);            

            auto result = txn.exec_params("DELETE FROM ToDoItems WHERE id = $1", id);
            this->release(conn_ptr);
            if (result.affected_rows() == 0) 
            {
                throw runtime_error("No ToDo item found with given ID");
            }
            else
            {                
                txn.commit();
            }
        }
        catch (const pqxx::sql_error& se) 
        {
            cerr << "Database error: " << se.what() << "\n";
            return false;
        }
        catch (const exception& e) 
        {
            cerr << "Error: " << e.what() << "\n";
            return false;
        }
        return true;
    }


private:
    string conn_str_;
    size_t size_;
    vector<shared_ptr<pqxx::connection>> conns_;
    mutex mtx_;
};

#endif