# ToDoService POC - Real-time Collaborative TODO List API

A proof-of-concept REST API server for a collaborative TODO list application, built with:

- **C++** (C++20)
- **Boost.Beast** (HTTP/WebSocket server)
- **Boost.JSON** (JSON handling)
- **libpqxx** (PostgreSQL client)
- **PostgreSQL** (database)

This is a **minimal POC** focused on basic CRUD operations for TODO items, with filtering & sorting support.  
The design is inspired by a real-time collaborative TODO system (CRDTs, Redis Pub/Sub, Kafka event sourcing), but only the REST layer is implemented so far.

## Current Features

- REST endpoints for TODO items (flat table – no lists/users yet)
  - `POST /todos` – create item
  - `GET /todos` – list items (with filters: status, due_date range, priority, tags)
  - `GET /todos/{id}` – get single item
  - `PATCH /todos/{id}` – update fields
  - `DELETE /todos/{id}` – delete item
- Query parameters supported on `GET /todos`:
  - `?status=In%20Progress`
  - `?due_date_after=2026-02-01T00:00:00Z`
  - `?due_date_before=2026-03-01T00:00:00Z`
  - `?min_priority=3` / `?max_priority=5`
  - `?tag=work` (items that contain this tag)
  - `?sort=name|due_date|status|id|priority`
  - `?order=asc|desc`
- PostgreSQL storage (with enum for status)
- UUID v4 generation for item IDs
- Basic unit tests (GoogleTest) for UUID generator

## Planned / Future Features (not yet implemented)

- List scoping (`list_id` + endpoints under `/lists/{listId}/todos`)
- User authentication & permissions
- Kafka consumer + event sourcing processing
- Sharing links

## Project Structure
    ToDoService/
    ├── CMakeLists.txt          # Build system configuration
    ├── README.md
    ├── src/
    │   └── Server.cpp              # Main HTTP server
    │   └── Utility.hpp             # Helper functions
    │   └── DbAccess.hpp            # PgPool connection pool + low-level CRUD methods
    │   └── ToDoService.cpp         # Implementation of ToDoService class
    │   └── ToDoService.hpp         # Service layer: business logic, CRUD wrappers
    └── tests/
        └── todo_service_test.cpp   # GoogleTest unit tests

## Prerequisites

- **CMake** 3.15+
- **C++20** compiler (MSVC 2022 recommended)
- **vcpkg** (dependency manager) at `C:/vcpkg`
- **PostgreSQL** 13+ (local instance)
- **GoogleTest** (via vcpkg)

## Installed vcpkg packages:
    vcpkg install boost-asio:x64-windows
    vcpkg install boost-beast:x64-windows
    vcpkg install boost-system:x64-windows
    vcpkg install boost-thread:x64-windows
    vcpkg install boost-json:x64-windows
    vcpkg install libpqxx:x64-windows
    vcpkg install gtest:x64-windows

## Build Instructions
1. Configure CMake (from project root)

    ```cmd
    mkdir build
    cd build
    cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows
    ```

2. Build (Debug or Release)

    ```cmd
    cmake --build . --config Debug
    # or
    cmake --build . --config Release
    ```

3. Run the server

    ```cmd
    .\Debug\ToDoService.exe
    # or
    .\Release\ToDoService.exe
    ```

Server listens on: http://localhost:8080


## Run Unit Tests
    cd build
    ctest -C Debug -V (or .\Debug\todo_tests.exe..)
    # or
    ctest -C Release -V (or .\Release\todo_tests.exe..)



