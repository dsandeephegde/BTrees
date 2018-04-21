#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {  /* B-tree node */
    int n;        /* Number of keys in node */
    int *key;      /* Node's keys */
    long *child;    /* Node's child subtree offsets */
} btree_node;

int order = 4;    /* B-tree order */

btree_node rootNode;  /* Single B-tree node */

btree_node newNode() {
    btree_node node;
    node.n = 0;
    node.key = (int *) calloc(order - 1, sizeof(int));
    node.child = (long *) calloc(order, sizeof(long));
    return node;
}

void addKey(int key);

void writeNode(btree_node node, FILE *fp, long offset);

btree_node readNode(FILE *fp, long offset);

int main() {
    FILE *fp; /* Input file stream */
    long root; /* Offset of B-tree root node */

    fp = fopen("index.bin", "r+b");

    /* If file doesn't exist, set root offset to unknown and create * file, otherwise read the root offset at the front of the file */
    if (fp == NULL) {
        fp = fopen("index.bin", "w+b");
        root = sizeof(long);
        rootNode = newNode();
        fwrite(&root, sizeof(long), 1, fp);
    } else {
        fread(&root, sizeof(long), 1, fp);
        rootNode = readNode(fp, root);
    }

    char command[4096];
    int key;
    gets(command);

    while (strcmp(command, "end") != 0) {
        if (strncmp("add", command, 3) == 0) {
            strtok(command, " ");
            key = atoi(strtok(NULL, " "));
            addKey(key);
            writeNode(rootNode, fp, root);
        } else if (strncmp("find", command, 4) == 0) {
            strtok(command, " ");
            key = atoi(strtok(NULL, " "));
        } else if (strncmp("print", command, 5) == 0) {
            for (int i = 0; i < rootNode.n - 1; ++i) {
                printf( "%d,", rootNode.key[ i ] );
            }
            printf( "%d", rootNode.key[ rootNode.n - 1 ] );
        }
        gets(command);
    }

    return 0;
}

void addKey(int key) {
    rootNode.key[rootNode.n++] = key;
}

void writeNode(btree_node node, FILE *fp, long offset) {
    fseek(fp, offset, SEEK_SET);
    fwrite(&node.n, sizeof(int), 1, fp);
    fwrite(node.key, sizeof(int), order - 1, fp);
    fwrite(node.child, sizeof(long), order, fp);
}

btree_node readNode(FILE *fp, long offset) {
    btree_node result = newNode();
    fseek(fp, offset, SEEK_SET);
    fread(&result.n, sizeof(int), 1, fp);
    fread(result.key, sizeof(int), order-1, fp);
    fread(result.child, sizeof(long), order, fp);
    return result;
}
