#define _DEFAULT_SOURCE 1    //  This macro ensures fdopen, strdup are available(--std>=c99)
#define FLAG_IMPLEMENTATION  // Required for as flag.h is stb-style header-only lib.

#include <postgresql/libpq-fe.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "csvparser/csvparser.h"
#include "db.h"
#include "flag/flag.h"
#include "inventory.h"

#define ARRAY_SIZE(arr) (sizeof(arr) / (sizeof(arr[0])))
#define MAX_GLOBAL_FLAGS 10
#define MAX_SUBCOMMANDS 3

// Declare global Flags
static char* database_uri = "";
static char* queryString = "";
static char* csv_file = "";
static char* inventory_dept = "";
static char* billable_type = "";

void handleUploadCSV(FlagArgs args) {
  const char* csv_file = *(const char**)flag_value(args.flags, args.num_flags, "csv");
  const char* inventory_dept = *(const char**)flag_value(args.flags, args.num_flags, "dept");
  const char* billable_type = *(const char**)flag_value(args.flags, args.num_flags, "billable");

  uploadCSVFile(conn, csv_file, inventory_dept, billable_type);
}

int main(int argc, char* argv[]) {
  // Create the flag context.
  flag_ctx ctx = {0};
  flag_context_init(&ctx, MAX_GLOBAL_FLAGS, MAX_SUBCOMMANDS);

  // Add database URI global flag.
  flag_add(&ctx, "db", &database_uri, FLAG_STRING, "PG connection URI", false);

  // Initialize flags for query subcommand
  flag query_flags[] = {
    {"query", &queryString, FLAG_STRING, "Query string to execute", true, NULL},
  };

  // Initialize the flags for the upload subcommand.
  flag upload_flags[] = {
    {"csv", &csv_file, FLAG_STRING, "CSV file to upload", true, NULL},
    {"dept", &inventory_dept, FLAG_STRING, "Inventory department", true, NULL},
    {"billable", &billable_type, FLAG_STRING, "Billable type", true, NULL},
  };

  subcommand subcommands[] = {
    {
      .name = "exec",
      .description = "Execute a query",
      .callback = query,
      .flags = query_flags,
      .num_flags = ARRAY_SIZE(query_flags),
    },
    {
      .name = "sql",
      .description = "Start an interactive SQL prompt",
      .callback = startSQLPrompt,
      .flags = NULL,
    },
    {
      .name = "upload",
      .description = "Upload a CSV file",
      .callback = handleUploadCSV,
      .flags = upload_flags,
      .num_flags = ARRAY_SIZE(upload_flags),
    },
  };

  flag_add_subcommands(&ctx, subcommands, ARRAY_SIZE(subcommands));

  // Parse the flags and validate them.
  subcommand* matchingCmd = parse_flags(&ctx, argc, argv);

  // connect to the database as database_uri will be populated(if available)
  connect_to_database(database_uri);

  // If a subcommand matches, run it.
  if (matchingCmd) {
    subcommand_call(matchingCmd, &ctx);
  }

  // Free dynamically memory used by flag context.
  flag_destroy_context(&ctx);

  if (conn) {
    PQfinish(conn);
  }
  return 0;
}
