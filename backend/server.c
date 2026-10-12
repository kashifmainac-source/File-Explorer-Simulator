#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include "fs.h"

#define PORT 8080
#define BUFFER_SIZE 4096
#define BODY_SIZE 10000
#define TREE_SIZE 20000
#define STACK_SIZE 50

Node *root;
Node *current;

// back and forward history, each is a stack of folders
Node *backStack[STACK_SIZE];
int backTop = 0;
Node *forwardStack[STACK_SIZE];
int forwardTop = 0;

void push(Node **stack, int *top, Node *folder) {
    if (*top == STACK_SIZE) {
        // stack full, throw away the oldest entry
        for (int i = 1; i < STACK_SIZE; i++) {
            stack[i - 1] = stack[i];
        }
        (*top)--;
    }
    stack[*top] = folder;
    (*top)++;
}

Node *pop(Node **stack, int *top) {
    if (*top == 0) {
        return NULL;
    }
    (*top)--;
    return stack[*top];
}

void setupDefaultFolders() {
    root = createNode("root", 1);
    addChild(root, createNode("Documents", 1));
    addChild(root, createNode("Pictures", 1));
    addChild(root, createNode("Music", 1));
    addChild(root, createNode("readme.txt", 0));

    Node *docs = findChild(root, "Documents");
    addChild(docs, createNode("notes.txt", 0));
    addChild(docs, createNode("College", 1));

    current = root;
}

int hexValue(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

// browser sends "my%20file" for "my file", so we convert it back
void urlDecode(char *src, char *dest) {
    int i = 0, j = 0;
    while (src[i] != '\0') {
        if (src[i] == '%' && hexValue(src[i + 1]) != -1 && hexValue(src[i + 2]) != -1) {
            dest[j] = hexValue(src[i + 1]) * 16 + hexValue(src[i + 2]);
            i += 3;
        } else if (src[i] == '+') {
            dest[j] = ' ';
            i++;
        } else {
            dest[j] = src[i];
            i++;
        }
        j++;
    }
    dest[j] = '\0';
}

// takes value of key=... from query string, for example key "name" or "newname"
void getParam(char *query, char *key, char *value) {
    char raw[100] = "";
    char search[30];
    value[0] = '\0';

    sprintf(search, "%s=", key);
    int keyLen = (int)strlen(search);

    // key must be at the start or right after an &, so "name" does not match "newname"
    char *start = query;
    while (start != NULL) {
        if (strncmp(start, search, keyLen) == 0) {
            break;
        }
        start = strchr(start, '&');
        if (start != NULL) {
            start++;
        }
    }
    if (start == NULL) {
        return;
    }
    start += keyLen;

    int i = 0;
    while (start[i] != '\0' && start[i] != '&' && i < 99) {
        raw[i] = start[i];
        i++;
    }
    raw[i] = '\0';
    urlDecode(raw, value);
}

// adds the folders under this node to out as nested json, files are not shown in the tree
void buildTree(Node *node, char *out) {
    char part[100];
    sprintf(part, "{\"name\":\"%s\",\"children\":[", node->name);
    strcat(out, part);

    Node *temp = node->firstChild;
    int first = 1;
    while (temp != NULL) {
        // keep some space free so the brackets can still be closed
        if (temp->isFolder && strlen(out) + 200 < TREE_SIZE) {
            if (!first) {
                strcat(out, ",");
            }
            first = 0;
            buildTree(temp, out);
        }
        temp = temp->nextSibling;
    }
    strcat(out, "]}");
}

// makes the JSON that frontend reads
void buildResponse(int ok, char *msg, char *body) {
    char path[PATH_SIZE];
    char item[150];

    getPath(current, path);
    sprintf(body, "{\"ok\":%s,\"msg\":\"%s\",\"path\":\"%s\",\"items\":[",
            ok ? "true" : "false", msg, path);

    Node *temp = current->firstChild;
    while (temp != NULL) {
        sprintf(item, "{\"name\":\"%s\",\"type\":\"%s\"}",
                temp->name, temp->isFolder ? "folder" : "file");

        if (strlen(body) + strlen(item) + 5 > BODY_SIZE) {
            break;
        }
        strcat(body, item);
        if (temp->nextSibling != NULL) {
            strcat(body, ",");
        }
        temp = temp->nextSibling;
    }
    strcat(body, "]}");
}

void makeItem(char *name, int isFolder, char *msg, int *ok) {
    if (!isValidName(name)) {
        strcpy(msg, "Invalid name");
        *ok = 0;
        return;
    }
    Node *newNode = createNode(name, isFolder);
    if (newNode == NULL) {
        strcpy(msg, "Out of memory");
        *ok = 0;
        return;
    }
    if (!addChild(current, newNode)) {
        free(newNode);
        sprintf(msg, "%s already exists", name);
        *ok = 0;
        return;
    }
    sprintf(msg, "%s created", name);
}

void openFolder(char *name, char *msg, int *ok) {
    Node *folder = findChild(current, name);
    if (folder == NULL) {
        strcpy(msg, "Not found");
        *ok = 0;
    } else if (!folder->isFolder) {
        strcpy(msg, "That is a file, not a folder");
        *ok = 0;
    } else {
        push(backStack, &backTop, current);
        forwardTop = 0;
        current = folder;
        sprintf(msg, "Opened %s", name);
    }
}

void goUp(char *msg, int *ok) {
    if (current->parent == NULL) {
        strcpy(msg, "Already at root");
        *ok = 0;
    } else {
        push(backStack, &backTop, current);
        forwardTop = 0;
        current = current->parent;
        strcpy(msg, "Moved up");
    }
}

void goBack(char *msg, int *ok) {
    Node *folder = pop(backStack, &backTop);
    if (folder == NULL) {
        strcpy(msg, "Nothing to go back to");
        *ok = 0;
        return;
    }
    push(forwardStack, &forwardTop, current);
    current = folder;
    strcpy(msg, "Went back");
}

void goForward(char *msg, int *ok) {
    Node *folder = pop(forwardStack, &forwardTop);
    if (folder == NULL) {
        strcpy(msg, "Nothing to go forward to");
        *ok = 0;
        return;
    }
    push(backStack, &backTop, current);
    current = folder;
    strcpy(msg, "Went forward");
}

void deleteItem(char *name, char *msg, int *ok) {
    if (!deleteChild(current, name)) {
        strcpy(msg, "Not found");
        *ok = 0;
        return;
    }
    // history may point to folders that are now freed, so clear it
    backTop = 0;
    forwardTop = 0;
    sprintf(msg, "%s deleted", name);
}

void renameItem(char *name, char *newName, char *msg, int *ok) {
    int result = renameChild(current, name, newName);
    if (result == 1) {
        sprintf(msg, "Renamed to %s", newName);
        return;
    }
    *ok = 0;
    if (result == 0) {
        strcpy(msg, "Not found");
    } else if (result == -1) {
        strcpy(msg, "Invalid name");
    } else {
        sprintf(msg, "%s already exists", newName);
    }
}

void sendResponse(SOCKET client, char *status, char *body) {
    char header[300];
    sprintf(header,
            "HTTP/1.1 %s\r\n"
            "Content-Type: application/json\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Content-Length: %d\r\n"
            "Connection: close\r\n\r\n",
            status, (int)strlen(body));
    send(client, header, (int)strlen(header), 0);
    send(client, body, (int)strlen(body), 0);
}

void handleRequest(SOCKET client, char *request) {
    char method[10], url[300];
    char route[100], query[200] = "";
    char name[100], newName[100];
    char msg[150] = "OK";
    char body[BODY_SIZE];
    int ok = 1;

    if (sscanf(request, "%9s %299s", method, url) != 2) {
        return;
    }
    printf("%s %s\n", method, url);

    // split /mkdir?name=abc into route "/mkdir" and query "name=abc"
    char *q = strchr(url, '?');
    if (q != NULL) {
        *q = '\0';
        strncpy(query, q + 1, 199);
        query[199] = '\0';
    }
    strncpy(route, url, 99);
    route[99] = '\0';

    getParam(query, "name", name);
    getParam(query, "newname", newName);

    if (strcmp(route, "/list") == 0) {
        // nothing to do, just send current folder
    } else if (strcmp(route, "/mkdir") == 0) {
        makeItem(name, 1, msg, &ok);
    } else if (strcmp(route, "/touch") == 0) {
        makeItem(name, 0, msg, &ok);
    } else if (strcmp(route, "/open") == 0) {
        openFolder(name, msg, &ok);
    } else if (strcmp(route, "/up") == 0) {
        goUp(msg, &ok);
    } else if (strcmp(route, "/back") == 0) {
        goBack(msg, &ok);
    } else if (strcmp(route, "/forward") == 0) {
        goForward(msg, &ok);
    } else if (strcmp(route, "/delete") == 0) {
        deleteItem(name, msg, &ok);
    } else if (strcmp(route, "/rename") == 0) {
        renameItem(name, newName, msg, &ok);
    } else {
        sendResponse(client, "404 Not Found", "{\"ok\":false,\"msg\":\"Unknown route\"}");
        return;
    }

    buildResponse(ok, msg, body);
    sendResponse(client, "200 OK", body);
}

int main() {
    WSADATA wsa;
    SOCKET server, client;
    struct sockaddr_in addr;
    char request[BUFFER_SIZE];

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("WSAStartup failed\n");
        return 1;
    }

    server = socket(AF_INET, SOCK_STREAM, 0);
    if (server == INVALID_SOCKET) {
        printf("Could not create socket\n");
        return 1;
    }

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(PORT);

    if (bind(server, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR) {
        printf("Bind failed. Is port %d already in use?\n", PORT);
        return 1;
    }
    listen(server, 5);

    setupDefaultFolders();
    printf("Server running at http://localhost:%d\n", PORT);
    printf("Open frontend/index.html in your browser\n\n");

    while (1) {
        client = accept(server, NULL, NULL);
        if (client == INVALID_SOCKET) {
            continue;
        }
        int len = recv(client, request, BUFFER_SIZE - 1, 0);
        if (len > 0) {
            request[len] = '\0';
            handleRequest(client, request);
        }
        closesocket(client);
    }

    closesocket(server);
    WSACleanup();
    return 0;
}
