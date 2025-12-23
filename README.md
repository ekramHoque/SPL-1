# PicoDB - Lightweight File-Based Database

**PicoDB** is a compact, educational database management system implemented in C++17 that stores data in binary files with efficient indexing mechanisms. It supports SQL-like commands for table creation, data insertion, retrieval, and display.

---

## 🚀 Features

- **SQL-like Command Interface**: CREATE, INSERT, SELECT, SHOW commands
- **Multiple Indexing Methods**: Hash-based indexing and B+ Tree (planned)
- **Compression**: Varint encoding for efficient integer storage
- **Binary Storage**: Compact binary file format for data persistence
- **Type Support**: INT, TEXT, FLOAT, BOOL data types
- **Primary Key Constraints**: Enforce unique primary keys
- **WHERE Clause**: Query data with equality and BETWEEN conditions

---

## 📁 Project Structure

```
picodb/
├── src/
│   ├── main.cpp              # Entry point
│   ├── commands/             # Command execution logic
│   │   ├── create.cpp/h      # CREATE TABLE handler
│   │   ├── insert.cpp/h      # INSERT handler
│   │   ├── select.cpp/h      # SELECT handler
│   │   ├── show.cpp/h        # SHOW TABLE handler
│   │   └── utils.cpp/h       # Utility functions
│   ├── parser/
│   │   └── parser.cpp/h      # SQL command parser
│   ├── storage/
│   │   ├── file_manager.cpp/h   # File I/O operations
│   │   ├── varint.cpp/h         # Varint compression
│   │   └── bitfield.cpp/h       # Bitfield utilities
│   └── index/
│       ├── hash_index.cpp/h     # Hash table indexing
│       └── bplusTree_index.cpp/h # B+ tree (future)
├── data/                     # Database files (auto-created)
│   └── [table_name]/
│       ├── [table_name].meta    # Table schema
│       ├── [table_name].data    # Binary data records
│       └── [table_name].hashidx # Hash index file
└── docs/
    ├── readme_Compres.md     # Compression details
    └── readme_Hashing.md     # Hash indexing details
```

---

## 🛠️ Building and Running

### Prerequisites
- C++17 compatible compiler (g++ 7+, clang 5+)
- Linux/Unix environment

### Compilation
```bash
cd PicoDB
g++ -std=c++17 -O2 \
  -I./src -I./src/storage -I./src/commands -I./src/parser -I./src/index \
  -o picodb \
  src/main.cpp src/commands/*.cpp src/parser/*.cpp src/storage/*.cpp src/index/*.cpp
```

### Run
```bash
./picodb
```

---

## 📖 Usage Examples

### 1. Start PicoDB
```
=============================================
              PicoDB - File Database         
=============================================
Choose indexing method:
  1. Hashing 
  2. B+ Tree 
Enter option (1 or 2): 1
[INFO] Hashing index selected.
PicoDB> 
```

### 2. Create a Table
```sql
PicoDB> CREATE TABLE student(id INT PRIMARY, name TEXT, dept TEXT, gpa FLOAT);
[OK] Table student created sucessFully
[INFO] : Primary Key -> id
```

### 3. Insert Records
```sql
PicoDB> INSERT INTO student VALUES(1, "Ekram", "IIT", 3.7);
Insertes succesfully at offset 0

PicoDB> INSERT INTO student VALUES(2, "Opu", "EEE", 3.5);
Insertes succesfully at offset 28

PicoDB> INSERT INTO student VALUES(3, "Rafin", "CSE", 3.9);
Insertes succesfully at offset 56
```

### 4. Display All Records
```sql
PicoDB> SHOW TABLE student;
-------------------------------------------------
| id | name | dept | gpa 
-------------------------------------------------
| 1 | Ekram | IIT | 3.7 
| 2 | Opu | EEE | 3.5 
| 3 | Rafin | CSE | 3.9 
-------------------------------------------------
```

### 5. Query with WHERE Clause
```sql
PicoDB> SELECT * FROM student WHERE name = "Ekram";
[INFO] Search for name
-------------------------------------------------
| id | name | dept | gpa 
-------------------------------------------------
| 1 | Ekram | IIT | 3.7

PicoDB> SELECT * FROM student WHERE id = 2;
[INFO] Search for id
-------------------------------------------------
| id | name | dept | gpa 
-------------------------------------------------
| 2 | Opu | EEE | 3.5 
```

### 6. Exit
```sql
PicoDB> exit
Exiting PicoDB. Goodbye!
```

---

## 🏗️ System Architecture

### High-Level Data Flow

```
┌──────────────┐
│     User     │
└──────┬───────┘
       │ SQL Command
       ▼
┌──────────────────┐
│  Parser Module   │  ◄── Regex-based SQL parsing
└──────┬───────────┘
       │ ParsedCommand struct
       ▼
┌──────────────────┐
│ Command Executor │  ◄── Route to appropriate handler
└──────┬───────────┘
       │
       ├─────────────┬─────────────┬──────────────┐
       ▼             ▼             ▼              ▼
   ┌────────┐   ┌────────┐   ┌────────┐    ┌────────┐
   │ CREATE │   │ INSERT │   │ SELECT │    │  SHOW  │
   └────┬───┘   └────┬───┘   └────┬───┘    └────┬───┘
        │            │            │             │
        ▼            ▼            ▼             ▼
   ┌──────────────────────────────────────────────┐
   │          File Manager (Storage Layer)        │
   │  • writeMeta()  • readMeta()                 │
   │  • appendRecord()  • readRecord()            │
   └──────────────┬───────────────────────────────┘
                  │
        ┌─────────┴─────────┐
        ▼                   ▼
   ┌─────────┐         ┌──────────┐
   │  Index  │         │   Data   │
   │  Files  │         │   Files  │
   │ (.hashidx)│       │  (.data) │
   └─────────┘         └──────────┘
```

### Storage Format Details

#### 1. **Metadata File (`.meta`)**
Stores table schema in plain text:
```
COLUMN: 3
id INTPRIMARY
name TEXT
dept TEXT
```

#### 2. **Data File (`.data`)**
Binary format with Varint-encoded record lengths:
```
[Varint: RecordLength] [Record Data]
[Varint: RecordLength] [Record Data]
...
```

**Record Structure:**
```
[TypeFlag: 1 byte] [Value Data]
[TypeFlag: 1 byte] [Value Data]
...
```

Type Flags:
- `'I'` = Integer (Varint-encoded)
- `'S'` = String (Varint length + raw bytes)
- `'F'` = Float (4 bytes, IEEE 754)
- `'B'` = Boolean (1 byte: 0 or 1)

#### 3. **Hash Index File (`.hashidx`)**
Binary format mapping column values to file offsets:
```
[ColumnName\0] [Value\0] [OffsetCount: 8 bytes] [Offset1: 8 bytes] [Offset2: 8 bytes] ...
```

---

## 🔄 Detailed Flow Diagrams

### INSERT Operation Flow

```
┌─────────────────────────────────────────────────────────┐
│ INSERT INTO student VALUES(1, "Ekram", "IIT")          │
└────────────────────┬────────────────────────────────────┘
                     ▼
            ┌─────────────────┐
            │  Parse Command  │
            └────────┬────────┘
                     ▼
            ┌─────────────────┐
            │  Read .meta     │ ◄─── Load schema
            └────────┬────────┘
                     ▼
            ┌─────────────────┐
            │ Load Hash Index │ ◄─── Load existing index from disk
            └────────┬────────┘
                     ▼
            ┌─────────────────┐
            │ Check Primary   │ ◄─── Verify uniqueness
            │ Key Constraint  │
            └────────┬────────┘
                     ▼
            ┌─────────────────┐
            │ Encode Record:  │
            │  'I' + varint(1)│ ◄─── Compress data
            │  'S' + len +    │
            │      "Ekram"    │
            │  'S' + len +    │
            │      "IIT"      │
            └────────┬────────┘
                     ▼
            ┌─────────────────┐
            │ Append to .data │ ◄─── Returns offset (e.g., 0)
            └────────┬────────┘
                     ▼
            ┌─────────────────┐
            │ Update Index:   │
            │  id -> 1 -> [0] │ ◄─── Map each column/value to offset
            │  name -> Ekram  │
            │         -> [0]  │
            │  dept -> IIT    │
            │         -> [0]  │
            └────────┬────────┘
                     ▼
            ┌─────────────────┐
            │ Save Index to   │ ◄─── Persist index to disk
            │   .hashidx      │
            └─────────────────┘Ekram
```

### SELECT Operation Flow

```
┌─────────────────────────────────────────────────────────┐
│ SELECT * FROM student WHERE name = "Ekram"              │
└────────────────────┬────────────────────────────────────┘
                     ▼
            ┌─────────────────┐
            │  Parse Command  │
            │  Extract:       │
            │   - table: student
            │   - column: name │
            │   - value: Ekram │
            └────────┬────────┘
                     ▼
            ┌─────────────────┐
            │  Read .meta     │ ◄─── Load schema
            └────────┬────────┘
                     ▼
            ┌─────────────────┐
            │ Load Hash Index │ ◄─── Load index into memory
            │   from .hashidx │
            └────────┬────────┘
                     ▼
            ┌─────────────────┐
            │   Hash Lookup:  │
            │ idx["name"]     │ ◄─── O(1) average lookup
            │    ["Ekram"]    │
            │    → [0]        │
            └────────┬────────┘
                     ▼
            ┌─────────────────┐
            │ Read Record at  │
            │   offset 0      │ ◄─── Seek to position in .data
            └────────┬────────┘
                     ▼
            ┌─────────────────┐
            │ Decode Record:  │
            │  Parse varint   │ ◄─── Decompress data
            │  Extract fields │
            └────────┬────────┘
                     ▼
            ┌─────────────────┐
            │ Display Result  │
            └─────────────────┘
```

---

## 🎯 Key Design Decisions

### 1. **Varint Compression**
- Small integers use fewer bytes (e.g., `1` = 1 byte instead of 8)
- Details: See [docs/readme_Compres.md](docs/readme_Compres.md)

### 2. **Hash Index Structure**
- Multi-level map: `Column → Value → [Offsets]`
- Supports multiple records with same value
- Details: See [docs/readme_Hashing.md](docs/readme_Hashing.md)

### 3. **Binary Storage**
- Type flags distinguish data types at runtime
- Self-describing format for schema evolution

### 4. **Parser Design**
- Regex-based for simplicity
- Handles quotes, semicolons, whitespace variations

---

## 📊 Performance Characteristics

| Operation | Time Complexity | Space Complexity |
|-----------|----------------|------------------|
| INSERT    | O(1) average   | O(n) index size  |
| SELECT (indexed) | O(1) average | O(1) per result |
| SHOW      | O(n)           | O(1)             |
| CREATE    | O(1)           | O(m) metadata    |

*where n = number of records, m = number of columns*

---

## 🧪 Testing Example

```bash
# Clean start
rm -rf data/

# Run PicoDB
./picodb

# Test session:
1  # Select hashing
CREATE TABLE employee(id INT PRIMARY, name TEXT, salary FLOAT);
INSERT INTO employee VALUES(101, "Roni", 50000.5);
INSERT INTO employee VALUES(102, "Rahi", 60000.0);
SHOW TABLE employee;
SELECT * FROM employee WHERE id = 101;
exit
```

---

## 🔍 File Format Examples

### After creating and inserting data:

**data/student/student.meta:**
```
COLUMN: 3
id INTPRIMARY
name TEXT
dept TEXT
```

**data/student/student.data (hexdump):**
```
00000000: 09 49 01 53 05 41 6c 69 63 65 53 03 43 53 45  .I.S.Ekram.S.IIT
          ^  ^  ^  ^  ^  ^^^^^^^^^^^  ^  ^  ^^^^^^^  
          │  │  │  │  │      └─ "Ekram"
          │  │  │  │  └─ Length: 5
          │  │  │  └─ Type: String
          │  │  └─ Varint: 1
          │  └─ Type: Integer  
          └─ Record length: 9 bytes
```

**data/student/student.hashidx (structure):**
```
id\0 1\0 [count=1] [offset=0]
name\0 Ekram\0 [count=1] [offset=0]
dept\0 IIT\0 [count=1] [offset=0]
```

---

## 🚧 Limitations & Future Work

### Current Limitations
- No UPDATE or DELETE commands
- No JOIN operations
- Single-user, file-locking not implemented
- Limited WHERE operators (only `=` supported)
- No BETWEEN implementation yet(later)

### Planned Features
- [ ] B+ Tree indexing for range queries
- [ ] UPDATE and DELETE operations
- [ ] Transaction support
- [ ] Multi-table queries (JOIN)
- [ ] Query optimization
- [ ] Crash recovery

---

## 📚 Additional Documentation

- **[Compression Details](docs/readme_Compres.md)**: Varint encoding algorithm
- **[Hash Indexing](docs/readme_Hashing.md)**: Hash table implementation
- **[B+ Tree](docs/readme_BPlusTree.md)**: Planned tree-based indexing

---

## 🤝 Contributing

This is an educational project. Contributions and improvements are welcome!

---

## 📄 License

Educational use - see repository for details.

---

**Built with ❤️ for learning database internals**
