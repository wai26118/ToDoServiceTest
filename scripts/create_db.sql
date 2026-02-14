CREATE TYPE todo_item_status AS ENUM (
    'Completed',    
    'In Progress',
    'Not Started'    
);

CREATE TABLE ToDoItems (
    id          UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    name        TEXT NOT NULL,
    description TEXT,
    due_date    TIMESTAMPTZ,
    status      todo_item_status NOT NULL DEFAULT 'Not Started'
);