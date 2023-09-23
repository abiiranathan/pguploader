#include "inventory.h"
#include "db.h"

const char* itemInsertStmt =
  "INSERT INTO inventory_items (name, dept, type, quantity) "
  "VALUES ($1, $2, $3, 0) ON CONFLICT (name, type) DO NOTHING RETURNING id";

const char* priceInsertStmt =
  "INSERT INTO prices (item_id, cash, uap,san_care, jubilee, prudential, aar, "
  "saint_catherine, icea, liberty) VALUES ($1, "
  "$2,0,0,0,0,0,0,0, 0) ON CONFLICT (item_id) DO UPDATE SET cash = "
  "$2 WHERE prices.item_id = $1";

static void parseIntoItem(CsvRow* row, InventoryItem* item) {
  strncpy(item->Name, row->fields[0], sizeof(item->Name) - 1);
  char* endptr;
  if (strcmp(row->fields[1], "") == 0) {
    item->CashPrice = 0;
  } else {
    item->CashPrice = strtoul(row->fields[1], &endptr, 10);
    // check possible overflow or underflow and set cash to 0
    if (endptr == row->fields[1] || *endptr != '\0') {
      item->CashPrice = 0;
    }
  }
}

// Validate inventory department for the file.
bool validateDepartment(const char* inventory_dept) {
  int size = 4;
  char* validTypes[4] = {LABORATORY, RADIOLOGY, PHARMACY, NOT_APPLICABLE};

  for (int i = 0; i < size; i++) {
    if (strcmp(validTypes[i], inventory_dept) == 0) {
      return true;
    }
  }
  return false;
}

bool validateBillableType(const char* billable_type) {
  char* validTypes[9] = {
    Consultation,  Drug,        Investigation,  Procedure,     Consumable,
    Accommodation, NursingCare, MedicalReviews, Miscellaneous,
  };

  for (int i = 0; i < 9; i++) {
    if (strcmp(validTypes[i], billable_type) == 0) {
      return true;
    }
  }
  return false;
}

static void exitOnError(PGresult* res, const char* msg) {
  if (!res)
    return;

  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    fprintf(stderr, "%s\n", msg);
    PQclear(res);
    exit(EXIT_FAILURE);
  }
}

// Upload CSV file to the database.
void uploadCSVFile(PGconn* conn, const char* filename,
                   const char* inventory_dept, const char* billable_type) {

  printf("[DEGUG]: filename=%s\n", filename);
  printf("[DEGUG]: inventory_dept=%s\n", inventory_dept);
  printf("[DEGUG]: billable_type=%s\n", billable_type);

  // Check if the department is valid
  if (!validateDepartment(inventory_dept)) {
    fprintf(stderr, "ERROR: invalid inventory department\n");
    exit(EXIT_FAILURE);
  }

  // Check if the billable type is valid
  if (!validateBillableType(billable_type)) {
    fprintf(stderr, "ERROR: invalid billable type\n");
    exit(EXIT_FAILURE);
  }

  int fd = csv_fdopen(filename);
  if (fd == -1) {
    fprintf(stderr, "ERROR: unable to open file %s for reading\n", filename);
    exit(EXIT_FAILURE);
  }

  bool success = true;
  PGresult* res = NULL;
  CsvParser* parser = csv_new_parser(fd);
  if (!parser) {
    fprintf(stderr, "ERROR: unable to creater new csv parser\n");
    goto cleanup;
  }

  // Ignore the first line of the CSV file
  csv_set_skip_header(parser, true);

  CsvRow** rows = csv_parse(parser);
  if (!rows) {
    fprintf(stderr, "ERROR: unable to parse csv file\n");
    goto cleanup;
  }

  res = PQprepare(conn, "insert_inventory_item", itemInsertStmt, 3, NULL);
  exitOnError(
    res, "ERROR: unable to prepare insert_inventory_item prepared statement");

  res = PQprepare(conn, "insert_price", priceInsertStmt, 2, NULL);
  exitOnError(res, "ERROR: unable to prepare insert_price statement");

  // BEGIN TRANSACTION
  execute_query(conn, "BEGIN");

  int num_rows = csv_get_numrows(parser);
  for (size_t i = 0; i < num_rows; i++) {
    CsvRow* row = rows[i];

    // Get the values from the row into the InventonryItem struct
    InventoryItem item;
    parseIntoItem(row, &item);

    // Check if the item is valid
    if (strcmp(item.Name, "") == 0) {
      fprintf(stderr, "ERROR: empty item name at index %zu, skipping row\n", i);
      success = false;
      goto cleanup;
    }

    // Insert inventory item.
    const char* paramValues[3] = {item.Name, inventory_dept, billable_type};
    res = PQexecPrepared(conn, "insert_inventory_item", 3, paramValues, NULL,
                         NULL, 0);
    if (PQresultStatus(res) != PGRES_COMMAND_OK &&
        PQresultStatus(res) != PGRES_TUPLES_OK) {
      fprintf(stderr, "ERROR: unable to insert inventory item\n");
      fprintf(stderr, "[ERROR]: %s\n", PQerrorMessage(conn));
      success = false;
      goto cleanup;
    }

    int numRows = PQntuples(res);
    if (numRows == 0) {
      continue;  // possibly already exists. Error ignored by DO NOTHING
    }

    // retrieve the item Id
    char* id = PQgetvalue(res, 0, PQfnumber(res, "id"));
    int id_int = atoi(id);
    if (id_int <= 0) {
      fprintf(stderr, "ERROR: unable to get id of inserted row\n");
      success = false;
      goto cleanup;
    }


    // Check if the cash price is > 0
    if (item.CashPrice > 0) {
      // convert the cash price to string
      char cash_price[20];
      snprintf(cash_price, sizeof(cash_price) - 1, "%u", item.CashPrice);
      cash_price[sizeof(cash_price) - 1] = '\0';

      // execute the prepared statement
      const char* paramValues[2] = {id, cash_price};
      res = PQexecPrepared(conn, "insert_price", 2, paramValues, NULL, NULL, 0);
      if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr,
                "ERROR: unable to execute insert_price prepared statement");
        fprintf(stderr, "[ERROR]: %s\n", PQerrorMessage(conn));
        success = false;
        goto cleanup;
      }
    }
  }

cleanup:
  // Free resources
  csv_parser_free(parser);

  // COMMIT TRANSACTION if successful, ROLLBACK otherwise
  if (success) {
    execute_query(conn, "COMMIT");
  } else {
    execute_query(conn, "ROLLBACK");
  }

  PQclear(res);
}
