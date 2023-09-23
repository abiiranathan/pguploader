#ifndef __DB_H__
#define __DB_H__
#define _DEFAULT_SOURCE                                                        \
  1  //  This macro ensures fdopen, strdup are available(--std>=c99)

#include <postgresql/libpq-fe.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "flag/flag.h"

#define ARENA_SIZE 4096  // Arena buffer size

// Structure for the arena allocator
typedef struct {
  char buffer[ARENA_SIZE];
  size_t used;
} ArenaAllocator;

extern PGconn* conn;
extern PQprintOpt defaultPrintOptions;

// Called from main to connect to the database.
void connect_to_database(const char* database_uri);

// Helper to execute a query and return the result.
PGresult* execute_query(PGconn* conn, const char* query);

// pretty-print sql result in psql style.
void prettyPrint(PGresult* res, PQprintOpt* printOptions);

// Callback function for query subcommand.
void query(FlagArgs args);

// callback function for sql subcommand.
void startSQLPrompt(FlagArgs args);

#endif /* __DB_H__ */
