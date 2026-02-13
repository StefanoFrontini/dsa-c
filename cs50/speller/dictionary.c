// Implements a dictionary's functionality

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "dictionary.h"

// Represents a node in a hash table
typedef struct Node {
  char word[LENGTH + 1];
  struct Node *next;
} Node;

// TODO: Choose number of buckets in hash table
const unsigned int N = 1125;

// Hash table
Node *table[N];

int words_in_dict = 0;

// Returns true if word is in dictionary, else false
bool check(const char *word) {
  // TODO
  int bucket = hash(word);

  Node *list = table[bucket];
  if (list == NULL) {
    return false;
  } else {

    for (Node *ptr = list; ptr != NULL; ptr = ptr->next) {
      // printf("ptr->word: %s\n", ptr->word);
      // printf("word: %s\n", word);
      if (strcasecmp(ptr->word, word) == 0) {
        return true;
      }
    }
    return false;
  }
}

// Hashes word to a number
unsigned int hash(const char *word)

{
  int i = 0;
  int sum = 0;

  while (word[i] != '\0') {
    sum += word[i] < 65 ? 0 : (toupper(word[i]) - 'A');
    i++;
  }
  // printf("word: %s, sum: %i\n", word, sum);
  return sum;
  // TODO: Improve this hash function
  /* return toupper(word[0]) - 'A'; */
}

// Loads dictionary into memory, returning true if successful, else false
bool load(const char *dictionary) {
  FILE *input = fopen(dictionary, "r");
  if (input == NULL) {
    printf("Could not open %s.\n", dictionary);
    return false;
  }

  // char buffer;
  char new_word[LENGTH + 1];
  // int i = 0;
  words_in_dict = 0;
  while (fscanf(input, "%s", new_word) != EOF) {
    Node *n = malloc(sizeof(Node));

    if (n == NULL) {
      return false;
    }
    n->next = NULL;
    strcpy(n->word, new_word);

    int bucket = hash(n->word);
    // printf("bucket: %i, word: %s\n", bucket, n->word);

    if (table[bucket] == NULL) {
      table[bucket] = n;
    } else {
      n->next = table[bucket];
      table[bucket] = n;
    }
    words_in_dict++;
  }
  /*     while (fread(&buffer, sizeof(char), 1, input) != 0)
      {
          if(buffer != '\n')
          {
             new_word[i] = buffer;
             i++;
          }
          else
          {
              new_word[i] = '\0';

              node *n = malloc(sizeof(node));

              if(n == NULL)
              {
                  return false;
              }
              n->next = NULL;
              strcpy(n->word, new_word);

              int bucket = hash(n->word);

              if(table[bucket] == NULL)
              {
                  table[bucket] = n;
              }
              else
              {
                  n->next = table[bucket];
                  table[bucket] = n;
              }
              words_in_dict++;
              i = 0;

          }

      } */
  fclose(input);
  return true;
}

// Returns number of words in dictionary if loaded, else 0 if not yet loaded
unsigned int size(void) {
  // TODO
  if (words_in_dict > 0) {
    return words_in_dict;
  } else {
    return 0;
  }
}

// Unloads dictionary from memory, returning true if successful, else false
bool unload(void) {

  for (int i = 0; i < N; i++) {

    if (table[i] == NULL) {

      continue;
    } else {

      Node *cursor = table[i];
      while (cursor != NULL) {
        Node *tmp = cursor;
        cursor = cursor->next;
        free(tmp);
      }

      /*             for(node *cursor = table[i]; cursor != NULL;
         cursor=cursor->next)
                  {
                      printf("word: %s, i: %i, next pointer: %p\n",
         cursor->word, i, cursor->next); node *tmp = cursor; free(cursor);
                      cursor = tmp;

                  } */
    }
  }
  return true;
}
