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
  char* csv_file = *(char**)FlagValue(args.flags, args.num_flags, "csv");
  char* inventory_dept = *(char**)FlagValue(args.flags, args.num_flags, "dept");
  char* billable_type =
    *(char**)FlagValue(args.flags, args.num_flags, "billable");

  uploadCSVFile(conn, csv_file, inventory_dept, billable_type);
}

int main(int argc, char* argv[]) {
  // Create the flag context.
  flag_ctx* ctx = CreateFlagContext();

  // Add database URI global flag.
  AddFlag(ctx, .name = "db", .value = &database_uri, .type = FLAG_STRING,
          .desc = "PG connection URI", .req = false);

  // Execute a query subcommand
  subcommand* execCmd =
    AddSubCmd(ctx, .name = "exec", .desc = "Execute a query", .handler = query,
              .capacity = 1);

  AddSubCmdFlag(execCmd, .name = "query", .value = &queryString,
                .type = FLAG_STRING, "Query string to execute", .req = true);

  // upload subcommand
  subcommand* uploadCmd =
    AddSubCmd(ctx, .name = "upload", .desc = "Upload a CSV file for price list",
              .handler = handleUploadCSV, .capacity = 3);

  AddSubCmdFlag(uploadCmd, .name = "csv", .value = &csv_file,
                .type = FLAG_STRING, "CSV file to upload", .req = true);
  AddSubCmdFlag(uploadCmd, .name = "dept", .value = &inventory_dept,
                .type = FLAG_STRING, "Inventory department", .req = true);
  AddSubCmdFlag(uploadCmd, .name = "billable", .value = &billable_type,
                .type = FLAG_STRING, "Billable type", .req = true);

  // sql subcommand
  AddSubCmd(ctx, .name = "sql", .desc = "Start an interactive SQL prompt",
            .handler = startSQLPrompt, .capacity = 0);

  // Parse the flags and validate them.
  subcommand* matchingCmd = ParseFlags(ctx, argc, argv);

  // connect to the database as database_uri will be populated(if available)
  connect_to_database(database_uri);

  // If a subcommand matches, run it.
  if (matchingCmd) {
    InvokeSubCmd(matchingCmd, ctx);
  }

  // Free dynamically memory used by flag context.
  DestroyFlagContext(ctx);

  if (conn) {
    PQfinish(conn);
  }
  return 0;
}
