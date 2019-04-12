## DIY SQLite
By Mary Keenan

### Project Goal
The goal of this project was to implement my own version of SQLite, a common relational database. My MVP was a database that could save data to the disk and fetch it, and my stretch goal was to investigate more efficient ways of storing and fetching data.

### Learning Goals
My learning goal was to practice writing code in C and to learn more about how relational databases work. A relational database is a collection of data that is organized in tables where each row represents a data object and each column stores an attribute of that object. For example, if you are storing a list of customers in a relational database, each row might represent a different customer; then, the columns would be the customer attributes you want to save, such as names and emails. 

Beyond a general curiosity of relational databases, I was also interested in learning about how insert and lookup operations could be optimized. How do you add a row to the middle of a table without moving every row after it back one spot? Or, how do you find a specific row without searching the entire table? I hoped to be able to answer these questions by the end of my project.

### Resources
To achieve my project and learning goals, I followed a tutorial by Square Inc employee Connor Stack, located at https://cstack.github.io/db_tutorial/. The tutorial covered the theory behind SQLite (how does it work and why does it work like that?) and helped me gradually construct a decent SQLite implementation. It covered topics such as persistence, cursor abstraction, B-trees, binary search, and node splitting, as well as offered a brief look at RSpec testing.

### Results
<img align="right" width="140" height="600" src="https://github.com/mary-keenan/SoftSysDIYLite/blob/master/images/tree_structure.png">

I successfully completed the 13 parts of the tutorial mentioned above, which means I was able to create a SQLite-esque program that could handle storage for a specific (hardcoded) table structure with three columns: ID, username, and email. When run, the **diylite** program offers a command-line interface for the user to perform “insert” and “select” database operations; the image below shows the results of those commands. The **diylite** program also has a command, “mk_tree”, that produces a visualization of the current database tree structure; the image to the right shows a tree with four leaf nodes. 

![insert + select](/images/insert_select.png)

My implementation of a SQLite program stores data in a B-tree, which is a self-balancing tree data structure. The data itself is stored in leaf nodes, and leaf nodes map to internal nodes in a parent-child-like structure. An internal node can have multiple children (other internal nodes or leaf nodes) but leaf nodes are childless. Each internal node stores information about its children; in my implementation, the internal node tracks the highest ID of each of its children. As a result, the B-tree structure allows for quick data operations in logarithmic time, because not every node in the tree needs to be searched in order to find a specific value. Rather, at each level of the tree, the program can choose a single child to search based on its ID values. If you want more information about B-trees, this Wikipedia article has a good explanation: https://en.wikipedia.org/wiki/B-tree. 

The snippet of code below shows part of the split_leaf_and_insert() function. In this snippet, the program is initializing a new leaf node because the existing leaf node is at full capacity. When we initialize a new node, we grab an unused page and assign it to the node by populating it with the node's information. Because the node stores pointers to its parent and sibling (the leaf node to its right), it doesn't matter where the new node is stored -- if it's the page after its parent or if it's thousands of pages away. This makes it easy to add new nodes and build the tree, because the nodes behave similarly to a linked list; they don't need to be placed in contiguous blocks of memory.

![code snippet](/images/create_new_node.png)

As you can see in the code snippet, we update the relationships between the nodes when we create the new one. We give the new node the same parent as the old node and update the sibling information, so the old node points to the new node, and the new node points to the old node's prior sibling. This snippet is intended to show the interconnectedness of the nodes in the B-tree and explain why operating on a B-tree is so quick -- because you can jump parent-to-child, sibling-to-sibling, based on the information in each node you land on.

Unfortunately, I never fully implemented my B-tree data structure. I didn’t realize until I had finished the tutorial that the tutorial itself was incomplete; the last section, which covered internal node splitting, was never published. I tried to add this feature, which makes it possible for the database to hold more data, but I failed to get a working implementation. As a result, my database has a flat “tree” structure since it can only have one internal node -- the root node. The number of entries it can store is thus limited to the number of entries that can be stored per leaf node and the number of leaf node pointers that can be stored per internal node. Basically, my B-tree is just a horizontal line of leaf nodes.

### Design Decisions
The most crucial design decision was the choice to implement the B-tree. Prior to the B-Tree, data was stored in unrelated pages (a page is a chunk of memory that contains a fixed number of rows). If you wanted to search for a specific row, you would have to cycle through each row in each page until you found it. As a result, database operations had a linear runtime.

The B-tree implementation also uses pages, but each page is represented as a node, and nodes are connected to each other. As I described earlier, searching a B-tree takes logarithmic time, because you only need to search one node at each level of the tree (and the height of the tree is directly related to the amount of data its storing in its leaf nodes). So, using a B-tree can dramatically increase runtime performance.

There is, however, a tradeoff. The internal nodes in a B-tree don’t store actual data; only the leaf nodes store data. Each node takes up one page, so B-trees end up using a lot of extra pages to store the same amount of data as an unstructured set of pages with just raw data. So, there is a time-space tradeoff; B-trees are faster, but they also create larger database files (a SQLite database stores all of its data in a single file). 

Implementing a B-tree made sense because of how databases are used: frequently and, according to users’ expectations, without much delay. Storage space is relatively cheap, but time is invaluable. Since SQLite database files are rarely massive, in most cases, I think the additional space cost of the B-tree will be outweighed by the benefit in program runtime.

### Reflection
I surpassed my project MVP, but I was not able to spend as much time looking at ways to optimize database operations as I had hoped I would. This is partly because I ended up spending time learning about other things, like RSpec testing, that were unexpectedly related to my project. Nonetheless, I did learn about how a B-tree data structure improved the runtime for searching and inserting, and I was able to achieve my goals of learning more about C and relational databases. I had no knowledge of cursor abstraction or pagers prior to this project, and even my existing understanding of caching proved to be less than accurate, so I feel like I have a much better grasp on relational databases than I did before this project. Furthermore, while I had implemented plenty of tree data structures in other classes I took in Python and Java, doing it in C proved to be a lot more difficult and an interesting challenge as I had to consider minutiae like cell sizes and the memory layout of a page when thinking about how I would add or change information in a tree node. It was a different perspective on a familiar concept. Following a tutorial was also pretty neat, because I got to see more complex C code in action and learned some new tricks. I commented aggressively to show that I really understood the code since I did get a lot of it from a tutorial. Overall, while I failed to fully implement a SQLite database, I do think I learned a lot and I’m happy with the work I did!

