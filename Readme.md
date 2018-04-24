## B-trees

#### Goal 
To insert and search data on disk using B-trees.
To investigate how changing the order of a B-tree affects its performance.

#### User Interface
The user will communicate with your program through a set of commands typed at the keyboard.

add k
Add a new integer key with value k to index.bin.
find k
Find an entry with a key value of k in index.bin, if it exists.
print
Print the contents and the structure of the B-tree on-screen.
end
Update the root node's offset at the front of the index.bin, and close the index file, and end the program

#### Program Execution

`
cc main.c
`

`
./a.out index_file order
`

Example: 

`./a.out index.bin 4`
