#ifndef TODO_SERVICE_HPP
#define TODO_SERVICE_HPP

#include <boost/json.hpp>
#include <string>
#include <map>
#include <optional>
#include "DbAccess.hpp"  // PgPool + ToDoItem

class ToDoService 
{
public:
    explicit ToDoService(PgPool& pool) : pool_(pool) {}

    bool CreateToDo(const boost::json::value& body, std::string& out_id, std::string& error);

    bool GetAllToDos(map<string, string> params, boost::json::array& out_items, std::string& error);

    bool GetToDoById(const std::string& id, boost::json::object& out_item, std::string& error);

    bool UpdateToDo(const std::string& id, const boost::json::value& body, std::string& error);

    bool DeleteToDo(const std::string& id, std::string& error);

private:
    PgPool& pool_;
};

#endif