# pguploader

Postgresql database inventory(price lists) uploader for Eclinic HMS project.

## Installation

```bash
git clone https://github.com/abiiranathan/pguploader.git
cd pguploader
make
```

## Usage

```bash
./bin/pguploader -h
```

````bash
./bin/pguploader
Global flags:
  -db --db(Optional) <char *>: PG connection URI

Subcommands:
  exec: Execute a query
    -query    --query(Required) <char *>: Query string to execute

  sql: Start an interactive SQL prompt

  upload: Upload a CSV file
    -csv      --csv(Required) <char *>: CSV file to upload
    -dept     --dept(Required) <char *>: Inventory department
    -billable --billable(Required) <char *>: Billable type

``

```bash
./bin/pguploader -db postgres://username:password@localhost:5432/eclinic upload -csv ~/Downloads/price_list.csv -dept 'pharmacy' -billable 'Drug'
````

### Departments

- pharmacy
- laboratory
- radiology
- not_applicable

### Billable Types

- Drug
- Consultation
- Procedure
- Investigation
- Consumable
- Accommodation
- NursingCare
- MedicalReviews
- Miscellaneous

### Drugs

- Department: pharmacy
- Billable: Drug

### Consultations

- Department: not_applicable
- Billable: Consultation

### Procedures

- Department: not_applicable
- Billable: Procedure

### Laboratory Tests

- Department: laboratory
- Billable: Investigation

### Radiology Tests

- Department: radiology
- Billable: Investigation

### Consumables

- Department: not_applicable
- Billable: Consumable

### Accommodation

- Department: not_applicable
- Billable: Accommodation

### Nursing Care

- Department: not_applicable
- Billable: NursingCare

### Medical Reviews

- Department: not_applicable
- Billable: MedicalReviews

### Miscellaneous

- Department: not_applicable
- Billable: Miscellaneous

## License

MIT License

## Author

Dr. Abiira Nathan
