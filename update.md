# DIY SQLite Update
By Mary Keenan

## Project Goal
The goal of this project is to make my own version of SQLite, a relational database. My MVP will be a database that can save data to the disk and fetch it. A stretch goal will be to investigate more efficient ways of storing and fetching the data.

## Learning Goals
My learning goal is to practice C and learn more about how relational databases work. For example, I'm curious about how databases are structured to optimize insert/lookup/etc operations.

## Progress
I have been following this tutorial, https://cstack.github.io/db_tutorial/, to reach my MVP. I have completed the first 8 of 13 parts of the tutorial. Technically, I have reached my MVP as my current code can persist and fetch data, but I'm continuing to follow the tutorial as it will help my optimize my database by implementing a B-Tree as the data structure. I have not needed any other resources as of yet (other than the occasional Stack Overflow lookup!).

## Next Steps
1. I am currently implementing binary searching in my DB by working through Part 9 of the tutorial.
   - **Deliverable**: As I have for each of the tutorial parts so far, I will push a commit with the name of the tutorial section (Part 9 - Binary Search and Duplicate Keys). The commit will contain the changes to my DB code made while following Part 9 of the tutorial, namely, the addition of binary searching.
2. Once I have completed the task above, I am going to implement leaf node splitting in my B-Tree implementation so my DB can grow as more data is added. I will do this by following Part 10 of the tutorial (Part 10 - Splitting a Leaf Node).
   - **Deliverable**: I will push a commit with the name of the tutorial section that contains the changes I made (adding leaf splitting) to my DB code while following Part 10 of the tutorial. 
3. Then, I will implement a recursive search algorithm for my DB to improve performance while following Part 11 of the tutorial (Part 11 - Recursively Searching the B-Tree).
   - **Deliverable**: I will push a commit with the name of the tutorial section that contains the changes I made (adding recursive searching) to my DB code while following Part 11 of the tutorial. 

I will also implement Part 12 and Part 13 of the tutorial to finish it. At this point, I will have an MVP+. Time permitting, I will start researching other ways to optimize DB operations.