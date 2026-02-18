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
        std::string priority_str = (body.is_object() && body.as_object().count("priority") > 0) ? body.at("priority").as_string().c_str() : "3";
        std::string tags_str = (body.is_object() && body.as_object().count("tags") > 0) ? body.at("tags").as_string().c_str() : "";

        if (status_str != "Not Started" && status_str != "In Progress" && status_str != "Completed") 
        {
            error = "Invalid status value";
            return false;
        }

        std::string new_id = generate_id();
        ToDoItem item{new_id, name, description, due_date_str, status_str, stoi(priority_str), {}};

        if (!tags_str.empty()) {
            // Parse tags string into vector of strings
            std::vector<std::string> tags;
            size_t start = 0;
            size_t end = 0;
            while ((end = tags_str.find(',', start)) != std::string::npos) {
                tags.push_back(tags_str.substr(start, end - start));
                start = end + 1;
            }
            tags.push_back(tags_str.substr(start)); // Add the last tag

            item.tags = tags;
        }

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

bool ToDoService::GetAllToDos(
    std::map<std::string, std::string> params,
    boost::json::array& out_items,
    std::string& error
) 
{
    try 
    {
        std::optional<std::string> status_filter;
        std::optional<std::string> due_after;
        std::optional<std::string> due_before;

        std::optional<int> min_priority;
        std::optional<int> max_priority;
        std::optional<std::string> tag_contains;

        if (params.count("status")) {
            std::string s = params["status"];
            if (s != "Not Started" && s != "In Progress" && s != "Completed") 
            {
                error = "Invalid status filter value";
                return false;
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

        if (params.count("min_priority")) 
        {
            try 
            {
                min_priority = std::stoi(params["min_priority"]);
                if (*min_priority < 1 || *min_priority > 5) 
                {
                    error = "min_priority must be between 1 and 5";
                    return false;
                }
            } 
            catch (...) 
            {
                error = "Invalid min_priority value";
                return false;
            }
        }

        if (params.count("max_priority")) 
        {
            try 
            {
                max_priority = std::stoi(params["max_priority"]);
                if (*max_priority < 1 || *max_priority > 5) 
                {
                    error = "max_priority must be between 1 and 5";
                    return false;
                }
            } 
            catch (...) 
            {
                error = "Invalid max_priority value";
                return false;
            }
        }

        if (params.count("tag")) 
        {
            tag_contains = params["tag"];
        }

        std::optional<std::string> sort_by;
        std::optional<std::string> sort_order;

        if (params.count("sort")) 
        {
            std::string field = params["sort"];
            if (field == "name" || field == "due_date" || field == "status" ||
                field == "id" || field == "priority") 
            {
                sort_by = field;
            } 
            else 
            {
                error = "Invalid sort field. Allowed: name, due_date, status, id, priority";
                return false;
            }
        }

        if (params.count("order")) 
        {
            std::string ord = params["order"];
            if (ord == "asc" || ord == "desc") 
            {
                sort_order = ord;
            } 
            else 
            {
                error = "Invalid sort order. Use 'asc' or 'desc'";
                return false;
            }
        }

        if (!sort_by.has_value()) 
        {
            sort_by = "due_date";
        }
        if (!sort_order.has_value()) 
        {
            sort_order = "asc";
        }

        bool dbResult = pool_.GetAllToDoItems(
            out_items,
            status_filter,
            due_after,
            due_before,
            min_priority,
            max_priority,
            tag_contains,
            sort_by,
            sort_order
        );

        if (!dbResult) 
        {
            error = "Failed to retrieve ToDo items from database";
            return false;
        }

        return true;
    } 
    catch (const std::exception& e) {
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
        if (body.is_object() && body.as_object().count("priority") > 0) 
        {
            // Check if priority is between 1 and 5
            //
            string p_str = body.at("priority").as_string().c_str();
            int p = stoi(p_str);
            if (p < 1 || p > 5) 
            {
                throw runtime_error("Priority must be between 1 and 5");
            }
            updates["priority"] = body.at("priority").as_string().c_str();
        }
        if (body.is_object() && body.as_object().count("tags") > 0) 
        {
            if (!body.at("tags").is_string()) 
            {
                throw runtime_error("Tags must be a comma-separated string");
            }
            updates["tags"] = "{" + std::string(body.at("tags").as_string().c_str()) + "}";
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