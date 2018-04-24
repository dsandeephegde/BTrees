#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef struct {  /* B-tree node */
    int n;        /* Number of keys in node */
    int *key;      /* Node's keys */
    long *child;    /* Node's child subtree offsets */
} btree_node;

int order = 4;    /* B-tree order */

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

int comparator(const void *p, const void *q);

void addKey(int key);

void writeNode(btree_node node, FILE *file, long offset);

btree_node readNode(FILE *file, long offset);

int findKey(int key, long node);

long getEndOffset(FILE *file);

void appendNode(btree_node node, FILE *file) ;

long findNodeWithChild(long offset, long childOffset, int key);

void printLevel(int level, long nodesToPrint[], int size);

void promoteToParentNode(long parentOffset, long rightChildOffset, int promotedKey);

int main() {
    fp = fopen("index.bin", "r+b");

    /* If file doesn't exist, set root offset to unknown and create * file, otherwise read the root offset at the front of the file */
    if (fp == NULL) {
        root = -1;
        fp = fopen("index.bin", "w+b");
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
            rootNode = readNode(fp, root);
        } else if (strncmp("find", command, 4) == 0) {
            strtok(command, " ");
            key = atoi(strtok(NULL, " "));
            rootNode = readNode(fp, root);
            if(findKey(key, root) == 1){
                printf("Entry with key=%d exists\n", key);
            } else {
                printf("Entry with key=%d does not exist\n", key);
            }
        } else if (strncmp("print", command, 5) == 0) {
            rootNode = readNode(fp, root);
            if(rootNode.n > 0){
                long nodesToPrint[1] = {root};
                printLevel(1, nodesToPrint, 1);
            }
        }
        gets(command);
    }
    fclose(fp);
    return 0;
}

void printLevel(int level, long nodesToPrint[], int size) {
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
    }
    printf("\n");
    if(index > 0){
        printLevel(level+1, nextLevelNodes, index);
    }
}

int findKey(int key, long offset) {
    btree_node node = readNode(fp, offset);
    int i;
    for (i = 0; i < node.n; ++i) {
        if(node.key[i] == key){
            return 1;
        } else if(node.key[i] > key) {
            break;
        }
    }

    if(node.child[i] != NULL) {
        return findKey(key, node.child[i]);
    } else{
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
        return findLeafFor(key, node.child[i]);
    } else{
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
        return findNodeWithChild(node.child[i], childOffset, key);
    } else if(node.child[i] == childOffset){
        return offset;
    } else {
        return NULL;
    }
}

void addKey(int key) {
    if(findKey(key, root) == 1){
        printf("Entry with key=%d already exists\n", key);
        return;
    }
    if(root == -1){
        rootNode = newNode();
        root = getEndOffset(fp);
        fseek(fp, 0, SEEK_SET);
        fwrite(&root, sizeof(long), 1, fp);
        appendNode(rootNode, fp);
    }
    long leafOffset = findLeafFor(key, root);
    btree_node leafNode = readNode(fp, leafOffset);
    if(leafNode.n < order - 1){
        leafNode.key[leafNode.n++] = key;
        qsort(leafNode.key, leafNode.n, sizeof(int), comparator);
        writeNode(leafNode, fp, leafOffset);
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

        leafNode.n = m;
        for (j = 0; j < leafNode.n; ++j) {
            leafNode.key[j] = temp[j];
        }
        writeNode(leafNode, fp, leafOffset);

        // insert key to node's parent.
        long parent = findNodeWithChild(root, leafOffset, leafNode.key[0]);
        if(parent == NULL){
            rootNode = newNode();
            rootNode.key[0] = temp[m];
            rootNode.n = 1;
            rootNode.child[0] = leafOffset;
            rootNode.child[1] = rightLeafOffset;
            root = getEndOffset(fp);
            fseek(fp, 0, SEEK_SET);
            fwrite(&root, sizeof(long), 1, fp);
            writeNode(rootNode, fp, root);
        } else {
            promoteToParentNode(parent, rightLeafOffset, temp[m]);
        }
    }
}

void promoteToParentNode(long parentOffset, long rightChildOffset, int promotedKey) {
    btree_node parentNode = readNode(fp, parentOffset);
    int k;
    if (parentNode.n < order - 1) {
        int index = 0;
        while (index < parentNode.n && parentNode.key[index] < promotedKey) {
            index++;
        }
        parentNode.n++;
        for (k = parentNode.n; k > index; --k) {
            parentNode.key[k] = parentNode.key[k - 1];
            parentNode.child[k] = parentNode.child[k - 1];
        }
        parentNode.key[index] = promotedKey;
        parentNode.child[index + 1] = rightChildOffset;
        writeNode(parentNode, fp, parentOffset);
    } else {
        int indexOfPromotedKey = 0;
        while (indexOfPromotedKey < parentNode.n && parentNode.key[indexOfPromotedKey] < promotedKey){
            indexOfPromotedKey++;
        }

        long tempChildren[order+1];
        int tempKeys[order];
        int i = 0;
        for (; i < indexOfPromotedKey; ++i) {
            tempKeys[i] = parentNode.key[i];
            tempChildren[i] = parentNode.child[i];
        }
        tempKeys[i] = promotedKey;
        tempChildren[i] = parentNode.child[indexOfPromotedKey];
        i++;
        tempChildren[i] = rightChildOffset;

        for (k = indexOfPromotedKey; k < order-1; ++k) {
            tempKeys[i] = parentNode.key[k];
            i++;
            tempChildren[i] = parentNode.child[k+1];
        }

        int m = (int) ceil((double) (order - 1) / 2);

        btree_node rightLeafNode = newNode();
        rightLeafNode.n = order - m - 1;
        int j = 0;
        for (; j < rightLeafNode.n; ++j) {
            rightLeafNode.key[j] = tempKeys[m + j + 1];
            rightLeafNode.child[j] = tempChildren[m + j + 1];
        }
        rightLeafNode.child[j] = tempChildren[m + j + 1];
        long rightLeafOffset = getEndOffset(fp);
        appendNode(rightLeafNode, fp);

        parentNode.n = m;
        int l;
        for (l = 0; l < parentNode.n; ++l) {
            parentNode.key[l] = tempKeys[l];
            parentNode.child[l] = tempChildren[l];
        }
        parentNode.child[l] = tempChildren[l];
        writeNode(parentNode, fp, parentOffset);

        // insert key to node's parent.
        long parent = findNodeWithChild(root, parentOffset, parentNode.key[0]);
        if (parent == NULL) {
            rootNode = newNode();
            rootNode.key[0] = tempKeys[m];
            rootNode.n = 1;
            rootNode.child[0] = parentOffset;
            rootNode.child[1] = rightLeafOffset;
            root = getEndOffset(fp);
            fseek(fp, 0, SEEK_SET);
            fwrite(&root, sizeof(long), 1, fp);
            writeNode(rootNode, fp, root);
        } else {
            promoteToParentNode(parent, rightLeafOffset, tempKeys[m]);
        }
    }
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