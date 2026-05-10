# Hash_KB_RLE_Fano

C++ lab covering four classical data structures and encoding algorithms, all implemented from scratch in a single file (`main.cpp`). Applied to a word-frequency dictionary backed by either a hash table or a red-black tree, with RLE and Shannon-Fano compression on Russian UTF-8 text.

**KB** stands for **Красно-Чёрное дерево** (Red-Black Tree).

---

## 1. Hash Table (`DictionaryWithHashTable`)

### `HashTable`

| Property | Value |
|----------|-------|
| Collision resolution | Separate chaining (`vector<list<HashNode>>`) |
| Hash function | Polynomial: `hash = Σ byte[i] × 31^i mod size` |
| Load factor threshold | 0.75 |
| Rehash strategy | `new_size = old_size × 2 + 1`, full rehash |
| Initial size | 101 |

Operations: `add`, `get`, `remove`, `clear`, `print`, `visualize` (prints all buckets with chains).

### `Dictionary` (hash-backed)

Wraps `HashTable` as a word-frequency counter. Words are normalized to lowercase via manual UTF-8 byte-level Cyrillic/Latin mapping before insertion. Supports `addWord`, `removeWord`, `findWord`, `loadFromFile`.

---

## 2. Red-Black Tree (`DictionaryWithRBTree`)

### `RBTree`

Full implementation with a sentinel `NIL` node (avoids null checks throughout).

**Rotations:** `leftRotate`, `rightRotate`.

**Insertion fixup** — 3 cases per side (symmetric):
1. Uncle is red → recolor, move problem up
2. Uncle is black, node is inner child → rotate parent
3. Uncle is black, node is outer child → recolor + rotate grandparent

**Deletion fixup** — 4 cases per side (symmetric):
1. Sibling is red → rotate parent
2. Both sibling's children black → recolor sibling
3. Sibling's far child black → rotate sibling
4. Sibling's far child red → recolor + rotate parent

**Visualization:** sideways tree dump with R/B labels per node, plus level-by-level printing.

Operations: `insert`, `search`, `remove`, `clear`, `print` (in-order), `visualize`.

### `Dictionary` (RBT-backed)

Same interface as the hash-backed dictionary; uses RB tree for O(log n) operations and sorted in-order traversal.

---

## 3. RLE Compression (`RLE`)

### Text generation

`generateRandomText()` produces Russian UTF-8 text using a **Champernowne distribution**:

```
PDF(x) = 1.57 / (π × cosh(1.57 × (x − 4.0)))
```

Characters sampled from: 30 Russian lowercase letters + 10 digits + 7 punctuation symbols. A 20% chance of generating a run of 3–10 identical characters is applied to create compressible data.

### Encoding format (`advancedRleEncode`)

| Sequence type | Format |
|---------------|--------|
| Run of ≥ 3 identical chars | `N#C` (e.g. `5#а`) |
| Literal sequence (< 3 repeats) | `-N#<raw_bytes>` (e.g. `-4#тест`) |

Separator `#` separates the count from the payload.

`advancedRleDecode` reverses the above, with full error checking (missing separator, out-of-range reads).

---

## 4. Shannon-Fano Coding (`Fano`)

### `buildFanoCodes(text)`

1. Count character frequencies in the input
2. Convert to probabilities, sort descending
3. Recursively split the sorted list at the point that minimizes probability imbalance between the two halves
4. Left half gets prefix `0`, right half gets `1`

### Encoding / Decoding

- `encodeFano(text, codes)` → bit string
- `decodeFano(bit_string, codes)` → original text (reverse lookup via `map<string,char>`)

---

## Common utilities

| Function | Description |
|----------|-------------|
| `readFileToString(path)` | Read file as binary into `std::string` |
| `processTextToWords(text)` | Tokenize by whitespace and punctuation |
| `normalizeWordToLower(s)` | Manual UTF-8 lowercase: Latin `A-Z` + Cyrillic `А-Я` + `Ё` |

---

## Files

| File | Description |
|------|-------------|
| `main.cpp` | All four implementations + menu |
| `sample_text_rus.txt` | Sample Russian input text |
| `Hash_KB_RLE_Fano-4.pdf` | Full lab report |

## Build

```bash
g++ -std=c++17 -O2 main.cpp -o main
./main
```
