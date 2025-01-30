# Advanced Database Organisation (Fall 2024) - Database Management System

## Group 18:

1. Nandini Devraj - A20593534, ndevraj@hawk.iit.edu
2. Shanika Kadidal Sundresh - A20585446, skadidalsundresh@hawk.iit.edu
3. Aung Kham Naung - A20491106, anaung@hawk.iit.edu

# Steps to compile and run the assignment 4:

1. 'make clean'
2. 'make'
3. './test_assign4_group18'

# Steps involed in performing the operations:

/////////////////////////////////////////// AUNG ///////////////////////////////////////////

1. The function 'initIndexManager(void \*mgmtData)' initializes the Index Manager. It sets up the required storage management system and prepares the Index Manager for further operations.

2. This function, 'shutdownIndexManager', gracefully shuts down the Index Manager by closing the active B+ Tree if it exists, ensuring proper cleanup. It logs a success message upon completion or an error message if the B+ Tree fails to close.

3. The 'createBtree' function creates a new B+ Tree with the specified identifier ('idxId'), key type ('keyType'), and order ('n'). It initializes the tree's structure, including its root node, while ensuring proper memory allocation and error handling.

4. The 'openBtree' function opens an existing B+ Tree identified by 'idxId' and assigns it to the provided handle. It verifies the existence and validity of the B+ Tree and logs a success message upon successful opening.

5. The 'closeBtree' function closes the currently open B+ Tree by deallocating all its nodes, keys, and associated resources to ensure proper memory cleanup. It safely frees the tree's management data and logs a success message once the operation is complete.

6. The 'deleteBtree' function deletes an existing B+ Tree identified by 'idxId', ensuring it matches the currently active tree. It deallocates all associated memory and resources, logging a success message upon completion.

7. The 'getNumNodes' function retrieves the total number of nodes in the specified B+ Tree and stores the result in the provided variable. It ensures the input arguments are valid and logs the node count for the specified tree.

8. The 'getNumEntries' function retrieves the total number of entries stored in the specified B+ Tree and saves the result in the provided variable. It validates input arguments and logs the entry count for the tree.

9. The function is 'getKeyType'. It retrieves the key type of a B-Tree and stores it in the provided 'result' variable.

10. The function is 'initKeyType'. It initializes the B-Tree's key type based on the first key inserted and sets the root node as the head of the node list if the B-Tree is empty.

11. The 'insertKeyPairIntoRoot' function inserts a key-RID pair into the root of the B-Tree if there is space, handling memory allocation for the key and RID. It also performs error checks and ensures proper memory management when the key type is a string.

///////////////////////////////// SHANIKA //////////////////////////////////

12. The 'createNewLeaf' function creates a new leaf node by extracting the last key-RID pair from the root, handling memory allocation for the new node's keys and RIDs. It also ensures proper error handling and updates the node list by inserting the new leaf node at the head of the list.

13. The 'insertKeyPairIntoNode' function inserts a key-RID pair into a specified B-Tree node, allocating memory for the key and RID. It handles both string and non-string key types and includes error handling to ensure proper memory management during the insertion process.

14. The 'compareKeys' function compares two keys based on their data type, supporting integer, string, float, and boolean types. It returns an integer indicating the comparison result, and includes an error message for unsupported data types.

15. The 'insertKey' function manages the insertion of a key-RID pair into a B-Tree, utilizing helper functions such as 'initKeyType', 'insertKeyPairIntoRoot', 'createNewLeaf', and 'insertKeyPairIntoNode'. These functions work together to initialize the key type, insert into the root or a new node, and handle memory allocation and error checks for each operation.

16. The 'findKey' function searches through a linked list of B-Tree nodes for the given key, comparing each key in the node based on its data type. If the key is found, it returns the corresponding RID, otherwise, it prints a message indicating that the key was not found.

17. The 'deleteKey' function attempts to delete a given key from the B-Tree by searching through the linked list of nodes and removing the key if found. It shifts the remaining keys and RIDs to fill the gap, updates the key count, and prints a message upon successful deletion or if the key is not found.

//////////////////////////////////// NANDINI //////////////////////////////////

18. The 'openTreeScan' function initializes a scan over the B-Tree by allocating memory for a scan handle and setting up the scan's internal state, including the current node and key index. It ensures proper memory allocation and prints a message confirming that the scan has been successfully opened for the B-Tree.

19. The 'nextEntry' function advances the scan to the next entry in the B-Tree, updating the scanned entries count and checking if there are more entries to scan. If the scan has reached the end of the entries, it returns an error indicating no more entries are available.

20. The 'closeTreeScan' function frees the memory allocated for the scan handle and its associated data. It ensures proper resource cleanup and prints a message confirming that the scan has been successfully closed.

21. The 'printTree' function generates a textual representation of the B-Tree structure. It traverses the tree starting from the root, collects node data, and formats it into a readable string for output.

22. A stack is used to perform a depth-first search (DFS) of the tree, pushing nodes onto the stack as they are encountered. The nodes are then collected into an array (nodesArray) for formatting and printing, ensuring the tree structure is traversed correctly without missing any nodes.

23. Memory is dynamically allocated for the resulting string that holds the tree structure (result), and the node information is formatted using sprintf. The function constructs the tree's textual representation step by step, ensuring efficient memory management by limiting the number of nodes processed.
