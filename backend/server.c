#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include "fs.h"

#define PORT 8080
#define BUFFER_SIZE 4096
#define BODY_SIZE 10000

Node *root;
Node *current;

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

// takes value of name=... from query string
void getNameParam(char *query, char *name) {
    char raw[100] = "";
    name[0] = '\0';

    char *start = strstr(query, "name=");
    if (start == NULL) {
        return;
    }
    start += 5;

    int i = 0;
    while (start[i] != '\0' && start[i] != '&' && i < 99) {
        raw[i] = start[i];
        i++;
    }
    raw[i] = '\0';
    urlDecode(raw, name);
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
        current = folder;
        sprintf(msg, "Opened %s", name);
    }
}

void goUp(char *msg, int *ok) {
    if (current->parent == NULL) {
        strcpy(msg, "Already at root");
        *ok = 0;
    } else {
        current = current->parent;
        strcpy(msg, "Moved up");
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
    char name[100];
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

    getNameParam(query, name);

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
