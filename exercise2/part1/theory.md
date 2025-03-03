# Theoretical Exercises: Top/down Parsing Tables

### Grammar:

1. S → i C B E F n
2. C → c
3. B → B ; + V
4. B → + V
5. V → x
6. V → y
7. E → e B
8. E → ϵ
9. F → f 
10. F → ϵ

## Task 1

Problem: Non-terminal B uses left recursion

Fix 1: B → ; + V B | + V

1. S → i C B E F n
2. C → c
3. B → + V ; B
4. B → + V
5. V → x
6. V → y
7. E → e B
8. E → ϵ
9. F → f 
10. F → ϵ

Problem 2: Non-terminal B uses common prefix "+"
Fix 2: left factoring

1. S → i C B E F n
2. C → c
3. B → + V B'
4. B' → ; B
5. B' → ϵ 
6. V → x
7. V → y
8. E → e B
9. E → ϵ
10. F → f 
11. F → ϵ
