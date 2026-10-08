#ifndef FS_H
#define FS_H

#define NAME_SIZE 50
#define PATH_SIZE 500

// one file or folder in our virtual file system
typedef struct Node {
    char name[NAME_SIZE];
    int isFolder;
    struct Node *parent;
    struct Node *firstChild;
    struct Node *nextSibling;
} Node;

Node *createNode(char *name, int isFolder);
int addChild(Node *folder, Node *child);
Node *findChild(Node *folder, char *name);
int countChildren(Node *folder);
void getPath(Node *node, char *path);
int isValidName(char *name);
void freeSubtree(Node *node);
int deleteChild(Node *folder, char *name);
int renameChild(Node *folder, char *oldName, char *newName);

#endif
