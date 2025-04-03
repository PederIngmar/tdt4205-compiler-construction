#include <vslc.h>

// Declaration of global symbol table
symbol_table_t* global_symbols;

// Declarations of helper functions defined further down in this file
static void find_globals(void);
static void bind_names(symbol_table_t* local_symbols, node_t* root);
static void print_symbol_table(symbol_table_t* table, int nesting);
static void destroy_symbol_tables(void);

static size_t add_string(char* string);
static void print_string_list(void);
static void destroy_string_list(void);

/* External interface */

// Creates a global symbol table, and local symbol tables for each function.
// All usages of symbols are bound to their symbol table entries.
// All strings are entered into the string_list
void create_tables(void)
{
  find_globals();

  for (size_t i = 0; i < global_symbols->n_symbols; i++) {
    symbol_t* symbol = global_symbols->symbols[i];
    if (symbol->type == SYMBOL_FUNCTION) {
      bind_names(symbol->function_symtable, symbol->node->children[2]);
    }
  }
  
  // TODO:
  // First use find_globals() to create the global symbol table.
  // As global symbols are added, function symbols get their own local symbol tables as well.
  //
  // Once all global symbols are added, go through all functions bodies and bind references.
  //
  // Binding should performed by bind_names(function symbol table, function body AST node).
  // IDENTIFIERs that reference declarations should point to the symbol_t they reference.
  // It handles pushing and popping scopes, and adding variables to the local symbol table.
  // A final task performed by bind_names(...), is adding strings to the global string list.
}

// Prints the global symbol table, and the local symbol tables for each function.
// Also prints the global string list.
// Finally prints out the AST again, with bound symbols.
void print_tables(void)
{
  print_symbol_table(global_symbols, 0);
  printf("\n == STRING LIST == \n");
  print_string_list();
  printf("\n == BOUND SYNTAX TREE == \n");
  print_syntax_tree();
}

// Cleans up all memory owned by symbol tables and the global string list
void destroy_tables(void)
{
  destroy_symbol_tables();
  destroy_string_list();
}

/* Internal matters */

// Goes through all global declarations, adding them to the global symbol table.
// When adding functions, a local symbol table with symbols for its parameters are created.
static void find_globals(void)
{
  global_symbols = symbol_table_init();

  for (size_t i = 0; i < root->n_children; i++) {
    node_t* child = root->children[i];
    
    if (child->type == FUNCTION) {
      symbol_t* symbol = malloc(sizeof(symbol_t));
      symbol->name = child->children[0]->data.identifier;
      symbol->type = SYMBOL_FUNCTION;
      symbol->node = child;
      symbol->function_symtable = symbol_table_init();
      symbol_table_insert(global_symbols, symbol);

      symbol_table_t* local_symbols = symbol->function_symtable;
      node_t* parameters = child->children[1];
      for (size_t j = 0; j < parameters->n_children; j++) {
        node_t* parameter = parameters->children[j];
        symbol_t* parameter_symbol = malloc(sizeof(symbol_t));
        parameter_symbol->name = parameter->data.identifier;
        parameter_symbol->type = SYMBOL_PARAMETER;
        parameter_symbol->node = parameter;
        parameter_symbol->function_symtable = NULL;
        symbol_table_insert(local_symbols, parameter_symbol);
      }

    } else if (child->type == GLOBAL_DECLARATION) {
        node_t* declaration = child->children[0];
        for (size_t j = 0; j < declaration->n_children; j++) {
          node_t* identifier = declaration->children[j];
          symbol_t* symbol = malloc(sizeof(symbol_t));
          if (identifier->type == ARRAY_INDEXING) {
            node_t* array = identifier->children[0];
            symbol->name = array->data.identifier;
            symbol->type = SYMBOL_GLOBAL_ARRAY;
            symbol->node = array;
          } else if (identifier->type == IDENTIFIER) {
              symbol->name = identifier->data.identifier;
              symbol->type = SYMBOL_GLOBAL_VAR;
              symbol->node = identifier;
          }
          symbol->function_symtable = NULL;
          symbol_table_insert(global_symbols, symbol);
        }
    }
  }
  // TODO: Create symbols for all global defintions (global variables, arrays and functions),
  // and add them to the global symbol table. See the symtype_t enum in "symbols.h"

  // When creating a symbol for a function, also create a local symbol_table_t for it.
  // Store this local symbol table in the function symbol's function_symtable field.
  // Any parameters the function may have should be added to this local symbol table.

  // TIP: create symbols using malloc(sizeof(symbol_t)), and assigning the relevant fields.
  // Use symbol_table_insert() (from "symbol_table.h") to insert new symbols into tables.

  // If a symbol already exists with the same name, the insertion will return INSERT_COLLISION.
  // Feel free to print an error message and abort using exit(EXIT_FAILURE),
  // but we will not be testing your compiler on invalid VSL.
}

static void bind_names(symbol_table_t* local_symbols, node_t* node) {
  if (node == NULL) {
    return;
  }

  if (node->type == BLOCK) {
    symbol_hashmap_t* backup = local_symbols->hashmap;
    local_symbols->hashmap = symbol_hashmap_init();
    local_symbols->hashmap->backup = backup;
    
    if (node->n_children == 2) {
      node_t* declaration_list = node->children[0];
      for (size_t i = 0; i < declaration_list->n_children; i++) {
        node_t* declaration = declaration_list->children[i];
        for (size_t j = 0; j < declaration->n_children; j++) {
          node_t* identifier = declaration->children[j];
          symbol_t* symbol = malloc(sizeof(symbol_t));
          symbol->name = identifier->data.identifier;
          symbol->type = SYMBOL_LOCAL_VAR;
          symbol->node = NULL;
          symbol->function_symtable = NULL;
          if (symbol_table_insert(local_symbols, symbol) == INSERT_COLLISION) {
            fprintf(stderr, "Error: Redeclaration of variable '%s'\n", identifier->data.identifier);
            exit(EXIT_FAILURE);
          }
        }
      }
      bind_names(local_symbols, node->children[1]);
    } else {
      bind_names(local_symbols, node->children[0]);
    }
    backup = local_symbols->hashmap->backup;
    symbol_hashmap_destroy(local_symbols->hashmap);
    local_symbols->hashmap = backup;
    return;
  } else if (node->type == IDENTIFIER) {
    symbol_t* symbol = symbol_hashmap_lookup(local_symbols->hashmap, node->data.identifier);
    if (symbol == NULL) {
      symbol = symbol_hashmap_lookup(global_symbols->hashmap, node->data.identifier);
    }
    node->symbol = symbol;
  } else if (node->type == STRING_LITERAL) {
      size_t index = add_string(node->data.string_literal);
      node->data.string_list_index = index;
      node->type = STRING_LIST_REFERENCE;
  }
  for (size_t i = 0; i < node->n_children; i++) {
    bind_names(local_symbols, node->children[i]);
  }
}

// Prints the given symbol table, with sequence number, symbol names and types.
// When printing function symbols, its local symbol table is recursively printed, with indentation.
static void print_symbol_table(symbol_table_t* table, int nesting)
{
  for (size_t i = 0; i < table->n_symbols; i++)
  {
    symbol_t* symbol = table->symbols[i];

    printf(
        "%*s%ld: %s(%s)\n",
        nesting * 4,
        "",
        symbol->sequence_number,
        SYMBOL_TYPE_NAMES[symbol->type],
        symbol->name);

    // If the symbol is a function, print its local symbol table as well
    if (symbol->type == SYMBOL_FUNCTION)
      print_symbol_table(symbol->function_symtable, nesting + 1);
  }
}

// Frees up the memory used by the global symbol table, all local symbol tables, and their symbols
static void destroy_symbol_tables(void)
{
  for (size_t i = 0; i < global_symbols->n_symbols; i++) {
    symbol_t* symbol = global_symbols->symbols[i];
    if (symbol->type == SYMBOL_FUNCTION) {
      symbol_table_destroy(symbol->function_symtable);
    }
  }
  symbol_table_destroy(global_symbols);

  // TODO: Implement cleanup. All symbols in the program are owned by exactly one symbol table.

  // TIP: Using symbol_table_destroy() goes a long way, but it only cleans up the given table.
  // Try cleaning up all local symbol tables before cleaning up the global one.
}

// Declaration of global string list
char** string_list;
size_t string_list_len;
static size_t string_list_capacity;

// Adds the given string to the global string list, resizing if needed.
// Takes ownership of the string, and returns its position in the string list.
static size_t add_string(char* string)
{
  if (string_list_len == string_list_capacity) {
    if (string_list_capacity == 0) {
      string_list_capacity = 1;
    } else {
      string_list_capacity *= 2;
    }
    string_list = realloc(string_list, string_list_capacity * sizeof(char*));
    if (string_list == NULL) {
      perror("Failed to reallocate string list");
      exit(EXIT_FAILURE);
    }
  }

  string_list[string_list_len] = string;
  return string_list_len++;

  // TODO: Write a helper function you can use during bind_names(),
  // to easily add a string into the dynamically growing string_list.

  // The length of the string list should be stored in string_list_len.

  // The variable string_list_capacity should contain the maximum number of char*
  // that can fit in the current string_list before we need to allocate a larger array.
  // If length is about to surpass capacity, create a larger allocation first.
  // Tip: See the realloc function from the standard library

  // Return the position the added string gets in the list.
}

// Prints all strings added to the global string list
static void print_string_list(void)
{
  for (size_t i = 0; i < string_list_len; i++)
    printf("%ld: %s\n", i, string_list[i]);
}

// Frees all strings in the global string list, and the string list itself
static void destroy_string_list(void)
{
  for (size_t i = 0; i < string_list_len; i++) {
    free(string_list[i]);
  }
  free(string_list);

  // TODO: Called during cleanup, free strings, and the memory used by the string list itself
}
