# Varint Compression in PicoDB

## 📝 Overview

**Varint (Variable-length Integer)** is a compression technique that encodes integers using fewer bytes for smaller values. PicoDB uses Varint encoding to optimize storage for integer fields, reducing file sizes significantly.

### Why Varint?

- **Space Efficiency**: Small integers (0-127) use only 1 byte instead of 8 bytes (standard uint64_t)
- **Common in Databases**: Used in Protocol Buffers, SQLite, LevelDB
- **Self-Describing**: No external metadata needed to decode

---

## 🔧 How Varint Works

### Encoding Algorithm

Each byte in a Varint:
- **Bit 7 (MSB)**: Continuation bit
  - `1` = More bytes follow
  - `0` = Last byte of the number
- **Bits 0-6**: 7 bits of actual data

### Encoding Process

```cpp
std::vector<uint8_t> encode(uint64_t value) {
    std::vector<uint8_t> out;
    while (value >= 0x80) {              // While value >= 128
        out.push_back((value & 0x7F) | 0x80); // Take lower 7 bits + continuation bit
        value >>= 7;                     // Shift right by 7 bits
    }
    out.push_back(value);                // Last byte (no continuation bit)
    return out;
}
```

### Decoding Process

```cpp
uint64_t decode(const std::vector<uint8_t> &buffer, size_t startIndex, size_t &readBytes) {
    uint64_t value = 0;
    size_t shift = 0;
    readBytes = 0;
    
    for (size_t i = startIndex; i < buffer.size(); ++i) {
        uint8_t byte = buffer[i];
        value |= (byte & 0x7F) << shift;  // Extract 7 data bits
        readBytes++;
        if ((byte & 0x80) == 0) break;    // Check continuation bit
        shift += 7;
    }
    return value;
}
```

---

## 📊 Encoding Examples

### Example 1: Small Integer (1)

**Decimal**: `1`  
**Binary**: `0000 0001`  
**Varint**: `0x01` (1 byte)

```
Bit layout:
┌───┬───┬───┬───┬───┬───┬───┬───┐
│ 0 │ 0 │ 0 │ 0 │ 0 │ 0 │ 0 │ 1 │  = 0x01
└─┬─┴───┴───┴───┴───┴───┴───┴───┘
  │
  └─ Continuation bit = 0 (last byte)
```

**Savings**: 7 bytes (87.5% reduction from 8 bytes)

---

### Example 2: Medium Integer (300)

**Decimal**: `300`  
**Binary**: `0000 0001 0010 1100`  
**Varint**: `0xAC 0x02` (2 bytes)

#### Step-by-step encoding:

1. **300 in binary**: `100101100` (9 bits needed)

2. **Split into 7-bit chunks** (right to left):
   - Chunk 1: `0101100` (lower 7 bits)
   - Chunk 2: `0000010` (upper bits)

3. **Add continuation bits**:
   - Byte 1: `1` + `0101100` = `10101100` = `0xAC`
   - Byte 2: `0` + `0000010` = `00000010` = `0x02`

```
Byte 1:                    Byte 2:
┌───┬───────────────┐      ┌───┬───────────────┐
│ 1 │ 0 1 0 1 1 0 0│      │ 0 │ 0 0 0 0 0 1 0│
└─┬─┴───────────────┘      └─┬─┴───────────────┘
  │                          │
  └─ More bytes follow       └─ Last byte
```

#### Decoding 300:

```
1. Read 0xAC: Extract 0101100, shift = 0
   value = 0101100 = 44
   
2. Read 0x02: Extract 0000010, shift = 7
   value = 44 | (2 << 7) = 44 | 256 = 300
```

**Savings**: 6 bytes (75% reduction)

---

### Example 3: Large Integer (16,384)

**Decimal**: `16,384`  
**Binary**: `0100 0000 0000 0000`  
**Varint**: `0x80 0x80 0x01` (3 bytes)

#### Encoding breakdown:

1. **16384 in binary**: `100000000000000` (15 bits)

2. **Split into 7-bit chunks**:
   - Chunk 1: `0000000`
   - Chunk 2: `0000000`
   - Chunk 3: `0000001`

3. **Add continuation bits**:
   - Byte 1: `10000000` = `0x80`
   - Byte 2: `10000000` = `0x80`
   - Byte 3: `00000001` = `0x01`

```
Byte 1:         Byte 2:         Byte 3:
┌───┬───────┐   ┌───┬───────┐   ┌───┬───────┐
│ 1 │0000000│   │ 1 │0000000│   │ 0 │0000001│
└───┴───────┘   └───┴───────┘   └───┴───────┘
```

**Savings**: 5 bytes (62.5% reduction)

---

## 📈 Compression Efficiency Table

| Value Range | Std Size | Varint Size | Savings |
|-------------|----------|-------------|----------|
| 0 - 127 | 8 bytes | 1 byte | 87.5% |
| 128 - 16,383 | 8 bytes | 2 bytes | 75.0% |
| 16,384 - 2,097,151 | 8 bytes | 3 bytes | 62.5% |
| 2,097,152 - 268,435,455 | 8 bytes | 4 bytes | 50.0% |
| 268,435,456 - 34,359,738,367 | 8 bytes | 5 bytes | 37.5% |
| ... | ... | ... | ... |
| Max (2^64-1) | 8 bytes | 10 bytes | -25.0% ⚠️ |

⚠️ **Note**: Very large numbers (> 2^56) use more space than fixed 8 bytes.

---

## 🎯 Use Cases in PicoDB

### 1. Record Length Prefix

Each record in the `.data` file starts with a Varint-encoded length:

```
[Varint: Length] [Record Data]
```

**Example**: Record of 15 bytes
```
File bytes: 0x0F [15 bytes of data]
            ^^^^  
            Length encoded as Varint
```

### 2. String Length

String fields store their length as Varint:

```
['S'] [Varint: Length] [String bytes]
```

**Example**: "Ekram" (5 characters)
```
Hex: 53 05 41 6C 69 63 65
     ^  ^  ^^^^^^^^^^^^^^^
     │  │  └─ "Ekram"
     │  └─ Length: 5 (Varint)
     └─ Type flag: 'S'
```

### 3. Integer Values

INT fields are Varint-encoded:

```
['I'] [Varint: Value]
```

**Example**: ID = 42
```
Hex: 49 2A
     ^  ^^
     │  └─ 42 as Varint (1 byte)
     └─ Type flag: 'I'
```

---

## 🔍 Real Record Example

Let's encode this record:
```
id: 1
name: "Opu"
dept: "EEE"
```

### Step-by-step encoding:

#### Field 1: id = 1
- Type flag: `'I'` = `0x49`
- Value: `1` → Varint = `0x01`
- **Bytes**: `49 01`

#### Field 2: name = "Opu"
- Type flag: `'S'` = `0x53`
- Length: `3` → Varint = `0x03`
- String: "Opu" = `0x42 0x6F 0x62`
- **Bytes**: `53 03 42 6F 62`

#### Field 3: dept = "EEE"
- Type flag: `'S'` = `0x53`
- Length: `3` → Varint = `0x03`
- String: "EEE" = `0x43 0x53 0x45`
- **Bytes**: `53 03 43 53 45`

### Complete Record

```
Total data: 49 01 53 03 42 6F 62 53 03 43 53 45
            ^^^^^ ^^^^^^^^^^^^^^^ ^^^^^^^^^^^^^^^
            id=1     name="Opu"      dept="EEE"
            
Record length: 12 bytes
Length prefix: 0x0C (Varint)

Final file bytes: 0C 49 01 53 03 42 6F 62 53 03 43 53 45
                  ^^  ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
                  │         Record data (12 bytes)
                  └─ Length prefix
```

---

## 🧮 Bit-Level Example: Encoding 16,384

### Input: `16,384` (decimal)

**Binary representation**:
```
0000 0000 0000 0000 0100 0000 0000 0000
                    └──────────────────┘
                    Only bit 14 is set
```

### Step 1: Extract 7-bit groups (right to left)

```
Bit positions: ...14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
Binary:        ... 1  0  0  0  0  0  0  0  0  0  0  0  0  0  0

Group into 7-bit chunks:
                ┌─────────┐┌─────────┐┌─────────┐
                │0 0 0 0 0││0 0 0 0 0││0 0 0 0 0│
                │  0  1   ││  0  0   ││  0  0   │
                └─────────┘└─────────┘└─────────┘
                  Chunk 3    Chunk 2    Chunk 1
```

### Step 2: Add continuation bits

```
Chunk 1 (bits 0-6):   0000000 → 1 0000000 = 0x80 (continuation bit = 1)
Chunk 2 (bits 7-13):  0000000 → 1 0000000 = 0x80 (continuation bit = 1)
Chunk 3 (bit 14):     0000001 → 0 0000001 = 0x01 (continuation bit = 0)
```

### Step 3: Output bytes

```
Varint bytes: 0x80  0x80  0x01
              ^^^^  ^^^^  ^^^^
              Byte1 Byte2 Byte3
```

### Verification (Decoding):

```
1. Read 0x80: 
   Data bits = 0000000, shift = 0
   value = 0
   
2. Read 0x80:
   Data bits = 0000000, shift = 7
   value = 0 | (0 << 7) = 0
   
3. Read 0x01:
   Data bits = 0000001, shift = 14
   value = 0 | (1 << 14) = 16,384 ✓
```

---

## 💡 Key Insights

### Advantages
1. **Dynamic sizing**: Adapts to data magnitude
2. **No schema needed**: Self-describing format
3. **Fast decode**: Simple bitwise operations
4. **Industry standard**: Compatible with Protocol Buffers

### Trade-offs
1. **Overhead for large numbers**: 10 bytes for max uint64_t vs 8 bytes fixed
2. **CPU cost**: Bit manipulation vs direct memory copy
3. **Not byte-aligned**: Can't directly cast to integer types

### When to Use Varint
✅ **Good for**:
- IDs, counters, timestamps (often small)
- String lengths (usually < 128)
- File sizes, record counts

❌ **Avoid for**:
- Uniformly distributed random data
- Cryptographic values
- Very large numbers (> 2^56)

---

## 📚 Further Reading

- **Protocol Buffers Encoding**: https://protobuf.dev/programming-guides/encoding/
- **SQLite Varint Format**: https://www.sqlite.org/src4/doc/trunk/www/varint.wiki
- **LEB128 (similar)**: https://en.wikipedia.org/wiki/LEB128

---

**Varint: Small is beautiful! 🎯**
