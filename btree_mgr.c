#include "btree_mgr.h"
#include "dberror.h"
#include "storage_mgr.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////            Aung Kham Naung                 ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct BTreeNodeInternal {
    bool isLeaf;
    int numKeys;
    Value **keys;
    RID **rids;
    struct BTreeNodeInternal **children;
    struct BTreeNodeInternal *next;
} BTreeNodeInternal;

typedef struct BTreeInternal {
    DataType keyType;
    int order;
    BTreeNodeInternal *root;
    int numNodes;
    int numEntries;
    BTreeNodeInternal *nodeListHead;
} BTreeInternal;

typedef struct BTScanInternal {
    BTreeNodeInternal *currentNode;
    int currentKeyIndex;
    int scannedEntries;
} BTScanInternal;

BTreeHandle *singleBTree = NULL;

/**
 * Initialize the Index Manager
 */
RC initIndexManager(void *mgmtData) {
    initStorageManager();
    printf("Index Manager initialized successfully.\n");
    return RC_OK;
}

/**
 * Shutdown the Index Manager
 */
RC shutdownIndexManager() {
    if (singleBTree != NULL) {
        RC rc = closeBtree(singleBTree);
        if (rc != RC_OK) {
            fprintf(stderr, "Error closing B-Tree during shutdown.\n");
            return rc;
        }
    }
    printf("Index Manager shut down successfully.\n");
    return RC_OK;
}

/**
 * Creates a new B-Tree with given idxId, keyType, and order
 */
RC createBtree(char *idxId, DataType keyType, int n) {
    if (singleBTree != NULL) {
        THROW(RC_IM_KEY_ALREADY_EXISTS, "A B-Tree already exists. Only one B-Tree is supported.");
    }

    // Allocate BTreeHandle
    singleBTree = (BTreeHandle *)malloc(sizeof(BTreeHandle));
    if (!singleBTree) {
        THROW(RC_ERROR, "Failed to allocate memory for BTreeHandle.");
    }

    singleBTree->keyType = keyType;
    singleBTree->idxId = strdup(idxId);
    if (!singleBTree->idxId) {
        free(singleBTree);
        singleBTree = NULL;
        THROW(RC_ERROR, "Failed to allocate memory for idxId.");
    }

    // Allocate BTreeInternal management data
    singleBTree->mgmtData = malloc(sizeof(BTreeInternal));
    if (!singleBTree->mgmtData) {
        free(singleBTree->idxId);
        free(singleBTree);
        singleBTree = NULL;
        THROW(RC_ERROR, "Failed to allocate memory for BTreeInternal.");
    }

    BTreeInternal *btree = (BTreeInternal *)singleBTree->mgmtData;
    btree->keyType = keyType;
    btree->order = n;
    btree->root = NULL; 
    btree->numNodes = 0;
    btree->numEntries = 0;
    btree->nodeListHead = NULL;

    // Initialize root node
    btree->root = (BTreeNodeInternal *)malloc(sizeof(BTreeNodeInternal));
    if (!btree->root) {
        free(singleBTree->mgmtData);
        free(singleBTree->idxId);
        free(singleBTree);
        singleBTree = NULL;
        THROW(RC_ERROR, "Failed to allocate memory for root node.");
    }

    btree->root->isLeaf = 1;
    btree->root->numKeys = 0;
    btree->root->keys = (Value **)malloc(sizeof(Value *) * (btree->order - 1));
    btree->root->rids = (RID **)malloc(sizeof(RID *) * (btree->order - 1));
    btree->root->children = NULL; 
    btree->root->next = NULL;

    if (!btree->root->keys || !btree->root->rids) {
        free(btree->root->keys);
        free(btree->root->rids);
        free(btree->root);
        free(singleBTree->mgmtData);
        free(singleBTree->idxId);
        free(singleBTree);
        singleBTree = NULL;
        THROW(RC_ERROR, "Failed to allocate memory for root node's keys or RIDs.");
    }

    btree->numNodes = 1;

    printf("B-Tree '%s' created successfully with key type %d and order %d.\n", idxId, keyType, n);
    return RC_OK;
}

/**
 * Opens an existing B-Tree identified by idxId and assigns singleBTree to the provided handle
 */
RC openBtree(BTreeHandle **tree, char *idxId) {
    if (singleBTree == NULL) {
        THROW(RC_FILE_NOT_FOUND, "No B-Tree exists to open.");
    }

    if (strcmp(singleBTree->idxId, idxId) != 0) {
        THROW(RC_FILE_NOT_FOUND, "B-Tree with the given ID does not exist.");
    }

    *tree = singleBTree;

    printf("B-Tree '%s' opened successfully.\n", idxId);
    return RC_OK;
}

/**
 * Closes the currently open B-Tree 
 */
RC closeBtree(BTreeHandle *tree) {
    if (tree == NULL) {
        THROW(RC_ERROR, "Cannot close a NULL B-Tree handle.");
    }

    BTreeInternal *btree = (BTreeInternal *)tree->mgmtData;
    if (btree == NULL) {
        return RC_OK;
    }

    BTreeNodeInternal *currentNode = btree->nodeListHead;
    // Frees all nodes and the BTreeInternal structure.
    while (currentNode != NULL) {
        BTreeNodeInternal *temp = currentNode;
        currentNode = currentNode->next;

        for (int i = 0; i < temp->numKeys; i++) {
            if (btree->keyType == DT_STRING && temp->keys[i]->v.stringV != NULL) {
                free(temp->keys[i]->v.stringV);
                temp->keys[i]->v.stringV = NULL;
            }
            if (temp->keys[i] != NULL) {
                free(temp->keys[i]);
                temp->keys[i] = NULL;
            }
            if (temp->rids[i] != NULL) {
                free(temp->rids[i]);
                temp->rids[i] = NULL;
            }
        }

        free(temp->keys);
        temp->keys = NULL;
        free(temp->rids);
        temp->rids = NULL;
        free(temp);
        temp = NULL;
    }

    free(btree);
    tree->mgmtData = NULL;

    printf("B-Tree closed successfully.\n");
    return RC_OK;
}

/**
 * Deletes the B-Tree identified by idxId
 */
RC deleteBtree(char *idxId) {
    if (singleBTree == NULL) {
        THROW(RC_FILE_NOT_FOUND, "Cannot delete non-existent B-Tree.");
    }

    if (strcmp(singleBTree->idxId, idxId) != 0) {
        THROW(RC_FILE_NOT_FOUND, "B-Tree with the given ID does not exist.");
    }

    // Frees the BTreeHandle and associated memory
    if (singleBTree != NULL) {
        if (singleBTree->idxId != NULL) {
            free(singleBTree->idxId);
            singleBTree->idxId = NULL; 
        }
        free(singleBTree);
        singleBTree = NULL; 
    }
  

    printf("B-Tree '%s' deleted successfully.\n", idxId);
    return RC_OK;
}

/**
 * Get the number of nodes in the B-Tree
 */
RC getNumNodes(BTreeHandle *tree, int *result) {
    if (tree == NULL || result == NULL) {
        THROW(RC_ERROR, "NULL argument passed to getNumNodes.");
    }

    BTreeInternal *btree = (BTreeInternal *)tree->mgmtData;
    *result = btree->numNodes;

    printf("B-Tree '%s' has %d nodes.\n", tree->idxId, *result);
    return RC_OK;
}

/**
 * Get the number of entries in the B-Tree
 */
RC getNumEntries(BTreeHandle *tree, int *result) {
    if (tree == NULL || result == NULL) {
        THROW(RC_ERROR, "NULL argument passed to getNumEntries.");
    }

    BTreeInternal *btree = (BTreeInternal *)tree->mgmtData;
    *result = btree->numEntries;

    printf("B-Tree '%s' has %d entries.\n", tree->idxId, *result);
    return RC_OK;
}

/**
 * Get the key type of the B-Tree
 */
RC getKeyType(BTreeHandle *tree, DataType *result) {
    if (tree == NULL || result == NULL) {
        THROW(RC_ERROR, "NULL argument passed to getKeyType.");
    }

    *result = tree->keyType;

    printf("B-Tree '%s' has key type %d.\n", tree->idxId, *result);
    return RC_OK;
}

/* Initialize B-Tree key type if this is the first insert */
static RC initKeyType(BTreeHandle *tree, Value *key) {
    BTreeInternal *btree = (BTreeInternal *)tree->mgmtData;
    if (btree->numEntries == 0) {
        btree->keyType = key->dt;
        printf("B-Tree key type set to %d based on the first key inserted.\n", btree->keyType);
        btree->nodeListHead = btree->root;
    }
    return RC_OK;
}

/* Insert a key-RID pair into the root when there's space */
static RC insertKeyPairIntoRoot(BTreeInternal *btree, Value *key, RID rid) {
    int idx = btree->root->numKeys;
    btree->root->keys[idx] = (Value *)malloc(sizeof(Value));
    if (!btree->root->keys[idx]) {
        fprintf(stderr, "Error: Failed to allocate memory for key.\n");
        return RC_ERROR;
    }

    if (btree->keyType == DT_STRING) {
        btree->root->keys[idx]->dt = DT_STRING;
        btree->root->keys[idx]->v.stringV = strdup(key->v.stringV);
        if (!btree->root->keys[idx]->v.stringV) {
            free(btree->root->keys[idx]);
            fprintf(stderr, "Error: Failed to allocate memory for string key.\n");
            return RC_ERROR;
        }
    } else {
        *(btree->root->keys[idx]) = *key;
    }

    btree->root->rids[idx] = (RID *)malloc(sizeof(RID));
    if (!btree->root->rids[idx]) {
        if (btree->keyType == DT_STRING) {
            free(btree->root->keys[idx]->v.stringV);
        }
        free(btree->root->keys[idx]);
        fprintf(stderr, "Error: Failed to allocate memory for RID.\n");
        return RC_ERROR;
    }
    *(btree->root->rids[idx]) = rid;

    btree->root->numKeys++;
    btree->numEntries++;

    return RC_OK;
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////   Shanika   ///////////////////////////
/////////////////////////////////////////////////////////////////////////////////

/* Create a new leaf node by extracting the last key-RID pair from the root */
static RC createNewLeaf(BTreeInternal *btree, BTreeNodeInternal **newNode) {
    *newNode = (BTreeNodeInternal *)malloc(sizeof(BTreeNodeInternal));
    if (!(*newNode)) {
        fprintf(stderr, "Error: Failed to allocate memory for new BTreeNodeInternal.\n");
        return RC_ERROR;
    }
    (*newNode)->isLeaf = 1;
    (*newNode)->numKeys = 1;
    (*newNode)->children = NULL; 
    (*newNode)->next = NULL; 

    (*newNode)->keys = (Value **)malloc(sizeof(Value *) * (btree->order - 1));
    (*newNode)->rids = (RID **)malloc(sizeof(RID *) * (btree->order - 1));
    if (!(*newNode)->keys || !(*newNode)->rids) {
        fprintf(stderr, "Error: Failed to allocate memory for new node's keys or RIDs.\n");
        if ((*newNode)->keys) free((*newNode)->keys);
        if ((*newNode)->rids) free((*newNode)->rids);
        free(*newNode);
        return RC_ERROR;
    }

    (*newNode)->keys[0] = (Value *)malloc(sizeof(Value));
    if (!(*newNode)->keys[0]) {
        fprintf(stderr, "Error: Failed to allocate memory for new node's key.\n");
        free((*newNode)->keys);
        free((*newNode)->rids);
        free(*newNode);
        return RC_ERROR;
    }

    BTreeNodeInternal *root = btree->root;
    int idx = root->numKeys - 1;

    // Copy the last key of root to new node
    if (btree->keyType == DT_STRING) {
        (*newNode)->keys[0]->dt = DT_STRING;
        (*newNode)->keys[0]->v.stringV = strdup(root->keys[idx]->v.stringV);
        if (!(*newNode)->keys[0]->v.stringV) {
            free((*newNode)->keys[0]);
            free((*newNode)->keys);
            free((*newNode)->rids);
            free(*newNode);
            fprintf(stderr, "Error: Failed to allocate memory for string key in new node.\n");
            return RC_ERROR;
        }
    } else {
        *(*newNode)->keys[0] = *(root->keys[idx]);
    }

    // Copy the last RID of root to new node
    (*newNode)->rids[0] = (RID *)malloc(sizeof(RID));
    if (!(*newNode)->rids[0]) {
        fprintf(stderr, "Error: Failed to allocate memory for new node's RID.\n");
        if (btree->keyType == DT_STRING) {
            free((*newNode)->keys[0]->v.stringV);
        }
        free((*newNode)->keys[0]);
        free((*newNode)->keys);
        free((*newNode)->rids);
        free(*newNode);
        return RC_ERROR;
    }
    *(*newNode)->rids[0] = *(root->rids[idx]);

    root->numKeys--;

    // Insert new node at head of node list
    (*newNode)->next = btree->nodeListHead;
    btree->nodeListHead = *newNode;

    return RC_OK;
}

/* Insert a key-RID pair into a given node */
static RC insertKeyPairIntoNode(BTreeInternal *btree, BTreeNodeInternal *node, Value *key, RID rid) {
    int idx = node->numKeys;
    node->keys[idx] = (Value *)malloc(sizeof(Value));
    if (!node->keys[idx]) {
        fprintf(stderr, "Error: Failed to allocate memory for key.\n");
        return RC_ERROR;
    }

    // Copy the key
    if (btree->keyType == DT_STRING) {
        node->keys[idx]->dt = DT_STRING;
        node->keys[idx]->v.stringV = strdup(key->v.stringV);
        if (!node->keys[idx]->v.stringV) {
            free(node->keys[idx]);
            fprintf(stderr, "Error: Failed to allocate memory for string key in node.\n");
            return RC_ERROR;
        }
    } else {
        *(node->keys[idx]) = *key;
    }

    node->rids[idx] = (RID *)malloc(sizeof(RID));
    if (!node->rids[idx]) {
        if (btree->keyType == DT_STRING) {
            free(node->keys[idx]->v.stringV);
        }
        free(node->keys[idx]);
        fprintf(stderr, "Error: Failed to allocate memory for RID.\n");
        return RC_ERROR;
    }
    *(node->rids[idx]) = rid;

    node->numKeys++;
    return RC_OK;
}

/* Compare two keys based on their data type */
static int compareKeys(Value *key1, Value *key2, DataType dt) {
    switch (dt) {
        case DT_INT:
            return key1->v.intV - key2->v.intV;
        case DT_STRING:
            return strcmp(key1->v.stringV, key2->v.stringV);
        case DT_FLOAT:
            if (key1->v.floatV < key2->v.floatV) return -1;
            if (key1->v.floatV > key2->v.floatV) return 1;
            return 0;
        case DT_BOOL:
            return (key1->v.boolV == key2->v.boolV) ? 0 : (key1->v.boolV ? 1 : -1);
        default:
            fprintf(stderr, "Error: Unknown DataType in compareKeys.\n");
            return 0;
    }
}

/**
 * Insert a key-RID pair into the B-Tree
 */
RC insertKey(BTreeHandle *tree, Value *key, RID rid) {
    if (tree == NULL || key == NULL) {
        fprintf(stderr, "Error: NULL argument passed to insertKey.\n");
        return RC_ERROR;
    }

    BTreeInternal *btree = (BTreeInternal *)tree->mgmtData;

    RC rc = initKeyType(tree, key);
    if (rc != RC_OK) {
        return rc;
    }

    if (key->dt != btree->keyType) {
        fprintf(stderr, "Error: Key data type mismatch.\n");
        return RC_ERROR;
    }

    // If root has space, insert key there
    if (btree->root->numKeys < btree->order - 1) {
        rc = insertKeyPairIntoRoot(btree, key, rid);
        if (rc != RC_OK) {
            return rc;
        }
        printf("Inserted key into root node of B-Tree '%s'.\n", tree->idxId);
    } 

    // Root is full, create a new leaf and redistribute
    else {
        BTreeNodeInternal *newNode = NULL;
        rc = createNewLeaf(btree, &newNode);
        if (rc != RC_OK) {
            return rc; 
        }

        int cmp = compareKeys(key, newNode->keys[0], btree->keyType);

        if (cmp < 0) {
            rc = insertKeyPairIntoNode(btree, btree->root, key, rid);
        } 
        else {
            rc = insertKeyPairIntoNode(btree, newNode, key, rid);
        }

        if (rc != RC_OK) {
            return rc;
        }

        btree->numNodes++;
        btree->numEntries++;

        printf("Inserted key into a new node of B-Tree '%s'.\n", tree->idxId);
    }

    return RC_OK;
}
    
/**
 * Search the linked list of nodes for the given key
 */
RC findKey(BTreeHandle *tree, Value *key, RID *result) {
    if (tree == NULL || key == NULL || result == NULL) {
        THROW(RC_ERROR, "NULL argument passed to findKey.");
    }

    BTreeInternal *btree = (BTreeInternal *)tree->mgmtData;

    // Search through nodeListHead
    BTreeNodeInternal *currentNode = btree->nodeListHead;
    while (currentNode != NULL) {
        for (int i = 0; i < currentNode->numKeys; i++) {
            
            int cmp = 0;
            switch (btree->keyType) {
                case DT_INT:
                    if (key->dt != DT_INT) {
                        THROW(RC_ERROR, "Key data type mismatch.");
                    }
                    cmp = (currentNode->keys[i]->v.intV - key->v.intV);
                    break;
                case DT_STRING:
                    if (key->dt != DT_STRING) {
                        THROW(RC_ERROR, "Key data type mismatch.");
                    }
                    cmp = strcmp(currentNode->keys[i]->v.stringV, key->v.stringV);
                    break;
                case DT_FLOAT:
                    if (key->dt != DT_FLOAT) {
                        THROW(RC_ERROR, "Key data type mismatch.");
                    }
                    if (currentNode->keys[i]->v.floatV < key->v.floatV) {
                        cmp = -1;
                    } else if (currentNode->keys[i]->v.floatV > key->v.floatV) {
                        cmp = 1;
                    } else {
                        cmp = 0;
                    }
                    break;
                default:
                    THROW(RC_ERROR, "Unknown DataType.");
            }

            if (cmp == 0) {
                
                *result = *(currentNode->rids[i]);
                printf("Key found in B-Tree '%s' at RID: Page %d, Slot %d.\n", 
                       tree->idxId, result->page, result->slot);
                return RC_OK;
            }
        }
        currentNode = currentNode->next; 
    }

    
    printf("Key not found in B-Tree '%s'.\n", tree->idxId);
    return RC_IM_KEY_NOT_FOUND;
}

/**
 * Attempt to delete the given key from the B-Tree
 */
RC deleteKey(BTreeHandle *tree, Value *key) {
    if (tree == NULL || key == NULL) {
        THROW(RC_ERROR, "NULL argument passed to deleteKey.");
    }

    BTreeInternal *btree = (BTreeInternal *)tree->mgmtData;

    BTreeNodeInternal *currentNode = btree->nodeListHead;
    while (currentNode != NULL) {
        for (int i = 0; i < currentNode->numKeys; i++) {
            
            int cmp = 0;
            switch (btree->keyType) {
                case DT_INT:
                    cmp = currentNode->keys[i]->v.intV - key->v.intV;
                    break;
                case DT_STRING:
                    cmp = strcmp(currentNode->keys[i]->v.stringV, key->v.stringV);
                    break;
                case DT_FLOAT:
                    cmp = (currentNode->keys[i]->v.floatV > key->v.floatV) - 
                          (currentNode->keys[i]->v.floatV < key->v.floatV);
                    break;
                default:
                    THROW(RC_ERROR, "Unknown DataType.");
            }

            if (cmp == 0) {
                
                for (int j = i; j < currentNode->numKeys - 1; j++) {
                    currentNode->keys[j] = currentNode->keys[j + 1];
                    currentNode->rids[j] = currentNode->rids[j + 1];
                }
                currentNode->numKeys--;
                btree->numEntries--;
                printf("Key deleted from B-Tree '%s'.\n", tree->idxId);
                return RC_OK;
            }
        }
        currentNode = currentNode->next;
    }

    
    printf("Key not found in B-Tree '%s'.\n", tree->idxId);
    return RC_IM_KEY_NOT_FOUND;
}

///////////////////////////////////////////////////////////////////////////
////////////////////////////////////  Nandini  ////////////////////////////
///////////////////////////////////////////////////////////////////////////

/**
 * Initializes a scan over the B-Tree
 */
RC openTreeScan(BTreeHandle *tree, BT_ScanHandle **handle) {
    if (tree == NULL || handle == NULL) {
        THROW(RC_ERROR, "NULL argument passed to openTreeScan.");
    }

    BTreeInternal *btree = (BTreeInternal *)tree->mgmtData;
    *handle = (BT_ScanHandle *)malloc(sizeof(BT_ScanHandle));
    if (!(*handle)) {
        THROW(RC_ERROR, "Failed to allocate memory for BT_ScanHandle.");
    }

    (*handle)->tree = tree;
    (*handle)->mgmtData = malloc(sizeof(BTScanInternal));
    if (!((*handle)->mgmtData)) {
        free(*handle);
        THROW(RC_ERROR, "Failed to allocate memory for BTScanInternal.");
    }

    // Initialize scan
    BTScanInternal *scan = (BTScanInternal *)(*handle)->mgmtData;
    scan->currentNode = btree->nodeListHead;
    scan->currentKeyIndex = 0;
    scan->scannedEntries = 0; 

    printf("Tree scan opened for B-Tree '%s'.\n", tree->idxId);
    return RC_OK;
}

/**
 * Moving onto next entry
 */
RC nextEntry(BT_ScanHandle *handle, RID *result) {
    if (handle == NULL || result == NULL) {
        THROW(RC_ERROR, "NULL argument passed to nextEntry.");
    }
    BTreeHandle *tree = handle->tree;
    BTreeInternal *btree = (BTreeInternal *)tree->mgmtData;
    BTScanInternal *scan = (BTScanInternal *)handle->mgmtData;



    int *currentPos = (int *)handle->mgmtData;

    if (*currentPos >= btree->numEntries) {
        return RC_IM_NO_MORE_ENTRIES;
    }
    scan->scannedEntries++;
}

/**
 * Frees Handle Data
 */
RC closeTreeScan(BT_ScanHandle *handle) {
    if (handle == NULL) {
        THROW(RC_ERROR, "Cannot close a NULL BT_ScanHandle.");
    }
    free(handle->mgmtData);
    free(handle);

    printf("Scan closed.\n");
    return RC_OK;
}

/**
 * Prints the B-Tree structure 
 */
char* printTree(BTreeHandle *tree) {
    if (tree == NULL) {
        THROW(RC_ERROR, "NULL argument passed to printTree.");
    }

    BTreeInternal *btree = (BTreeInternal *)tree->mgmtData;
    BTreeNodeInternal *root = btree->root;

    if (root == NULL) {
        char *emptyStr = (char*)malloc(11);
        if (!emptyStr) {
            THROW(RC_ERROR, "Failed to allocate memory for empty tree string.");
        }
        strcpy(emptyStr, "Empty B-Tree");
        return emptyStr;
    }

    BTreeNodeInternal *nodesArray[10];
    int totalNodes = 0;

    
    BTreeNodeInternal *stack[10];
    int stackSize = 0;

    
    stack[stackSize++] = root;

    while (stackSize > 0 && totalNodes < 10) {
        BTreeNodeInternal *node = stack[--stackSize];
        nodesArray[totalNodes++] = node;

        if (!node->isLeaf) {
            for (int i = node->numKeys; i >= 0; i--) {
                if (node->children[i] != NULL && totalNodes + (node->numKeys + 1) < 10) {
                    stack[stackSize++] = node->children[i];
                }
            }
        }
    }

    char *result = (char*)malloc(5000);
    if (!result) {
        THROW(RC_ERROR, "Failed to allocate memory for result.");
    }
    result[0] = '\0';
    for (int i = 0; i < totalNodes; i++) {
        BTreeNodeInternal *node = nodesArray[i];
        char line[512];
        int pos = 0;
        pos += sprintf(line + pos, "(%d)[", i);

        if (node->isLeaf) {
            for (int k = 0; k < node->numKeys; k++) {
                pos += sprintf(line + pos, "%d.%d,%d", 
                               node->rids[k]->page, node->rids[k]->slot,
                               node->keys[k]->v.intV);
                if (k < node->numKeys - 1) {
                    pos += sprintf(line + pos, ",");
                }
            }
        } 
        else {    
            for (int c = 0; c <= node->numKeys; c++) {
                BTreeNodeInternal *child = node->children[c];
                int childId = -1;
                if (child != NULL) {
                    for (int j = 0; j < totalNodes; j++) {
                        if (nodesArray[j] == child) {
                            childId = j;
                            break;
                        }
                    }
                }

                pos += sprintf(line + pos, "%d", childId);
                if (c < node->numKeys) {
                    pos += sprintf(line + pos, ",%d,", node->keys[c]->v.intV);
                }
            }
        }

        pos += sprintf(line + pos, "]\n");
        strcat(result, line);
    }
    return result;
}
