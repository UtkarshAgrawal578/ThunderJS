# JS Interpreter — Thunder Hackathon 2

A JavaScript interpreter built **from scratch in C++17** — no Node.js, no V8, no external parser or tokenizer libraries. It implements a complete pipeline:

```
Source Code → Lexer (Tokenizer) → Parser (AST) → Interpreter (Evaluator) → Output
```

---

## Project Structure

```
js-interpreter/
├── src/
│   ├── main.cpp          # Entry point
│   ├── lexer.h           # Tokenizer: source code → token stream
│   ├── ast.h             # AST node definitions
│   ├── parser.h          # Recursive-descent parser: tokens → AST
│   ├── value.h           # Runtime value types + helper functions
│   └── interpreter.h     # Tree-walk evaluator + all built-in globals
├── tests/
│   ├── test1.js          # Odd / Even Checker
│   ├── test2.js          # Triangle Pattern (nested for loops)
│   ├── test3.js          # Armstrong Number Check
│   ├── test4.js          # Array Reverse with spread operator
│   └── test5.js          # String Palindrome Check
├── Makefile              # Build + test (Linux/Mac)
├── CMakeLists.txt        # CMake build (alternative)
└── README.md
```

---

## Requirements

| Tool | Minimum Version |
|------|----------------|
| g++ | 7.0+ |
| C++ Standard | C++17 |
| make | any version (Linux/Mac only) |

> No external libraries required. Pure standard C++17.

---

## How to Build & Run

### On Linux / Mac

```bash
# 1. Clone the repository
git clone https://github.com/<your-username>/js-interpreter.git
cd js-interpreter

# 2. Build
make

# 3. Run a specific test
./jsrun tests/test1.js

# 4. Run ALL 5 tests at once
make test

# 5. Run any custom JS file
./jsrun myfile.js
```

---

### On Windows (PowerShell / VS Code Terminal)

```powershell
# 1. Clone the repository
git clone https://github.com/<your-username>/js-interpreter.git
cd js-interpreter

# 2. Build
g++ -std=c++17 -O2 -Isrc -o jsrun src/main.cpp

# 3. Run a specific test
.\jsrun.exe tests\test1.js

# 4. Run ALL 5 tests
.\jsrun.exe tests\test1.js
.\jsrun.exe tests\test2.js
.\jsrun.exe tests\test3.js
.\jsrun.exe tests\test4.js
.\jsrun.exe tests\test5.js

# 5. Run any custom JS file
.\jsrun.exe myfile.js
```

> **Note for Windows users:** Always use `.\jsrun.exe` (not just `jsrun.exe`) in PowerShell.

---

## Test Cases & Expected Output

### Test 1 — Odd / Even Checker
**Input (`tests/test1.js`):**
```js
let num = 7;
if (num % 2 === 0) {
    console.log(num + " is Even");
} else {
    console.log(num + " is Odd");
}
```
**Expected Output:**
```
7 is Odd
```

---

### Test 2 — Triangle Pattern using For Loop
**Input (`tests/test2.js`):**
```js
for (let i = 1; i <= 5; i++) {
    let row = "";
    for (let j = 1; j <= i; j++) {
        row += "* ";
    }
    console.log(row);
}
```
**Expected Output:**
```
* 
* * 
* * * 
* * * * 
* * * * * 
```

---

### Test 3 — Armstrong Number
**Input (`tests/test3.js`):**
```js
function isArmstrong(num) {
    let temp = num; let sum = 0;
    while (temp > 0) {
        let digit = temp % 10;
        sum += digit ** 3;
        temp = Math.floor(temp / 10);
    }
    return sum === num;
}
console.log(isArmstrong(153));
console.log(isArmstrong(123));
```
**Expected Output:**
```
true
false
```

---

### Test 4 — Array Reverse
**Input (`tests/test4.js`):**
```js
let arr = [1, 2, 3, 4, 5];
let reversed = [...arr].reverse();
console.log("Original: " + arr.join(", "));
console.log("Reversed: " + reversed.join(", "));
```
**Expected Output:**
```
Original: 1, 2, 3, 4, 5
Reversed: 5, 4, 3, 2, 1
```

---

### Test 5 — String Palindrome Check
**Input (`tests/test5.js`):**
```js
let str = "racecar";
let reversed = str.split("").reverse().join("");
if (str === reversed) {
    console.log(str + " is a Palindrome");
} else {
    console.log(str + " is not a Palindrome");
}
```
**Expected Output:**
```
racecar is a Palindrome
```

---

## Supported JavaScript Features

| Feature | Status |
|---------|--------|
| `let`, `const`, `var` | ✅ |
| `number`, `string`, `boolean`, `null`, `undefined` | ✅ |
| `object`, `array`, `function` | ✅ |
| Arithmetic, comparison, logical, assignment operators | ✅ |
| `if / else if / else` | ✅ |
| `switch / case / default` | ✅ |
| `for`, `while`, `do...while` | ✅ |
| `for...of`, `for...in` | ✅ |
| Functions (declaration, expression, arrow) | ✅ |
| Closures & default parameters | ✅ |
| Spread `...` and rest `...` operators | ✅ |
| Array methods: `push`, `pop`, `shift`, `unshift`, `slice`, `splice`, `concat`, `includes`, `indexOf`, `sort`, `reverse`, `map`, `filter`, `reduce`, `find`, `some`, `every`, `flat`, `flatMap`, `forEach`, `fill`, `join` | ✅ |
| String methods: `split`, `replace`, `replaceAll`, `substring`, `slice`, `trim`, `toUpperCase`, `toLowerCase`, `includes`, `startsWith`, `endsWith`, `indexOf`, `charAt`, `repeat`, `padStart`, `padEnd` | ✅ |
| Object literals, destructuring, spread | ✅ |
| `Math` object (floor, ceil, round, abs, sqrt, pow, max, min, random, log, sin, cos, tan) | ✅ |
| `class`, `constructor`, methods, `new` | ✅ |
| `try / catch / finally`, `throw` | ✅ |
| `typeof`, `instanceof` | ✅ |
| `parseInt`, `parseFloat`, `Number()`, `String()`, `Boolean()` | ✅ |
| `console.log`, `console.error` | ✅ |
| `JSON.stringify` | ✅ |
| `Date` object | ✅ |

---

## Architecture

```
┌─────────────────────────────────────────────────┐
│                   main.cpp                      │
│         reads file / stdin input                │
└────────────────────┬────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────┐
│                  lexer.h                        │
│   Converts raw JS source into a Token stream    │
│   Handles: keywords, operators, strings,        │
│   numbers, identifiers, comments                │
└────────────────────┬────────────────────────────┘
                     │  vector<Token>
                     ▼
┌─────────────────────────────────────────────────┐
│                  parser.h                       │
│   Recursive-descent parser builds an AST        │
│   Handles: expressions, statements, precedence, │
│   destructuring, arrow functions, classes       │
└────────────────────┬────────────────────────────┘
                     │  NodePtr (AST tree)
                     ▼
┌─────────────────────────────────────────────────┐
│               interpreter.h                     │
│   Tree-walk evaluator with Environment chain    │
│   for scoping. C++ exceptions used for          │
│   return / break / continue / throw signals.    │
│   All built-ins implemented as native lambdas.  │
└────────────────────┬────────────────────────────┘
                     │
                     ▼
                  stdout
```

---

## Constraints

- No V8, Node.js, or any JS engine wrapper used
- No external parser or tokenizer libraries
- Pure hand-written C++17

---

*Built for Thunder Hackathon 2 — JS Interpreter Challenge*