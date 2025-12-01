# Hash Indexing in PicoDB

## 📝 Overview

**Hash Indexing** in PicoDB provides O(1) average-case lookup time for equality queries. The index maps column values directly to file offsets where matching records are stored.

### Why Hash Indexing?

- **Fast Lookups**: O(1) average time for `WHERE col = value`
- **Multi-column Support**: Independent indexes for each column
- **Multiple Values**: Handles duplicate values (same value in multiple rows)
- **Simple Implementation**: Standard hash table (unordered_map)

---

## 🏗️ Index Structure

### In-Memory Data Structure

PicoDB uses a **nested hash map**:

```cpp
std::unordered_map<
    std::string,                           // Column name
    std::unordered_map<
        std::string,                       // Column value
        std::vector<uint64_t>              // List of file offsets
    >
> idx;
```

### Conceptual View

```
Hash Index (student table):

┌─────────────────────────────────────────────────┐
│  Column: "id"                                   │
│  ┌─────────────────────────────────────────┐    │
│  │ Value: "1"      → Offsets: [0]          │    │
│  │ Value: "2"      → Offsets: [28]         │    │
│  │ Value: "3"      → Offsets: [56]         │    │
│  └─────────────────────────────────────────┘    │
└─────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────┐
│  Column: "name"                                 │
│  ┌─────────────────────────────────────────┐    │
│  │ Value: "Ekram"  → Offsets: [0]          │    │
│  │ Value: "Opu"    → Offsets: [28]         │    │
│  │ Value: "Rafin"  → Offsets: [56]         │    │
│  └─────────────────────────────────────────┘    │
└─────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────┐
│  Column: "dept"                                 │
│  ┌─────────────────────────────────────────┐    │
│  │ Value: "IIT"    → Offsets: [0]          │    │
│  │ Value: "EEE"    → Offsets: [28]         │    │
|  | Value: "EEE"    → Offsets: [56]         |    |
│  └─────────────────────────────────────────┘    │
└─────────────────────────────────────────────────┘
```

**Note**: The "dept" column shows how multiple records can map to the same value!

---

## 🔧 Core Operations

### 1. Adding Records

```cpp
void HashIndex::addRecord(const string &col, const string &value, uint64_t offset) {
    string trimmedValue = trimSpaceC(value);
    string trimmedCol = trimSpaceC(col);
    
    idx[trimmedCol][trimmedValue].push_back(offset);
}
```

**Example**: After `INSERT INTO student VALUES(1, "Ekram", "IIT");`

```
addRecord("id", "1", 0)
addRecord("name", "Ekram", 0)
addRecord("dept", "IIT", 0)

Result:
idx["id"]["1"] = [0]
idx["name"]["Ekram"] = [0]
idx["dept"]["IIT"] = [0]
```

### 2. Finding Records

```cpp
vector<uint64_t> HashIndex::findRecord(const string &col, const string &value) {
    string trimmedValue = trimSpaceC(value);
    string trimmedCol = trimSpaceC(col);
    
    if (idx.count(trimmedCol) && idx[trimmedCol].count(trimmedValue)) {
        return idx[trimmedCol][trimmedValue];
    }
    return {};  // Empty vector if not found
}
```

**Example**: Query `SELECT * FROM student WHERE name = "Ekram";`

```
findRecord("name", "Ekram") → returns [0]
                              ↓
                    Read record at offset 0
```

### 3. Saving to Disk

Index is persisted as a binary file (`.hashidx`):

```cpp
void HashIndex::saveToDisk(const string &table) {
    string filePath = "data/" + table + "/" + table + ".hashidx";
    ofstream indexRecord(filePath, ios::binary);
    
    for (auto &columnEntry : idx) {
        const string &columnName = columnEntry.first;
        
        for (auto &columnValue : columnEntry.second) {
            const string &value = columnValue.first;
            const vector<uint64_t> &offsetList = columnValue.second;
            
            // Write: [ColumnName\0] [Value\0] [Count] [Offset1] [Offset2]...
            indexRecord.write(columnName.c_str(), columnName.size() + 1);
            indexRecord.write(value.c_str(), value.size() + 1);
            
            uint64_t offsetCount = offsetList.size();
            indexRecord.write((char*)&offsetCount, sizeof(offsetCount));
            
            for (auto &offset : offsetList) {
                indexRecord.write((char*)&offset, sizeof(offset));
            }
        }
    }
}
```

### 4. Loading from Disk

```cpp
void HashIndex::loadFromDisk(const string &table) {
    idx.clear();
    
    string filePath = "data/" + table + "/" + table + ".hashidx";
    ifstream indexRecordFile(filePath, ios::binary);
    
    while (indexRecordFile.peek() != EOF) {
        string columnName;
        getline(indexRecordFile, columnName, '\0');
        
        string columnValue;
        getline(indexRecordFile, columnValue, '\0');
        
        uint64_t offsetCount = 0;
        indexRecordFile.read((char*)&offsetCount, sizeof(offsetCount));
        
        vector<uint64_t> offsetList(offsetCount);
        for (uint64_t i = 0; i < offsetCount; i++) {
            indexRecordFile.read((char*)&offsetList[i], sizeof(uint64_t));
        }
        
        idx[columnName][columnValue] = offsetList;
    }
}
```

---

## 📊 File Format (.hashidx)

### Binary Layout

```
[Entry 1] [Entry 2] [Entry 3] ...
```

Each entry:
```
┌──────────────────┬──────────────┬─────────────┬──────────┬──────────┬─────┐
│ Column Name + \0 │ Value + \0   │ Count (8B)  │ Offset1  │ Offset2  │ ... │
└──────────────────┴──────────────┴─────────────┴──────────┴──────────┴─────┘
  Variable length     Variable       Fixed 8 bytes  8 bytes    8 bytes
```

### Real Example

After inserting:
```sql
INSERT INTO student VALUES(1, "Ekram", "IIT");
INSERT INTO student VALUES(2, "Opu", "EEE");
```

**Hexdump of `student.hashidx`**:

```
00000000: 69 64 00 31 00 01 00 00  00 00 00 00 00 00 00 00  |id.1............|
          ^^^^^ ^^  ^^^^^  ^^^^^^^^^^^^^^^^^^  ^^^^^^^^
          "id"  \0  "1"\0  Count = 1          Offset = 0
          
00000010: 00 69 64 00 32 00 01 00  00 00 00 00 00 00 1C 00  |.id.2...........|
          ^^ ^^^^^ ^^  ^^^^^  ^^^^^^^^^^^^^^^^^^  ^^^^^^^^
          \0 "id" \0  "2"\0  Count = 1          Offset = 28
          
00000020: 00 00 6E 61 6D 65 00 41  6C 69 63 65 00 01 00 00  |..name.Ekram....|
               ^^^^^^^^^ ^^  ^^^^^^^^^^^ ^^  ^^^^^^^^^^^^^^
               "name"    \0  "Ekram"    \0  Count = 1
               
00000030: 00 00 00 00 00 00 00 00  00 6E 61 6D 65 00 42 6F  |.........name.Opu|
          ^^^^^^^^^^^^^^^^^^  ^^^^^^^^^ ^^  ^^^^^^^
          Offset = 0          "name"    \0  "Opu"
          
00000040: 62 00 01 00 00 00 00 00  00 00 1C 00 00 00 ...
          ^^ ^^  ^^^^^^^^^^^^^^^^^^  ^^^^^^^^
          b  \0  Count = 1          Offset = 28
```

---

## 🎯 Example Walkthrough

### Scenario: Creating and Querying a Table

#### Step 1: Create Table

```sql
CREATE TABLE employee(id INT PRIMARY, name TEXT, dept TEXT, salary FLOAT);
```

**Result**: Metadata file created, no index yet.

---

#### Step 2: Insert First Record

```sql
INSERT INTO employee VALUES(101, "Rabi", "HR", 50000);
```

**Actions**:
1. Encode record → 18 bytes
2. Append to `.data` at offset **0**
3. Update index:
   ```
   idx["id"]["101"] = [0]
   idx["name"]["Rabi"] = [0]
   idx["dept"]["HR"] = [0]
   idx["salary"]["50000"] = [0]  // Note: stored as string internally
   ```
4. Save index to `.hashidx`

---

#### Step 3: Insert Second Record

```sql
INSERT INTO employee VALUES(102, "Rahi", "IT", 60000);
```

**Actions**:
1. Load existing index from disk
2. Encode record → 18 bytes
3. Append to `.data` at offset **18**
4. Update index:
   ```
   idx["id"]["101"] = [0]
   idx["id"]["102"] = [18]     ← New entry
   idx["name"]["Rabi"] = [0]
   idx["name"]["Rahi"] = [18]  ← New entry
   idx["dept"]["HR"] = [0]
   idx["dept"]["IT"] = [18]    ← New entry
   ...
   ```
5. Save updated index

---

#### Step 4: Insert Third Record (Duplicate Department)

```sql
INSERT INTO employee VALUES(103, "Rana", "IT", 55000);
```

**Actions**:
1. Load index
2. Append record at offset **36**
3. Update index:
   ```
   idx["id"]["103"] = [36]
   idx["name"]["Rana"] = [36]
   idx["dept"]["IT"] = [18, 36]  ← Multiple offsets!
   ```

**Note**: "IT" now maps to TWO offsets: 18 and 36

---

#### Step 5: Query by Name

```sql
SELECT * FROM employee WHERE name = "Rahi";
```

**Execution**:
```
1. Load index from .hashidx
2. Lookup: idx["name"]["Rahi"] → [18]
3. Read record at offset 18 from .data
4. Decode and display:
   | 102 | Jane | IT | 60000 |
```

**Time**: O(1) hash lookup + O(1) file seek

---

#### Step 6: Query by Department (Multiple Results)

```sql
SELECT * FROM employee WHERE dept = "IT";
```

**Execution**:
```
1. Load index
2. Lookup: idx["dept"]["IT"] → [18, 36]
3. For each offset:
   - Read record at offset 18 → | 102 | Rahi | IT | 60000 |
   - Read record at offset 36 → | 103 | Rana | IT | 55000 |
4. Display both results
```

---

## 📈 Performance Analysis

### Time Complexity

| Operation | Average Case | Worst Case | Notes |
|-----------|-------------|------------|-------|
| `addRecord` | O(1) | O(n) | Amortized for vector append |
| `findRecord` | O(1) | O(n) | Hash collision handling |
| `saveToDisk` | O(m×k) | O(m×k) | m = unique values, k = avg offsets |
| `loadFromDisk` | O(m×k) | O(m×k) | Same as save |

### Space Complexity

**In-Memory**:
```
Size = Σ (column_name_size + value_size + 8 × num_offsets)
```

**On-Disk (.hashidx)**:
```
Size = Σ (column_name_size + 1 + value_size + 1 + 8 + 8 × num_offsets)
```

### Example Space Calculation

For the employee table with 3 records:

```
Entries in index:
- id: "101", "102", "103"     → 3 entries × (2+3+1+8+8) = 66 bytes
- name: "Rabi", "Rahi", "Rana" → 3 entries × (4+4+1+8+8) = 75 bytes  
- dept: "HR", "IT" (2 offsets) → 2 entries × (4+2+1+8+16) = 62 bytes
- salary: 3 entries             → ~80 bytes

Total: ~283 bytes for 3 records
```

**Overhead per record**: ~94 bytes

---

## 🔍 Handling Edge Cases

### 1. Duplicate Values

```sql
INSERT INTO employee VALUES(201, "Ekram", "Sales", 45000);
INSERT INTO employee VALUES(202, "Opu", "Sales", 48000);
INSERT INTO employee VALUES(203, "Rafin", "Sales", 47000);
```

**Index state**:
```
idx["dept"]["Sales"] = [100, 120, 140]
                       ^^^^^^^^^^^^^^^^
                       All three offsets stored
```

**Query**: `SELECT * FROM employee WHERE dept = "Sales";`

**Result**: Returns all 3 records efficienValue: "EEE"    → Offsets: [28]tly (one hash lookup, three file reads)

---

### 2. Missing Index File

First insert after creating a table:

```cpp
void HashIndex::loadFromDisk(const string &table) {
    if (!fs::exists(filePath)) {
        cerr << "No hash index file found\n";
        return;  // idx remains empty, ready for new entries
    }
    // ...
}
```

**Behavior**: Starts with empty index, builds from scratch.

---

### 3. Primary Key Enforcement

Before inserting:

```cpp
if (!primaryColName.empty()) {
    string primaryKeyValue = cmd.values[primaryIdx];
    auto checkExist = globalHash.findRecord(primaryColName, primaryKeyValue);
    if (!checkExist.empty()) {
        cout << "Duplicate entry for primary key\n";
        return;  // Reject insert
    }
}
```

**Example**:
```sql
INSERT INTO student VALUES(1, "Ekram", "IIT");  -- OK
INSERT INTO student VALUES(1, "Opu", "EEE");    -- Error: Duplicate primary key
```

---

## 🚀 Optimization Techniques

### 1. Key Normalization

```cpp
void HashIndex::addRecord(const string &col, const string &value, uint64_t offset) {
    string trimmedValue = trimSpaceC(value);  // Remove leading/trailing spaces
    string trimmedCol = trimSpaceC(col);
    // ...
}
```

**Why**: Ensures `"Ekram"`, `" Ekram"`, and `"Ekram "` all map to the same key.

---

### 2. Lazy Loading

Index is only loaded when needed (SELECT, INSERT):

```cpp
globalHashSelect.loadFromDisk(cmd.table);  // Only in SELECT handler
```

**Benefit**: Reduces memory usage for large databases.

---

### 3. Incremental Updates

INSERT operation:
1. Load existing index
2. Add new entries
3. Save complete index

**Alternative** (future optimization): Append-only index log.

---

## 🎓 Comparison with Other Indexing

| Feature | Hash Index | B+ Tree | Full Scan |
|---------|-----------|---------|----------|
| Equality (`=`) | O(1) ✅ | O(log n) | O(n) |
| Range (`BETWEEN`) | ❌ | O(log n + k) ✅ | O(n) |
| Order-by | ❌ | ✅ | O(n log n) |
| Memory | Moderate | High | Low |
| Disk I/O | Random | Sequential | Sequential |
| Best for | Point queries | Range queries | Small tables |

---


## 📚 Further Reading

- **Hash Table Internals**: https://en.wikipedia.org/wiki/Hash_table
- **Database Indexing**: https://use-the-index-luke.com/
- **C++ unordered_map**: https://en.cppreference.com/w/cpp/container/unordered_map

---

**Hash indexing: Fast, simple, effective! 🚀**
