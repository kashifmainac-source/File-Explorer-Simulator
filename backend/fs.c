#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
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

// builds path like root/Documents/Notes by going up to root first
void getPath(Node *node, char *path) {
    if (node->parent == NULL) {
        strcpy(path, node->name);
        return;
    }
    getPath(node->parent, path);
    if (strlen(path) + strlen(node->name) + 2 < PATH_SIZE) {
        strcat(path, "/");
        strcat(path, node->name);
    }
}

// only letters, digits, space, dot, underscore and dash are allowed
int isValidName(char *name) {
    int len = (int)strlen(name);
    if (len == 0 || len >= NAME_SIZE) {
        return 0;
    }
    if (name[0] == ' ' || name[0] == '.') {
        return 0;
    }
    for (int i = 0; i < len; i++) {
        char c = name[i];
        if (!isalnum((unsigned char)c) && c != ' ' && c != '.' && c != '_' && c != '-') {
            return 0;
        }
    }
    return 1;
}
