Assignment 4 - B+ tree 

***Group No. 2: Team members
1.	Sneha Shahaji Kadam (skadam5@hawk.iit.edu)
2.	Ayelet Bendor (abendor@hawk.iit.edu)
3.	Naga Sirisha Chalapati (nchalapati@hawk.iit.edu)

***Homework Assignment 4 has been completed and following files were uploaded under Assignment4 folder:
1.	Makefile
2.	btree_mgr.c
3.	btree_mgr.h
4.	record_mgr.c
5.	record_mgr.h
6.	expr.c
7.	expr.h
8.	table.h
9.	rm_serializer.c
10.	buffer_mgr.c
11.	buffer_mgr.h
12.	buffer_mgr_stat.c
13.	buffer_mgr_stat.h
14.	dberror.c
15.	dberror.h
16.	dt.h
17.	storage_mgr.c
18.	storage_mgr.h
19.	test_assign4_1.c
20.	test_expr.c
21.	test_helper.h

***Steps to compile and execute program is as follows:
1.	Open terminal and navigate to project directory.
example: cd CS525/Assignment4
2.	Type make to compile source code files.
By default "make" will compile test_assign4_1.c(test case file).
For compiling source code file test_expr.c, type "make test_expr".
3.	Execute program using executable file.
example:  ./test_assign4 or ./test_expr
4.	Type "make clean" to delete old compiled files.
5.	We can check memory leaks with valgrind using following command:
	valgrind --leak-check=full --track-origins=yes ./test_assign4

***Assumptions made per code design:
1.	Current leaf node has at least one sibling node to its right or left 
2.	For deleteKey method we design our code based on the following assumptions:
a.	When a node contains a single key (which is allowed since it’s not below the minimum number of keys), and this key gets randomly selected to be deleted and sibling node contains a single key as well, we’ll perform a merge with sibling node.
b.	Terminology in the method:
resNode = root node
leafNode = current node where the key is stored
siblingNode = left sibling node
rightSiblingNode = right sibling node
2.	Delete key method works well for all scenarios except when doing a merge. With the time, we unfortunately left it as is. For a scenario when random keys are selected and no merge required, delete test case completes all tests.
We also noticed different behaviour between running the code on different machines and when it’s running on a Windows versus Linux environment. 
3.	Our program is having some memory leaks, most of those leaks are coming from mallocing pointers to nodes on createNode method. To solve this problem, we rewrite closeBtree (we left it as a comment in the program) to free node and tree. Shortly before submitting the assignment we ran into issues with the revised version of this method, we decided to go with the old one and left the new one for you to see that we addressed this issue. 


***Methods implemented for findKey method:
1.	int searchNodes(struct Node *pgHeader, int key, RID *result)
This method is called from the findKey method and is used to identify the appropriate leaf node of the key. It runs recursively by comparing the given key value with the key values of the non-leaf nodes starting from the root node. 


***Methods implemented for insertKey method:
1.	RID *createNewRecord(RID *rid)
This method creates a new record to hold the value to which a key refers.

2.	Node *createNewTree(BTreeManager_Metadata *metadata, int key, RID *record)
This method creates a new tree when there is the first key to be inserted.

3.	Node *createLeaf(BTreeManager_Metadata *metadata)
This method creates a new leaf by creating a node.

4.	Node *createNode(BTreeManager_Metadata *metadata)
This method creates a new general node, which can be either a leaf or a non-leaf node.

5.	Node *findLeaf(Node *root, int key)
This method finds the leaf node where the key can be inserted.

6.	Node *insertIntoLeaf(BTreeManager_Metadata *metadata, Node *leaf, int key, RID *record)
This method inserts a key into a leaf and returns the modified leaf.

7.	Node *insertIntoLeafAfterSplitting(BTreeManager_Metadata *metadata,Node *leaf, int key, RID *record)
This method will split the leaf into two and insert a key into the new leaf node.

8.	Node *insertIntoParent(BTreeManager_Metadata *metadata, Node *left, int key, Node *right)
This method inserts a new node (leaf or non-leaf node) into the B+ tree and returns the root of the tree after insertion.

9.	Node *insertIntoNode(BTreeManager_Metadata *metadata, Node *parent, int left_index, int key, Node *right)
This method inserts a new key and pointer to a node into a node.

10.	Node *insertIntoNodeAfterSplitting(BTreeManager_Metadata *metadata, Node *parent, int left_index, int key, Node *right)
This method will split the node into two and insert a new key and pointer into the new node.

11.	Node *insertIntoNewRoot(BTreeManager_Metadata *metadata, Node *left, int key, Node *right)
This method creates a new root and inserts the key into the new root.

12.	int getLeftIndex(Node *parent, Node *left)
This method finds the index of the parent's pointer to the node to the left of the key to be inserted.

***Additional method implemented to print page header to index file on disk :
writePageData(): This method prints the values of node number, isLeafNode, number of keys and the key values of the node to the respective page as page header.


***Methods implemented in btree_mgr.c are as follows:
// init and shutdown index manager
1.	initIndexManager ():
This method initializes the index manager.

2.	shutdownIndexManager ():
This method shuts down the index manager and de-allocates all the resources allocated to the index manager.

// create, destroy, open, and close an btree index
3.	createBtree ():
This method creates B+ tree and initializes the TreeManager structure which stores additional information about B+ Tree.

4.	openBtree ():
This method initializes the buffer pool followed by initializing existing tree structure.

5.	closeBtree ():
This method shuts down the buffer pool and free the space in memory occupied by tree metadata.

6.	deleteBtree ():
This method deletes the file page created by the buffer pool.

// access information about a b-tree
7.	getNumNodes ():
This method returns the number of nodes present in our B+ Tree.

8.	getNumEntries ():
This method returns the number of keys present in our B+ Tree.

9.	getKeyType ():
This method returns the key type present in our B+ Tree.

10.	gerMinKeysPerNode():
Based on n number of keys, this method calculates the min number of keys per node (taking in account whether it’s a leaf or non leaf node)

// index access
11.	findKey ():
This method calls searchNodes method which returns the appropriate RID of the key and if the key is not found it returns RC_IM_KEY_NOT_FOUND.

12.	insertKey ():
●	This method adds a new record with the specified key and RID.
●	Find the key inside  B+ Tree. If it is found, then returns RC_IM_KEY_ALREADY_EXISTS. If not found, then creates a record which stores the RID.
●	If the root of the tree is empty then creates a new B+ Tree and adds this entry to the tree. If a tree already exists then find the leaf node where the key can be inserted.
●	Once the leaf node has been found,  checks if it has space for the new key. If yes, then call insertIntoLeaf which performs the insertion.
●	If the leaf node is full, the calls insertIntoLeafAfterSplitting which splits the leaf node and then inserts the key. Also, update the key at the root node.
●	If the existing root node is full then calls insertIntoNodeAfterSplitting to split node, redistributes entries evenly, and pushes up the middle key in order to update the new root of the tree.

13.	deleteKey ():
This method receives a key from the user’s program, the key is to be delete from the tree index key. 
The method performs a search of the key in the B+ tree. To get the key location, we use a counter in searchKey method, which keeps a track of the number of times we entered a loop to find the correct node and the key index. 
With the above information, we can get the key’s current node and its sibling to the right or left. This information will come handy for when we decide on the delete method, which is divided into four cases in the method. 

14.	openTreeScan ():
This method initializes the scan structure which is used to scan the entries in the B+ Tree.

15.	nextEntry ():
This method is used to scan the entries in the B+ Tree.

16.	closeTreeScan ():
This method closes tree scan and frees up allocated space.

// debug and test functions
17.	printTree ():
This method prints the B+ Tree starting from the root node. Each node is printed on a new line with the node number. It prints the pointer position and the keys alternatively in a sequence for a non leaf node, and prints the RID (page and slot numbers) and keys.
