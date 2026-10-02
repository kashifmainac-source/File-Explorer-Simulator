#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fs.h"

Node *createNode(char *name, int isFolder) {
    Node *newNode = (Node *)malloc(sizeof(Node));
    if (newNode == NULL) {
        printf("Memory allocation failed\n");
        return NULL;
    }
    strncpy(newNode->name, name, NAME_SIZE - 1);
    newNode->name[NAME_SIZE - 1] = '\0';
    newNode->isFolder = isFolder;
    newNode->parent = NULL;
    newNode->firstChild = NULL;
    newNode->nextSibling = NULL;
    return newNode;
}

// adds child at the end of the folder's children list
// returns 0 if same name already exists
int addChild(Node *folder, Node *child) {
    if (findChild(folder, child->name) != NULL) {
        return 0;
    }

    child->parent = folder;

    if (folder->firstChild == NULL) {
        folder->firstChild = child;
        return 1;
    }

    Node *temp = folder->firstChild;
    while (temp->nextSibling != NULL) {
        temp = temp->nextSibling;
    }
    temp->nextSibling = child;
    return 1;
}

Node *findChild(Node *folder, char *name) {
    Node *temp = folder->firstChild;
    while (temp != NULL) {
        if (strcmp(temp->name, name) == 0) {
            return temp;
        }
        temp = temp->nextSibling;
    }
    return NULL;
}

int countChildren(Node *folder) {
    int count = 0;
    Node *temp = folder->firstChild;
    while (temp != NULL) {
        count++;
        temp = temp->nextSibling;
    }
    return count;
}
