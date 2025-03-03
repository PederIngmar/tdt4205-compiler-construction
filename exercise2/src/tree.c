#include "vslc.h"

// Global root for abstract syntax tree
node_t* root;

// Declarations of internal functions, defined further down
static void node_print(node_t* node, int nesting);
static void destroy_subtree(node_t* discard);

// Outputs the entire syntax tree to the terminal
void print_syntax_tree(void)
{
  // If the environment variable GRAPHVIZ_OUTPUT is set, print a GraphViz graph in the dot format
  if (getenv("GRAPHVIZ_OUTPUT") != NULL)
    graphviz_node_print(root);
  else
    node_print(root, 0);
}

// Frees all memory held by the syntax tree
void destroy_syntax_tree(void)
{
  destroy_subtree(root);
  root = NULL;
}

// Initialize a node with the given type and children
node_t* node_create(node_type_t type, size_t n_children, ...)
{
  // Allocate memory for the node
  node_t* new_node = malloc(sizeof(node_t));
  node_t** children = malloc(n_children * sizeof(node_t*));

  // Initialize the node
  new_node->type = type;
  new_node->children = children;
  new_node->n_children = n_children;

  va_list child_list;
  va_start(child_list, n_children);

  // Copy the children from the variable arguments
  for (size_t i = 0; i < n_children; i++)
    children[i] = va_arg(child_list, node_t*);

  va_end(child_list);

  return new_node;
}

// Append an element to the given LIST node, returns the list node
node_t* append_to_list_node(node_t* list_node, node_t* element)
{
  assert(list_node->type == LIST);

  // Calculate the minimum size of the new allocation
  size_t min_allocation_size = list_node->n_children + 1;

  // Round up to the next power of two
  size_t new_allocation_size = 1;
  while (new_allocation_size < min_allocation_size)
    new_allocation_size *= 2;

  // Resize the allocation
  list_node->children = realloc(list_node->children, new_allocation_size * sizeof(node_t*));

  // Insert the new element and increase child count by 1
  list_node->children[list_node->n_children] = element;
  list_node->n_children++;

  return list_node;
}

// Prints out the given node and all its children recursively
static void node_print(node_t* node, int nesting)
{
  // Indent the line based on how deep the node is in the syntax tree
  printf("%*s", nesting, "");

  if (node == NULL)
  {
    printf("(NULL)\n");
    return;
  }

  printf("%s", NODE_TYPE_NAMES[node->type]);

  // For nodes with extra data, include it in the printout
  switch (node->type)
  {
  case OPERATOR:
    printf(" (%s)", node->data.operator);
    break;
  case IDENTIFIER:
    printf(" (%s)", node->data.identifier);
    break;
  case NUMBER_LITERAL:
    printf(" (%ld)", node->data.number_literal);
    break;
  case STRING_LITERAL:
    printf(" (%s)", node->data.string_literal);
    break;
  case STRING_LIST_REFERENCE:
    printf(" (%zu)", node->data.string_list_index);
    break;
  default:
    break;
  }

  putchar('\n');

  // Recursively print children, with some more indentation
  for (size_t i = 0; i < node->n_children; i++)
    node_print(node->children[i], nesting + 1);
}

// Frees the memory owned by the given node, but does not touch its children
static void node_finalize(node_t* discard)
{

  if (discard == NULL)
  return;

  // Only free data if the data field is owned by the node
  switch (discard->type)
  {
  case IDENTIFIER:
    free(discard->data.identifier);
    break;
  case STRING_LITERAL:
    free(discard->data.string_literal);
    break;
  default:
    break;
  }
  free(discard->children);
  free(discard);
}


// Recursively frees the memory owned by the given node, and all its children
static void destroy_subtree(node_t* discard)
{
  if (discard == NULL)
    return;

  for (size_t i = 0; i < discard->n_children; i++)
    destroy_subtree(discard->children[i]);

  node_finalize(discard);
}

// Definition of the global string array NODE_TYPE_NAMES
const char* NODE_TYPE_NAMES[NODE_TYPE_COUNT] = {
#define NODE_TYPE(node_type) #node_type
#include "nodetypes.h"
};
