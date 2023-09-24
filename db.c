#include "db.h"


PGconn* conn = NULL;

// Print options
PQprintOpt defaultPrintOptions = {
  .header = 1,      // Include column headers
  .align = 1,       // Align column data
  .standard = 0,    // Use standard (ASCII) characters
  .html3 = 0,       // Do not use HTML formatting
  .expanded = 0,    // Do not expand tables
  .pager = 0,       // Do not use a pager
  .fieldSep = "|",  // Use "|" as the field separator
  .tableOpt = "",   // No table options
};

void connect_to_database(const char* database_uri) {
  if (database_uri == NULL || strlen(database_uri) == 0) {
    // try reading it from environment variable: ECLINIC_DATABASE_URI
    database_uri = getenv("ECLINIC_DATABASE_URI");
    if (database_uri == NULL) {
      fprintf(stderr, "database_uri is not provided\n");
      exit(EXIT_FAILURE);
    }
  }


  if (database_uri == NULL || strlen(database_uri) == 0) {
    fprintf(stderr, "database_uri is not provided\n");
    printf(
      "Please provide database_uri as an argument or set the "
      "ECLINIC_DATABASE_URI environment variable\n");
    exit(EXIT_FAILURE);
  }

  // Initialize the PostgreSQL connection object
  conn = PQconnectdb(database_uri);
  PQexec(conn, "SET client_min_messages = 'DEBUG1'");

  // Check if the connection was successful
  if (PQstatus(conn) == CONNECTION_BAD) {
    fprintf(stderr, "Connection to database failed: %s\n",
            PQerrorMessage(conn));
    PQfinish(conn);
    exit(1);
  }

  const char* database_name = PQdb(conn);
  printf("Connected to database: %s\n", database_name);
}

PGresult* execute_query(PGconn* conn, const char* query) {
  PGresult* result = PQexec(conn, query);

  if (PQresultStatus(result) != PGRES_TUPLES_OK &&
      PQresultStatus(result) != PGRES_COMMAND_OK) {
    fprintf(stderr, "Query execution failed: %s\n", PQerrorMessage(conn));
    PQclear(result);
    return NULL;
  }
  return result;
}

void prettyPrint(PGresult* res, PQprintOpt* printOptions) {
  if (!res) {
    fprintf(stderr, "Query result is NULL\n");
    return;
  }

  if (!printOptions) {
    printOptions = &defaultPrintOptions;
  }

  // Print query results to stdout
  PQprint(stdout, res, printOptions);
}

// Initialize the arena allocator
static void arenaInit(ArenaAllocator* arena) {
  arena->used = 0;
  memset(arena->buffer, 0, ARENA_SIZE);
}

// Allocate memory from the arena
static void* arenaAlloc(ArenaAllocator* arena, size_t size) {
  if (arena->used + size <= ARENA_SIZE) {
    void* ptr = arena->buffer + arena->used;
    arena->used += size;
    return ptr;
  } else {
    return NULL;  // Arena is full
  }
}

void query(FlagArgs args) {
  const char* sql_query =
    *(const char**)FlagValue(args.flags, args.num_flags, "query");
  if (!sql_query) {
    fprintf(stderr, "Query string is not provided\n");
    return;
  }

  PGresult* query_result = execute_query(conn, sql_query);
  if (!query_result) {
    fprintf(stderr, "Query execution failed. Exiting...\n");
    return;
  }

  // Print query results to stdout
  prettyPrint(query_result, NULL);

  // Release resources
  PQclear(query_result);
}

void startSQLPrompt(FlagArgs args) {
  bool running = true;
  char* input = NULL;

  // Initialize the arena allocator
  ArenaAllocator arena;
  arenaInit(&arena);

  while (running) {
    char* line = readline("> ");
    if (!line) {
      break;  // Exit on EOF (e.g., Ctrl+D)
    }

    // handle special commands
    if (strcmp(line, "\\q") == 0 || strcmp(line, "\\exit") == 0) {
      // exit the program
      running = false;
      free(line);
      break;
    }

    if (strcmp(line, "\\h") == 0 || strcmp(line, "\\help") == 0) {
      // print help
      printf("\n\nAvailable commands:\n");
      printf("\\q, \\exit: Exit the program\n");
      printf("\\h, \\help: Print this help message\n\n");
      free(line);
      continue;
    }

    // Concatenate the input into the current SQL statement
    if (input) {
      size_t inputLen = strlen(input);
      size_t lineLen = strlen(line);

      // +2 for space and null terminator
      char* newInput = arenaAlloc(&arena, inputLen + lineLen + 2);
      if (newInput) {
        strcpy(newInput, input);
        strcat(newInput, " ");
        strcat(newInput, line);
        input = newInput;
      } else {
        fprintf(stderr, "Memory allocation failed. Exiting...\n");
        free(line);
        break;
      }
    } else {
      input = strdup(line);
    }

    // Check if the line ends with a semicolon
    if (line[strlen(line) - 1] == ';') {
      // Add the input to history
      add_history(line);

      // Execute the SQL statement
      PGresult* query_result = execute_query(conn, input);

      if (!query_result) {
        arena.used = 0;
        free(line);
        input = NULL;
        continue;
      }

      prettyPrint(query_result, NULL);
      PQclear(query_result);
      // Clear the current SQL statement
      arena.used = 0;
      input = NULL;
    }

    free(line);
  }

  // clear the arena when exiting
  arena.used = 0;
}
