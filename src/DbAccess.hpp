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
    int priority;    // 1 (highest) to 5 (lowest)
    vector<string> tags;
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
            auto result = txn.exec_params(
                "INSERT INTO ToDoItems (id, name, description, due_date, status, priority, tags) "
                "VALUES ($1, $2, $3, $4, $5::todo_item_status, $6, $7::text[])",
                item.id, item.name, item.description.empty() ? nullopt : optional<string>{item.description},
                item.due_date.empty() ? nullopt : optional<string>{item.due_date},
                item.status, item.priority, item.tags
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

    bool GetAllToDoItems(
        boost::json::array& out_items,
        std::optional<std::string> status_filter      = std::nullopt,
        std::optional<std::string> due_date_after     = std::nullopt,
        std::optional<std::string> due_date_before    = std::nullopt,
        std::optional<int> min_priority               = std::nullopt,
        std::optional<int> max_priority               = std::nullopt,
        std::optional<std::string> tag_contains       = std::nullopt,
        std::optional<std::string> sort_by            = "due_date",
        std::optional<std::string> sort_order         = "asc"
    )   
    {
        out_items.clear();

        try {
            auto conn_ptr = this->get();
            if (!conn_ptr) 
            {
                std::cerr << "No available connection in pool" << std::endl;
                return false;
            }

            pqxx::work txn(*conn_ptr);

            std::string where_clause;
            std::vector<std::string> params;

            auto add_condition = [&](const std::string& cond, const std::string& val) {
                where_clause += (where_clause.empty() ? "WHERE " : " AND ");
                where_clause += cond + " $" + std::to_string(params.size() + 1);
                params.push_back(val);
            };

            if (status_filter.has_value()) 
            {
                add_condition("status = ", *status_filter);
            }

            if (due_date_after.has_value()) 
            {
                add_condition("due_date > ", *due_date_after);
            }

            if (due_date_before.has_value()) 
            {
                add_condition("due_date < ", *due_date_before);
            }

            if (min_priority.has_value()) 
            {
                add_condition("priority >= ", std::to_string(*min_priority));
            }

            if (max_priority.has_value()) 
            {
                add_condition("priority <= ", std::to_string(*max_priority));
            }

            if (tag_contains.has_value()) 
            {
                // PostgreSQL: check if array contains value
                where_clause += (where_clause.empty() ? "WHERE " : " AND ");
                where_clause += "$" + std::to_string(params.size() + 1) + " = ANY(tags)";
                params.push_back(*tag_contains);
            }

            std::string order_clause = " ORDER BY ";

            std::string field = sort_by.value_or("due_date");
            std::string direction = (sort_order.value_or("asc") == "desc") ? "DESC" : "ASC";

            if (field == "due_date") 
            {
                order_clause += "due_date " + direction + " NULLS LAST";
            } 
            else if (field == "name") 
            {
                order_clause += "name " + direction;
            } 
            else if (field == "status") 
            {
                order_clause += "status " + direction;
            } 
            else if (field == "id") 
            {
                order_clause += "id " + direction;
            } 
            else if (field == "priority") 
            {
                order_clause += "priority " + direction + " NULLS LAST";
            } 
            else 
            {
                order_clause += "due_date ASC NULLS LAST";  // fallback
            }

            // Final SQL query – include new columns
            std::string sql = 
                "SELECT id, name, description, due_date, status, priority, tags "
                "FROM ToDoItems "
                + where_clause
                + order_clause;

            pqxx::result rows;
            if (params.empty()) 
            {
                rows = txn.exec(sql);
            } 
            else if (params.size() == 1) 
            {
                rows = txn.exec_params(sql, params[0]);
            } 
            else if (params.size() == 2) 
            {
                rows = txn.exec_params(sql, params[0], params[1]);
            } 
            else if (params.size() == 3) 
            {
                rows = txn.exec_params(sql, params[0], params[1], params[2]);
            } 
            else if (params.size() == 4) 
            {
                rows = txn.exec_params(sql, params[0], params[1], params[2], params[3]);
            } 
            else if (params.size() == 5) 
            {
                rows = txn.exec_params(sql, params[0], params[1], params[2], params[3], params[4]);
            } 
            else if (params.size() == 6) 
            {
                rows = txn.exec_params(sql, params[0], params[1], params[2], params[3], params[4], params[5]);
            } 
            else if (params.size() == 7) 
            {
                rows = txn.exec_params(sql, params[0], params[1], params[2], params[3], params[4], params[5], params[6]);
            } 
            else if (params.size() == 8) 
            {
                rows = txn.exec_params(sql, params[0], params[1], params[2], params[3], params[4], params[5], params[6], params[7]);
            } 
            else if (params.size() == 9) 
            {
                rows = txn.exec_params(sql, params[0], params[1], params[2], params[3], params[4], params[5], params[6], params[7], params[8]);
            } 
            else 
            {
                throw std::runtime_error("Too many parameters for exec_params (max 9 supported in this impl)");
            }

            for (auto row : rows) 
            {
                boost::json::object item;

                item["id"]          = row["id"].as<std::string>();
                item["name"]        = row["name"].as<std::string>();
                item["description"] = row["description"].is_null() ? "" : row["description"].as<std::string>();
                item["due_date"]    = row["due_date"].is_null() ? "" : row["due_date"].as<std::string>();
                item["status"]      = row["status"].as<std::string>();
                item["priority"]    = row["priority"].as<int>();

                string tagsStr = row["tags"].is_null() ? "" : row["tags"].as<std::string>();
                if (!tagsStr.empty() && tagsStr.front() == '{' && tagsStr.back() == '}') 
                {
                    tagsStr = tagsStr.substr(1, tagsStr.size() - 2);
                }

                out_items.emplace_back(std::move(item));
                std::cout << "Fetched item: " << row["name"].as<std::string>() << std::endl;
            }

            release(conn_ptr);
            return true;
        }
        catch (const std::exception& e) 
        {
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

            auto row = txn.exec_params1("SELECT name, description, due_date, status, priority, tags "
                                        "FROM ToDoItems WHERE id = $1", id);

            string tagsStr = row["tags"].is_null() ? "" : row["tags"].as<std::string>();
            if (!tagsStr.empty() && tagsStr.front() == '{' && tagsStr.back() == '}') 
            {
                tagsStr = tagsStr.substr(1, tagsStr.size() - 2);
            }

            json::object foundItem
            {
                {"id",          id},
                {"name",        row["name"].as<string>()},
                {"description", row["description"].is_null() ? "" : row["description"].as<string>()},
                {"due_date",    row["due_date"].is_null() ? "" : row["due_date"].as<string>()},
                {"status",      row["status"].as<string>()},
                {"priority",    row["priority"].as<int>()},
                {"tags",        tagsStr}
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
            } 
            else if (params.size() == 2) 
            {
                txn.exec_params(query, params[0], params[1]);
            } 
            else if (params.size() == 3) 
            {
                txn.exec_params(query, params[0], params[1], params[2]);
            } 
            else if (params.size() == 4) 
            {
                txn.exec_params(query, params[0], params[1], params[2], params[3]);
            } 
            else if (params.size() == 5) 
            {
                txn.exec_params(query, params[0], params[1], params[2], params[3], params[4]);
            }
            else if (params.size() == 6) 
            {
                txn.exec_params(query, params[0], params[1], params[2], params[3], params[4], params[5]);
            }
            else if (params.size() == 7) 
            {
                txn.exec_params(query, params[0], params[1], params[2], params[3], params[4], params[5], params[6]);
            }
            else 
            {
                throw runtime_error("Too many parameters for update");
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