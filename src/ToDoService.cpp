#include "ToDoService.hpp"
#include "Utility.hpp"

bool ToDoService::CreateToDo(const boost::json::value& body, std::string& out_id, std::string& error) 
{
    try 
    {
        std::string name = body.at("name").as_string().c_str();
        std::string description = (body.is_object() && body.as_object().count("description") > 0) ? body.at("description").as_string().c_str() : "";
        std::string due_date_str = (body.is_object() && body.as_object().count("due_date") > 0) ? body.at("due_date").as_string().c_str() : "";
        std::string status_str = (body.is_object() && body.as_object().count("status") > 0) ? body.at("status").as_string().c_str() : "Not Started";

        if (status_str != "Not Started" && status_str != "In Progress" && status_str != "Completed") 
        {
            error = "Invalid status value";
            return false;
        }

        std::string new_id = generate_id();
        ToDoItem item{new_id, name, description, due_date_str, status_str};

        if (!pool_.CreateToDoItem(item)) 
        {
            error = "Failed to create ToDo item in database";
            return false;
        }

        out_id = new_id;
        return true;
    } 
    catch (const std::exception& e) 
    {
        error = e.what();
        return false;
    }
}

bool ToDoService::GetAllToDos(map<string, string> params, boost::json::array& out_items, std::string& error) 
{
    try {
        optional<string> status_filter;
        optional<string> due_after;
        optional<string> due_before;

        if (params.count("status")) 
        {
            string s = params["status"];
            if (s != "Not Started" && s != "In Progress" && s != "Completed") 
            {
                throw runtime_error("Invalid status filter value");
            }
            status_filter = s;
        }

        if (params.count("due_date_after")) 
        {
            due_after = params["due_date_after"];
        }
        if (params.count("due_date_before")) 
        {
            due_before = params["due_date_before"];
        }

        optional<string> sort_by;
        optional<string> sort_order;

        if (params.count("sort")) 
        {
            string field = params["sort"];
            if (field == "name" || field == "due_date" || field == "status" || field == "id") 
            {
                sort_by = field;
            } 
            else 
            {
                throw runtime_error("Invalid sort field. Allowed: name, due_date, status, id");
            }
        }

        if (params.count("order")) 
        {
            string ord = params["order"];
            if (ord == "asc" || ord == "desc") 
            {
                sort_order = ord;
            } 
            else 
            {
                throw runtime_error("Invalid sort order. Use 'asc' or 'desc'");
            }
        }

        // Default sorting if not provided
        if (!sort_by.has_value()) 
        {
            sort_by = "due_date";
        }
        if (!sort_order.has_value()) 
        {
            sort_order = "asc";
        }

        bool dbResult = pool_.GetAllToDoItems(out_items, status_filter, due_after, due_before, sort_by, sort_order);
        if (!dbResult) 
        {
            error = "Failed to retrieve ToDo items from database";
            return false;
        }

        return true;
    } 
    catch (const std::exception& e) 
    {
        error = e.what();
        return false;
    }
}

bool ToDoService::GetToDoById(const std::string& id, boost::json::object& out_item, std::string& error) 
{
    try 
    {
        bool dbResult = pool_.GetToDoItemById(id, out_item);
        if (!dbResult) 
        {
            error = "Failed to retrieve ToDo item from database";
            return false;
        }
        return true;
    } 
    catch (const std::exception& e) 
    {
        error = e.what();
        return false;
    }
}

bool ToDoService::UpdateToDo(const std::string& id, const boost::json::value& body, std::string& error) 
{
    try 
    {
        map<string, string> updates;

        if (body.is_object() && body.as_object().count("name") > 0)        
        {
            updates["name"] = body.at("name").as_string().c_str();
        }
        if (body.is_object() && body.as_object().count("description") > 0) 
        {
            updates["description"] = body.at("description").as_string().c_str();
        }
        if (body.is_object() && body.as_object().count("due_date") > 0)
        {
            updates["due_date"] = body.at("due_date").as_string().c_str();
        }
        if (body.is_object() && body.as_object().count("status") > 0) 
        {
            string s = body.at("status").as_string().c_str();
            if (s != "Not Started" && s != "In Progress" && s != "Completed")
            {
                throw runtime_error("Invalid status value");
            }
            updates["status"] = s;
        }

        if (updates.empty()) 
        {
            error = "No fields to update";
            return false;
        }

        bool dbResult = pool_.UpdateToDoItem(id, updates);
        if (!dbResult)
        {
            error = "Failed to update ToDo item in database";
            return false;
        }

        return true;
    } 
    catch (const std::exception& e) 
    {
        error = e.what();
        return false;
    }
}

bool ToDoService::DeleteToDo(const std::string& id, std::string& error) 
{
    try 
    {
        bool dbResult = pool_.DeleteToDoItem(id);
        if (!dbResult)
        {
            error = "Failed to delete ToDo item from database";
            return false;
        }
        return true;
    } 
    catch (const std::exception& e) 
    {
        error = e.what();
        return false;
    }
}