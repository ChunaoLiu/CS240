/* Name, library.c, CS 24000, Fall 2020
 * Last updated October 12, 2020
 */

/* Add any includes here */

#include "library.h"

#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <dirent.h>

#define IMPOSSIBLE             (-100000)
#define LEFT                   (-1)
#define RIGHT                  (1)

tree_node_t *g_song_library = {0};

/* find_parent_pointer will take a double pointer of
 * tree_node_t and a string as parameter, and will search
 * the string name in the given Binary search tree. If found
 * it will return a double pointer of that node. Note this
 * does not return its parent.
 */

tree_node_t **find_parent_pointer(tree_node_t **node,
const char *name) {
  assert(node != NULL);
  assert(*node != NULL);
  if (strcmp((*node)->song_name, name) == 0) {
    return node;
  }
  else if (strcmp(name, (*node)->song_name) > 0) {
    if ((*node)->right_child == NULL) {
      return NULL;
    }
    return find_parent_pointer(&(*node)->right_child, name);
  }
  else {
    if ((*node)->left_child == NULL) {
      return NULL;
    }
    return find_parent_pointer(&(*node)->left_child, name);
  }
} /* find_parent_pointer() */

/* tree_insert() will take a double pointer of the root of the search
 * tree. and a pointer of tree_node_t as the node we want to insert
 * the program will automatically find the best position and insert
 * the given node into the search tree. If the name is dublicated, it
 * will return DUPLICATE_SONG.
 */

int tree_insert(tree_node_t **root, tree_node_t* inserted) {
  assert(root != NULL);
  assert(*root != NULL);
  assert(inserted != NULL);
  tree_node_t *current = *root;
  while (true) {
    if (strcmp((current)->song_name, inserted->song_name) == 0) {
      return DUPLICATE_SONG;
    }
    if (strcmp((current)->song_name, inserted->song_name) > 0) {
      if ((current)->left_child == NULL) {
        (current)->left_child = inserted;
        return INSERT_SUCCESS;
      }
      else {
        current = current->left_child;
      }
    }
    else {
      if (current->right_child == NULL) {
        current->right_child = inserted;
        return INSERT_SUCCESS;
      }
      else {
        current = current->right_child;
      }
    }
  }
  return IMPOSSIBLE;
} /* tree_insert() */

/* remove_song_from_tree() will take a double pointer of the root of
 * the search tree, and a name of the song to be deleted. It will
 * find the song in the tree and remove it properly while re-assign
 * its children's position
 */

int remove_song_from_tree(tree_node_t **root, const char* name){
  assert(root != NULL);
  assert(*root != NULL);
  assert(name != NULL);
  tree_node_t *current = *root;
  tree_node_t *parent = *root;
  int direction = 0;
  while (true) {
    if (strcmp(current->song_name, name) == 0) {
      if (current == parent) {
        tree_node_t *right_smallest = current->right_child;
        tree_node_t *smallest_parent = current;
        while (true) {
          if (right_smallest->left_child != NULL) {
            smallest_parent = right_smallest;
            right_smallest = right_smallest->left_child;
          }
          else {
            break;
          }
        }
        if (current->right_child == right_smallest) {
          right_smallest->left_child = (*root)->left_child;
        }
        else if (right_smallest->right_child != NULL) {
          smallest_parent->left_child = right_smallest->right_child;
          right_smallest->right_child = (*root)->right_child;
          right_smallest->left_child = (*root)->left_child;
        }
        else {
          right_smallest->left_child = (*root)->left_child;
          right_smallest->right_child = (*root)->right_child;
          smallest_parent->left_child = NULL;
        }
        *root = right_smallest;
        free_node(current);
        return DELETE_SUCCESS;
      }
      if (current->left_child != NULL) {
        if (current->right_child != NULL) {

          /* If the deleted node has both side children, then we will
           * move its smallest node on the right child up and let it be
           * the current root. We will also assign its left children to
           * left child of the new node. This can be done 10 times easier
           * with recursion but I'm too dumb to not realize that at the
           * beginning
           */

          tree_node_t *right_smallest = current->right_child;
          tree_node_t *smallest_parent = current;
          while (true) {
            if (right_smallest->left_child != NULL) {
              smallest_parent = right_smallest;
              right_smallest = right_smallest->left_child;
            }
            else {
              break;
            }
          }

          /* we also need to know which way was the original deletion so
           * that we can assign the right child
           */

          if (right_smallest->right_child != NULL) {
            smallest_parent->left_child = right_smallest->right_child;
          }
          else {
            smallest_parent->left_child = NULL;
          }
          right_smallest->left_child = current->left_child;
          if (direction == RIGHT) {
            parent->right_child = right_smallest;
            right_smallest->right_child = current->right_child;
          }
          else {
            parent->left_child = right_smallest;
            right_smallest->right_child = current->right_child;
          }
        }
        else {
          if (direction == RIGHT) {
            parent->right_child = current->left_child;
          }
          else {
            parent->left_child = current->left_child;
          }
        }
      }

      /* I basically listed all cases here because no recursion, and
       * here it is the case when the left child is null while the
       * right child could be null or not. If it is null, just safely
       * delete the node. If not null, we just need to append the right
       * child up and we're good.
       */

      else {
        if (current->right_child != NULL) {
          if (direction == RIGHT) {
            parent->right_child = current->right_child;
          }
          else {
            parent->left_child = current->right_child;
          }
        }
        else {
          if (direction == RIGHT) {
            parent->right_child = NULL;
          }
          else {
            parent->left_child = NULL;
          }
        }
      }
      free_node(current);
      return DELETE_SUCCESS;
    }
    else if (strcmp(name, current->song_name) > 0) {
      if (current->right_child == NULL) {
        return SONG_NOT_FOUND;
      }
      parent = current;
      current = current->right_child;
      direction = RIGHT;
    }
    else {
      if (current->left_child == NULL) {
        return SONG_NOT_FOUND;
      }
      parent = current;
      current = current->left_child;
      direction = LEFT;
    }
  }
} /* remove_song_from_tree() */

/* free_node() will take a tree_node_t pointer
 * and will free all its associated memory
 */

void free_node(tree_node_t *current) {
  assert(current != NULL);
  free_song(current->song);
  current->song = NULL;
  free(current);
  current = NULL;
} /* free_node() */

/* print_node() will take a tree root and a file pointer,
 * and will print the song name into the given file. Simple
 */

void print_node(tree_node_t *root, FILE *file) {
  assert(root != NULL);
  assert(file != NULL);
  fprintf(file, "%s\n" , root->song_name);
} /* print_node() */

/* traverse_pre_order will take a pointer of the root, a
 * void arbitrary data and a function pointer. It will loop
 * the tree in pre_order form and will call the function
 * pointed by the function pointer using the node looped and
 * the arbitrary data.
 */

void traverse_pre_order(tree_node_t *root, void *arbi_data,
void (*traversal_func)(tree_node_t *, void *)) {
  if (root == NULL) {
    return;
  }
  traversal_func(root, arbi_data);
  traverse_pre_order(root->left_child, arbi_data, traversal_func);
  traverse_pre_order(root->right_child, arbi_data, traversal_func);
} /* traverse_pre_order() */

/* traverse_in_order will take a pointer of the root, a
 * void arbitrary data and a function pointer. It will loop
 * the tree in in_order form and will call the function
 * pointed by the function pointer using the node looped and
 * the arbitrary data.
 */

void traverse_in_order(tree_node_t *root, void *arbi_data,
void (*traversal_func)(tree_node_t *, void *)) {
  if (root == NULL) {
    return;
  }
  traverse_in_order(root->left_child, arbi_data, traversal_func);
  traversal_func(root, arbi_data);
  traverse_in_order(root->right_child, arbi_data, traversal_func);
} /* traverse_in_order() */

/* traverse_post_order will take a pointer of the root, a
 * void arbitrary data and a function pointer. It will loop
 * the tree in post_order form and will call the function
 * pointed by the function pointer using the node looped and
 * the arbitrary data.
 */

void traverse_post_order(tree_node_t *root, void *arbi_data,
void (*traversal_func)(tree_node_t *, void *)) {
  if (root == NULL) {
    return;
  }
  traverse_post_order(root->left_child, arbi_data, traversal_func);
  traverse_post_order(root->right_child, arbi_data, traversal_func);
  traversal_func(root, arbi_data);
} /* traverse_post_order() */

/* free_library will take a pointer of the root of a tree and
 * will free the entire tree and any memory associated with it
 */

void free_library(tree_node_t *root) {
  if (root == NULL) {
    return;
  }
  free_library(root->left_child);
  free_library(root->right_child);
  root->song_name = NULL;
  free_node(root);
} /* free_library() */

/* print_node_proxy is a proxy funtion for the recursion function
 * and the print_node function. It will cast the arbitrary data
 * into a file pointer type and call the print_node function so
 * that there will be no conflict.
 */

void print_node_proxy(tree_node_t *root, void *arbi_data) {
  print_node(root, (FILE *)arbi_data);
  return;
} /* print_node_proxy() */

/* write_song_list() will take a file pointer and a root of the tree
 * as parameters, and will traverse in order form to write all
 * song's name in order.
 */

void write_song_list(FILE *fp, tree_node_t *root) {
  traverse_in_order(root, fp, print_node_proxy);
} /* write_song_list() */

/* make_library() will take a string as file directory, and will
 * use dirent to search for all the files under that directory. If
 * it finds a midi file, it will read it into the library using
 * parse_file and insert_tree. If not, it will ignore the invalid
 * file and continue reading
 */

void make_library(const char *direction) {
  printf("%s\n", "Creating Library Now...");
  struct dirent *de = {0};
  DIR *dr = opendir(direction);
  assert(dr != NULL);
  while ((de = readdir(dr)) != NULL) {
    if (strcmp(de->d_name, ".") == 0) {
      continue;
    }
    if (strcmp(de->d_name, "..") == 0) {
      continue;
    }
    char full_path[100] = {0};
    strcat(full_path, direction);
    strcat(full_path, "/");
    strcat(full_path, de->d_name);

    /* We use strrchr to extract the last dot, which
     * usually represents extension, and see if it is
     * a midi file. If not, we will ignore it.
     */

    char *extension = strrchr(full_path, '.');
    if (extension == NULL) {
      continue;
    }
    if (strcmp(extension, ".mid") != 0) {
      continue;
    }
    song_data_t *songs = parse_file(full_path);
    tree_node_t *current = malloc(sizeof(tree_node_t));
    assert(current != NULL);
    current->song = songs;
    int file_pos = 0;

    /* We'll use pointer manipulation to directly extract
     * the name of the file instead of a relative path. It
     * basically makes our song_name points to part of the
     * path in the written song, so we don't need to malloc
     * or free it anymore.
     */

    for (int i = 1; i < strlen(current->song->path); i++) {
      if (current->song->path[i] == '/') {
        if (current->song->path[i + 1] == '\0') {
          break;
        }
        else {
          file_pos = i + 1;
        }
      }
    }
    current->song_name = &(current->song->path[file_pos]);
    printf("%s: %s\n", "Song name is", current->song_name);
    current->left_child = NULL;
    current->right_child = NULL;
    if (g_song_library == NULL) {
      g_song_library = current;
      printf("%s\n", "We got an library!");
    }
    else {
      tree_insert(&g_song_library, current);
    }
  }
  printf("%s\n", "Library Over.");
  closedir(dr);
} /* make_library() */
