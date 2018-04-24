#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef struct {    // B-tree node
    int n;          // Number of keys in node
    int *key;       // Node's keys
    long *child;    // Node's child subtree offsets
} btree_node;

int bTreeOrder;     // B-tree bTreeOrder
FILE *inputFile;    // Input file stream
long root;          // Offset of B-tree root node

btree_node newNode() {
    btree_node node;
    node.n = 0;
    node.key = (int *) calloc(bTreeOrder - 1, sizeof(int));
    node.child = (long *) calloc(bTreeOrder, sizeof(long));
    return node;
}

void freeNode(btree_node *node) {
    free(node->key);
    free(node->child);
}

void writeNode(btree_node node, FILE *file, long offset);

btree_node readNode(FILE *file, long offset);

void addKey(int key);

int findKey(int key, long offset);

void printTreeLevel(int level, long *nodesToPrint, int size);

long findNodeWithChild(long offset, long childOffset, int key);

void addKeyAndRightChildTo(long nodeOffset, long rightChildOffset, int key);

void assignToNewRoot(long leftNode, long rightNode, int key);

long getEndOffset(FILE *file);

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Enter Correct number of arguments, Usage: assn_4 index.bin 4");
        exit(0);
    }
    char *inputFileName = argv[1];
    bTreeOrder = atoi(argv[2]);

    inputFile = fopen(inputFileName, "r+b");
    if (inputFile == NULL) {
        root = -1;      // Root Unknown
        inputFile = fopen(inputFileName, "w+b");
        fwrite(&root, sizeof(long), 1, inputFile);
    } else {
        fread(&root, sizeof(long), 1, inputFile);
    }

    char command[4096];
    int key;
    gets(command);

    while (strcmp(command, "end") != 0) {
        if (strncmp("add", command, 3) == 0) {
            strtok(command, " ");
            key = atoi(strtok(NULL, " "));
            if (findKey(key, root) == 1) {
                printf("Entry with key=%d already exists\n", key);
            } else {
                addKey(key);
            }
        } else if (strncmp("find", command, 4) == 0) {
            strtok(command, " ");
            key = atoi(strtok(NULL, " "));
            if (findKey(key, root) == 1) {
                printf("Entry with key=%d exists\n", key);
            } else {
                printf("Entry with key=%d does not exist\n", key);
            }
        } else if (strncmp("print", command, 5) == 0) {
            if (root != -1) {
                long nodesToPrint[1] = {root};
                printTreeLevel(1, nodesToPrint, 1);
            }
        }
        gets(command);
    }
    fclose(inputFile);
    return 0;
}

void printTreeLevel(int level, long *nodesToPrint, int size) {
    long nextLevelNodes[(int) pow(bTreeOrder, level)];
    int index = 0;
    int i, j;
    printf(" %d: ", level);
    for (i = 0; i < size; ++i) {
        btree_node node = readNode(inputFile, nodesToPrint[i]);
        for (j = 0; j < node.n - 1; ++j) {
            printf("%d,", node.key[j]);
        }
        printf("%d ", node.key[node.n - 1]);
        if (node.child[0] != NULL) {
            for (j = 0; j < node.n + 1; ++j) {
                nextLevelNodes[index++] = node.child[j];
            }
        }
        freeNode(&node);
    }
    printf("\n");
    if (index > 0) {
        printTreeLevel(level + 1, nextLevelNodes, index);
    }
}

int findKey(int key, long offset) {
    btree_node node = readNode(inputFile, offset);
    int i;
    for (i = 0; i < node.n; ++i) {
        if (node.key[i] == key) {
            freeNode(&node);
            return 1;
        } else if (node.key[i] > key) {
            break;
        }
    }

    if (node.child[i] != NULL) {
        long childOffset = node.child[i];
        freeNode(&node);
        return findKey(key, childOffset);
    } else {
        freeNode(&node);
        return -1;
    }
}

long findLeafFor(int key, long offset) {
    return findNodeWithChild(offset, NULL, key);
}

long findNodeWithChild(long offset, long childOffset, int key) {
    btree_node node = readNode(inputFile, offset);
    int i = 0;
    while (i < node.n && node.key[i] < key) {
        i++;
    }
    if (node.child[i] != NULL && node.child[i] != childOffset) {
        long nodeChildOffset = node.child[i];
        freeNode(&node);
        return findNodeWithChild(nodeChildOffset, childOffset, key);
    } else if (node.child[i] == childOffset) {
        freeNode(&node);
        return offset;
    } else {
        freeNode(&node);
        return NULL;
    }
}

void addKey(int key) {
    if (root == -1) {
        btree_node rootNode = newNode();
        root = getEndOffset(inputFile);
        fseek(inputFile, 0, SEEK_SET);
        fwrite(&root, sizeof(long), 1, inputFile);
        writeNode(rootNode, inputFile, root);
        freeNode(&rootNode);
    }
    long leaf = findLeafFor(key, root);
    addKeyAndRightChildTo(leaf, NULL, key);
}

void addKeyAndRightChildTo(long nodeOffset, long rightChildOffset, int key) {
    btree_node node = readNode(inputFile, nodeOffset);
    int k;
    // Find index of key to be added
    int indexOfKey = 0;
    while (indexOfKey < node.n && node.key[indexOfKey] < key) {
        indexOfKey++;
    }
    // If key fits in the node
    if (node.n < bTreeOrder - 1) {
        node.n++;
        node.child[node.n] = node.child[node.n - 1];
        for (k = node.n - 1; k > indexOfKey; --k) {
            node.key[k] = node.key[k - 1];
            node.child[k] = node.child[k - 1];
        }
        node.key[indexOfKey] = key;
        node.child[indexOfKey + 1] = rightChildOffset;
        writeNode(node, inputFile, nodeOffset);
        freeNode(&node);
    } else {        // If adding key overflows the node
        // Temporary lists to maintain overflowed lists
        long tempChildren[bTreeOrder + 1];
        int tempKeys[bTreeOrder];
        int i = 0;
        for (; i < indexOfKey; ++i) {
            tempKeys[i] = node.key[i];
            tempChildren[i] = node.child[i];
        }
        tempKeys[i] = key;
        tempChildren[i] = node.child[indexOfKey];
        i++;
        tempChildren[i] = rightChildOffset;
        for (k = indexOfKey; k < bTreeOrder - 1; ++k) {
            tempKeys[i] = node.key[k];
            i++;
            tempChildren[i] = node.child[k + 1];
        }

        // Median Key Index to promote to parent node
        int medianKey = (int) ceil((double) (bTreeOrder - 1) / 2);

        // Create right Sibling to hold keys after the Median key
        btree_node rightNode = newNode();
        rightNode.n = bTreeOrder - medianKey - 1;
        for (k = 0; k < rightNode.n; ++k) {
            rightNode.key[k] = tempKeys[medianKey + k + 1];
            rightNode.child[k] = tempChildren[medianKey + k + 1];
        }
        rightNode.child[k] = tempChildren[medianKey + k + 1];
        long rightNodeOffset = getEndOffset(inputFile);
        writeNode(rightNode, inputFile, rightNodeOffset);
        freeNode(&rightNode);

        node.n = medianKey;
        for (k = 0; k < node.n; ++k) {
            node.key[k] = tempKeys[k];
            node.child[k] = tempChildren[k];
        }
        node.child[k] = tempChildren[k];
        writeNode(node, inputFile, nodeOffset);

        // insert key to node's parent.
        long parent = findNodeWithChild(root, nodeOffset, node.key[0]);
        freeNode(&node);
        if (parent == NULL) {
            assignToNewRoot(nodeOffset, rightNodeOffset, tempKeys[medianKey]);
        } else {
            addKeyAndRightChildTo(parent, rightNodeOffset, tempKeys[medianKey]);
        }
    }
}

void assignToNewRoot(long leftNode, long rightNode, int key) {
    btree_node rootNode = newNode();
    rootNode.key[0] = key;
    rootNode.n = 1;
    rootNode.child[0] = leftNode;
    rootNode.child[1] = rightNode;
    root = getEndOffset(inputFile);
    fseek(inputFile, 0, SEEK_SET);
    fwrite(&root, sizeof(long), 1, inputFile);
    writeNode(rootNode, inputFile, root);
    freeNode(&rootNode);
}

long getEndOffset(FILE *file) {
    fseek(file, 0, SEEK_END);
    return ftell(file);
}

void writeNode(btree_node node, FILE *file, long offset) {
    fseek(file, offset, SEEK_SET);
    fwrite(&node.n, sizeof(int), 1, file);
    fwrite(node.key, sizeof(int), bTreeOrder - 1, file);
    fwrite(node.child, sizeof(long), bTreeOrder, file);
}

btree_node readNode(FILE *file, long offset) {
    btree_node result = newNode();
    fseek(file, offset, SEEK_SET);
    fread(&result.n, sizeof(int), 1, file);
    fread(result.key, sizeof(int), bTreeOrder - 1, file);
    fread(result.child, sizeof(long), bTreeOrder, file);
    return result;
}