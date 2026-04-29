# PicoDB Quick Start

PicoDB is a small file-based database with standalone and client-server modes.

## Project Info

- Author: Ekramul Hoque
- Program: 3rd Semester Final Project, IIT, University of Dhaka
- Supervisor: Kazi Muheymin-Us-Sakib
- Title: Professor, University of Dhaka

## 1) Build

From the project root, run:

```bash
make
```

This creates:
- `picodb` for standalone mode
- `picodb_server` for server mode
- `picodb_client` for client mode

## 2) Standalone Mode

Run the database directly:

```bash
./picodb
```

Then choose the index type:
- `1` = Hashing
- `2` = B+ Tree

## 3) Multi-Client Mode on the Same PC

Terminal 1:

```bash
./picodb_server 8080
```

Terminal 2, 3, 4...:

```bash
./picodb_client 127.0.0.1 8080
```

You can open many client windows on the same machine and connect them to the same server.

## 4) Multi-Client Mode on Different PCs

On the server PC, run:

```bash
./picodb_server 8080
```

Find the server PC IP address, then on each client PC run:

```bash
./picodb_client <server-ip> 8080
```

Make sure both PCs are on the same network and the server port is open.

## 5) Commands

Short list of supported commands:
- `CREATE TABLE` - create a new table
- `INSERT INTO` - add a record
- `SHOW TABLE` - show table data
- `SELECT` - read records with a condition
- `UPDATE` - change matching records
- `DELETE` - remove matching records
- `quit` / `exit` / `\q` - close the client or standalone shell

## 6) Quick Example

```sql
CREATE TABLE student(id INT PRIMARY, name TEXT, dept TEXT);
INSERT INTO student VALUES(1, "Ekram", "IIT");
SHOW TABLE student;
SELECT * FROM student WHERE id = 1;
```

## Notes

- In standalone mode, PicoDB asks for the index type at startup.
