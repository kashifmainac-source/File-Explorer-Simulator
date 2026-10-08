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

// returns negative if a should come before b
// folders come first, then names in alphabetical order (ignoring case)
int compareNodes(Node *a, Node *b) {
    if (a->isFolder != b->isFolder) {
        return b->isFolder - a->isFolder;
    }
    int i = 0;
    while (a->name[i] != '\0' && b->name[i] != '\0') {
        int x = tolower((unsigned char)a->name[i]);
        int y = tolower((unsigned char)b->name[i]);
        if (x != y) {
            return x - y;
        }
        i++;
    }
    return (unsigned char)a->name[i] - (unsigned char)b->name[i];
}

// puts child at the right place in the folder's children list
// returns 0 if same name already exists
int addChild(Node *folder, Node *child) {
    if (findChild(folder, child->name) != NULL) {
        return 0;
    }

    child->parent = folder;

    if (folder->firstChild == NULL || compareNodes(child, folder->firstChild) < 0) {
        child->nextSibling = folder->firstChild;
        folder->firstChild = child;
        return 1;
    }

    Node *temp = folder->firstChild;
    while (temp->nextSibling != NULL && compareNodes(temp->nextSibling, child) < 0) {
        temp = temp->nextSibling;
    }
    child->nextSibling = temp->nextSibling;
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

// frees the node and everything inside it
void freeSubtree(Node *node) {
    Node *child = node->firstChild;
    while (child != NULL) {
        Node *next = child->nextSibling;
        freeSubtree(child);
        child = next;
    }
    free(node);
}

// takes the node out of the folder's list without freeing it
Node *unlinkChild(Node *folder, char *name) {
    Node *prev = NULL;
    Node *temp = folder->firstChild;
    while (temp != NULL && strcmp(temp->name, name) != 0) {
        prev = temp;
        temp = temp->nextSibling;
    }
    if (temp == NULL) {
        return NULL;
    }
    if (prev == NULL) {
        folder->firstChild = temp->nextSibling;
    } else {
        prev->nextSibling = temp->nextSibling;
    }
    temp->nextSibling = NULL;
    return temp;
}

// returns 0 if there is no such item
int deleteChild(Node *folder, char *name) {
    Node *node = unlinkChild(folder, name);
    if (node == NULL) {
        return 0;
    }
    freeSubtree(node);
    return 1;
}

// returns 1 on success, 0 if not found, -1 if new name is invalid, -2 if new name is taken
int renameChild(Node *folder, char *oldName, char *newName) {
    Node *node = findChild(folder, oldName);
    if (node == NULL) {
        return 0;
    }
    if (!isValidName(newName)) {
        return -1;
    }
    if (strcmp(oldName, newName) == 0) {
        return 1;
    }
    if (findChild(folder, newName) != NULL) {
        return -2;
    }

    // take it out, change the name and put it back so the order stays correct
    unlinkChild(folder, oldName);
    strncpy(node->name, newName, NAME_SIZE - 1);
    node->name[NAME_SIZE - 1] = '\0';
    addChild(folder, node);
    return 1;
}
