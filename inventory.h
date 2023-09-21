#ifndef __INVENTORY_H__
#define __INVENTORY_H__

#include <postgresql/libpq-fe.h>
#include "csvparser/csvparser.h"

// Billable types
#define Consultation "Consultation"
#define Drug "Drug"
#define Investigation "Investigation"
#define Procedure "Procedure"
#define Consumable "Consumable"
#define Accommodation "Accommodation"
#define NursingCare "NursingCare"
#define MedicalReviews "MedicalReviews"
#define Miscellaneous "Miscellaneous"

// Inventory departments
#define LABORATORY "laboratory"
#define RADIOLOGY "radiology"
#define PHARMACY "pharmacy"
#define NOT_APPLICABLE "not_applicable"

typedef struct InventoryItem {
  char Name[255];
  unsigned int CashPrice;
} InventoryItem;

// Validate inventory department for the file.
bool validateDepartment(const char* inventory_dept);

// Guess billable type from inventory dept and write it to billable_type.
bool validateBillableType(const char* billable_type);

// Upload CSV file to the database.
void uploadCSVFile(PGconn* conn, const char* filename, const char* inventory_dept,
                   const char* billable_type);

#endif /* __INVENTORY_H__ */
