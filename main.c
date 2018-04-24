#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef struct {  /* B-tree node */
    int n;        /* Number of keys in node */
    int *key;      /* Node's keys */
    long *child;    /* Node's child subtree offsets */
} btree_node;

int order;    /* B-tree order */
btree_node rootNode;  /* Single B-tree node */
FILE *fp; /* Input file stream */
long root; /* Offset of B-tree root node */

btree_node newNode() {
    btree_node node;
    node.n = 0;
    node.key = (int *) calloc(order - 1, sizeof(int));
    node.child = (long *) calloc(order, sizeof(long));
    return node;
}

void freeNode(btree_node *node) {
    free(node->key);
    free(node->child);
}

int comparator(const void *p, const void *q);

void addKey(int key);

void writeNode(btree_node node, FILE *file, long offset);

btree_node readNode(FILE *file, long offset);

int findKey(int key, long offset);

long getEndOffset(FILE *file);

void appendNode(btree_node node, FILE *file) ;

long findNodeWithChild(long offset, long childOffset, int key);

void printTreeLevel(int level, long *nodesToPrint, int size);

void promoteToParentNode(long nodeOffset, long rightChildOffset, int promotedKey);

void assignToNewRoot(long nodeOffset, long rightNodeOffset, int key);

int main() {
    fp = fopen("index.bin", "r+b");
    order = 4;

    if (fp == NULL) {
        root = -1;
        fp = fopen("index.bin", "w+b");
        fwrite(&root, sizeof(long), 1, fp);
    } else {
        fread(&root, sizeof(long), 1, fp);
    }

    char command[4096];
    int key;
    gets(command);

    while (strcmp(command, "end") != 0) {
        if (strncmp("add", command, 3) == 0) {
            strtok(command, " ");
            key = atoi(strtok(NULL, " "));
            if(findKey(key, root) == 1){
                printf("Entry with key=%d already exists\n", key);
            } else {
                addKey(key);
            }
        } else if (strncmp("find", command, 4) == 0) {
            strtok(command, " ");
            key = atoi(strtok(NULL, " "));
            if(findKey(key, root) == 1){
                printf("Entry with key=%d exists\n", key);
            } else {
                printf("Entry with key=%d does not exist\n", key);
            }
        } else if (strncmp("print", command, 5) == 0) {
            if(root != -1){
                long nodesToPrint[1] = {root};
                printTreeLevel(1, nodesToPrint, 1);
            }
        }
        gets(command);
    }
    fclose(fp);
    return 0;
}

void printTreeLevel(int level, long *nodesToPrint, int size) {
    long nextLevelNodes[(int)pow(order, level)];
    int index = 0;
    int i, j;
    printf(" %d: ", level);
    for (i = 0; i < size; ++i) {
        btree_node node = readNode(fp, nodesToPrint[i]);
        for (j = 0; j < node.n - 1; ++j) {
            printf("%d,", node.key[j]);
        }
        printf("%d ", node.key[node.n - 1] );
        if(node.child[0] != NULL){
            for (j = 0; j < node.n + 1; ++j) {
                nextLevelNodes[index++] = node.child[j];
            }
        }
        freeNode(&node);
    }
    printf("\n");
    if(index > 0){
        printTreeLevel(level + 1, nextLevelNodes, index);
    }
}

int findKey(int key, long offset) {
    btree_node node = readNode(fp, offset);
    int i;
    for (i = 0; i < node.n; ++i) {
        if(node.key[i] == key){
            freeNode(&node);
            return 1;
        } else if(node.key[i] > key) {
            break;
        }
    }

    if(node.child[i] != NULL) {
        long childOffset = node.child[i];
        freeNode(&node);
        return findKey(key, childOffset);
    } else{
        freeNode(&node);
        return -1;
    }
}

long findLeafFor(int key, long offset){
    btree_node node = readNode(fp, offset);
    int i = 0;
    while (i < node.n && node.key[i] < key){
        i++;
    }
    if(node.child[i] != NULL) {
        long childOffset = node.child[i];
        freeNode(&node);
        return findLeafFor(key, childOffset);
    } else{
        freeNode(&node);
        return offset;
    }
}

long findNodeWithChild(long offset, long childOffset, int key) {
    btree_node node = readNode(fp, offset);
    int i = 0;
    while (i < node.n && node.key[i] < key){
        i++;
    }
    if(node.child[i] != NULL && node.child[i] != childOffset) {
        long nodeChildOffset = node.child[i];
        freeNode(&node);
        return findNodeWithChild(nodeChildOffset, childOffset, key);
    } else if(node.child[i] == childOffset){
        freeNode(&node);
        return offset;
    } else {
        freeNode(&node);
        return NULL;
    }
}

void addKey(int key) {
    if(root == -1){
        rootNode = newNode();
        root = getEndOffset(fp);
        fseek(fp, 0, SEEK_SET);
        fwrite(&root, sizeof(long), 1, fp);
        appendNode(rootNode, fp);
        freeNode(&rootNode);
    }
    long leafOffset = findLeafFor(key, root);
    btree_node leafNode = readNode(fp, leafOffset);
    if(leafNode.n < order - 1){
        leafNode.key[leafNode.n++] = key;
        qsort(leafNode.key, leafNode.n, sizeof(int), comparator);
        writeNode(leafNode, fp, leafOffset);
        freeNode(&leafNode);
    } else {
        int temp[order];
        int i = 0;
        for (; i < leafNode.n; ++i) {
            temp[i] = leafNode.key[i];
        }
        temp[i] = key;
        qsort(temp, order, sizeof(int), comparator);

        int m = (int) ceil((double) (order - 1) / 2);

        btree_node rightLeafNode = newNode();
        rightLeafNode.n = order - m - 1;
        int j = 0;
        for (; j < rightLeafNode.n; ++j) {
            rightLeafNode.key[j] = temp[m+j+1];
        }
        long rightLeafOffset = getEndOffset(fp);
        appendNode(rightLeafNode, fp);
        freeNode(&rightLeafNode);

        leafNode.n = m;
        for (j = 0; j < leafNode.n; ++j) {
            leafNode.key[j] = temp[j];
        }
        writeNode(leafNode, fp, leafOffset);

        // insert key to node's parent.
        long parentOffset = findNodeWithChild(root, leafOffset, leafNode.key[0]);
        freeNode(&leafNode);
        if(parentOffset == NULL){
            assignToNewRoot(leafOffset, rightLeafOffset, temp[m]);
        } else {
            promoteToParentNode(parentOffset, rightLeafOffset, temp[m]);
        }
    }
}

void promoteToParentNode(long nodeOffset, long rightChildOffset, int promotedKey) {
    btree_node node = readNode(fp, nodeOffset);
    int k;
    if (node.n < order - 1) {
        int index = 0;
        while (index < node.n && node.key[index] < promotedKey) {
            index++;
        }
        node.n++;
        node.child[node.n] = node.child[node.n - 1];
        for (k = node.n - 1; k > index; --k) {
            node.key[k] = node.key[k - 1];
            node.child[k] = node.child[k - 1];
        }
        node.key[index] = promotedKey;
        node.child[index + 1] = rightChildOffset;
        writeNode(node, fp, nodeOffset);
        freeNode(&node);
    } else {
        int indexOfPromotedKey = 0;
        while (indexOfPromotedKey < node.n && node.key[indexOfPromotedKey] < promotedKey){
            indexOfPromotedKey++;
        }

        long tempChildren[order+1];
        int tempKeys[order];
        int i = 0;
        for (; i < indexOfPromotedKey; ++i) {
            tempKeys[i] = node.key[i];
            tempChildren[i] = node.child[i];
        }
        tempKeys[i] = promotedKey;
        tempChildren[i] = node.child[indexOfPromotedKey];
        i++;
        tempChildren[i] = rightChildOffset;

        for (k = indexOfPromotedKey; k < order-1; ++k) {
            tempKeys[i] = node.key[k];
            i++;
            tempChildren[i] = node.child[k+1];
        }

        int m = (int) ceil((double) (order - 1) / 2);

        btree_node rightNode = newNode();
        rightNode.n = order - m - 1;
        int j = 0;
        for (; j < rightNode.n; ++j) {
            rightNode.key[j] = tempKeys[m + j + 1];
            rightNode.child[j] = tempChildren[m + j + 1];
        }
        rightNode.child[j] = tempChildren[m + j + 1];
        long rightNodeOffset = getEndOffset(fp);
        appendNode(rightNode, fp);
        freeNode(&rightNode);

        node.n = m;
        int l;
        for (l = 0; l < node.n; ++l) {
            node.key[l] = tempKeys[l];
            node.child[l] = tempChildren[l];
        }
        node.child[l] = tempChildren[l];
        writeNode(node, fp, nodeOffset);

        // insert key to node's parent.
        long parentOffset = findNodeWithChild(root, nodeOffset, node.key[0]);
        freeNode(&node);
        if (parentOffset == NULL) {
            assignToNewRoot(nodeOffset, rightNodeOffset, tempKeys[m]);
        } else {
            promoteToParentNode(parentOffset, rightNodeOffset, tempKeys[m]);
        }
    }
}

void assignToNewRoot(long nodeOffset, long rightNodeOffset, int key) {
    rootNode = newNode();
    rootNode.key[0] = key;
    rootNode.n = 1;
    rootNode.child[0] = nodeOffset;
    rootNode.child[1] = rightNodeOffset;
    root = getEndOffset(fp);
    fseek(fp, 0, SEEK_SET);
    fwrite(&root, sizeof(long), 1, fp);
    writeNode(rootNode, fp, root);
    freeNode(&rootNode);
}

long getEndOffset(FILE *file) {
    fseek(file, 0, SEEK_END);
    return ftell(file);
}

void writeNode(btree_node node, FILE *file, long offset) {
    fseek(file, offset, SEEK_SET);
    fwrite(&node.n, sizeof(int), 1, file);
    fwrite(node.key, sizeof(int), order - 1, file);
    fwrite(node.child, sizeof(long), order, file);
}

void appendNode(btree_node node, FILE *file) {
    fseek(file, 0, SEEK_END);
    fwrite(&node.n, sizeof(int), 1, file);
    fwrite(node.key, sizeof(int), order - 1, file);
    fwrite(node.child, sizeof(long), order, file);
}

btree_node readNode(FILE *file, long offset) {
    btree_node result = newNode();
    fseek(file, offset, SEEK_SET);
    fread(&result.n, sizeof(int), 1, file);
    fread(result.key, sizeof(int), order-1, file);
    fread(result.child, sizeof(long), order, file);
    return result;
}

int comparator(const void *p, const void *q) {
    return *(const int *)p - *(const int *)q;
}