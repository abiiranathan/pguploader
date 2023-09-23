//  This macro ensures fdopen, strdup are available(--std>=c99)
#define _DEFAULT_SOURCE 1

#include <postgresql/libpq-fe.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "csvparser/csvparser.h"
#include "db.h"
#include "flag/flag.h"
#include "inventory.h"

#define ARRAY_SIZE(arr) (sizeof(arr) / (sizeof(arr[0])))

// Declare global Flags
static char* database_uri = "";
static char* queryString = "";
static char* csv_file = "";
static char* inventory_dept = "";
static char* billable_type = "";

void handleUploadCSV(FlagArgs args) {
  char* csv_file = *(char**)flag_value(args.flags, args.num_flags, "csv");
  char* inventory_dept =
    *(char**)flag_value(args.flags, args.num_flags, "dept");
  char* billable_type =
    *(char**)flag_value(args.flags, args.num_flags, "billable");

  uploadCSVFile(conn, csv_file, inventory_dept, billable_type);
}

int main(int argc, char* argv[]) {
  // Create the flag context.
  flag_ctx* ctx = flag_context_init();

  // Add database URI global flag.
  flag_add(ctx, "db", &database_uri, FLAG_STRING, "PG connection URI", false);

  // Execute a query subcommand
  subcommand* execCmd =
    flag_add_subcommand(ctx, "exec", "Execute a query", query, 1);
  subcommand_add_flag(execCmd, "query", &queryString, FLAG_STRING,
                      "Query string to execute", true, NULL);

  // upload subcommand
  subcommand* uploadCmd = flag_add_subcommand(
    ctx, "upload", "Upload a CSV file for price list", handleUploadCSV, 3);

  subcommand_add_flag(uploadCmd, "csv", &csv_file, FLAG_STRING,
                      "CSV file to upload", true, NULL);
  subcommand_add_flag(uploadCmd, "dept", &inventory_dept, FLAG_STRING,
                      "Inventory department", true, NULL);
  subcommand_add_flag(uploadCmd, "billable", &billable_type, FLAG_STRING,
                      "Billable type", true, NULL);

  // sql subcommand
  flag_add_subcommand(ctx, "sql", "Start an interactive SQL prompt",
                      startSQLPrompt, 0);

  // Parse the flags and validate them.
  subcommand* matchingCmd = parse_flags(ctx, argc, argv);

  // connect to the database as database_uri will be populated(if available)
  connect_to_database(database_uri);

  // If a subcommand matches, run it.
  if (matchingCmd) {
    subcommand_call(matchingCmd, ctx);
  }

  // Free dynamically memory used by flag context.
  flag_destroy_context(ctx);

  if (conn) {
    PQfinish(conn);
  }
  return 0;
}
