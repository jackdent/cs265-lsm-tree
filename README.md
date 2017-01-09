```
#!
____  ____       _       _______  ____   ____  _       _______     ______             
|_   ||   _|     / \     |_   __ \|_  _| |_  _|/ \     |_   __ \   |_   _ `.       
  | |__| |      / _ \      | |__) | \ \   / / / _ \      | |__) |    | | `. \     
  |  __  |     / ___ \     |  __ /   \ \ / / / ___ \     |  __ /     | |  | |      
 _| |  | |_  _/ /   \ \_  _| |  \ \_  \ ' /_/ /   \ \_  _| |  \ \_  _| |_.' / 
|____||____||____| |____||____| |___|  \_/|____| |____||____| |___||______.'          
        _..._                                               .----------.  
     .-'_..._''.               .-''-.                      /          /   
   .' .'      '.\            .' .-.  )                    /   ______.'   
  / .'                      / .'  / /         .-''''-.   /   /_           
 . '                       (_/   / /         /  .--.  \ /      '''--.    
 | |                            / /         /  /    '-''___          `.   
 | |                   _       / /         /  /.--.        `'.         |  
 . '                 .' |     . '         /  ' _   \          )        | 
  \ '.          .   .   | /  / /    _.-')/   .' )   | ......-'        /  
   '. `._____.-'/ .'.'| |//.' '  _.'.-'' |   (_.'   / \          _..'`   
     `-.______ /.'.'.-'  //  /.-'_.'      \       '    '------'''        
              ` .'   \_.'/    _.'           `----'                       
                        ( _.-'

```

#CS265 DSL
For the cs265 project we will provide a domain specific language (though much more simple than that of cs165) of tests and expected results. The purpose of this is to allow you as much flexibility in your design and API as possible while still allowing for testing on our part. Every project will support five commands: put, get, range, delete, and load. Each command is explained in greater detail below.
##Put
The *put* command will insert a key-value pair into the LSM tree. There are no duplicates so the repeated put of a key updates the value stored in the tree.
**Syntax**
    p [INT1] [INT2]
The 'p' indicates that this is a put command with key `INT1` and value `INT2`.
**Example:**
    p 10 7  
    p 63 222  
    p 10 5  
First the key 10 is added with value 7. Next, key 63 is added with value 222. The tree now holds two pairs. Finally, the key 10 is updated to have value 5. Note that the tree logically still holds only two pairs. These instructions include only puts therefore no output is expected.
##Get
The *get* command takes a single integer, the *key*, and returns the current *value* associated with that key.
**Syntax**
    g [INT1]
The 'g' indicates that this is a *get* for key `INT1`. The current value should be printed on a single line if the key exists and a blank line printed if the key is not in the tree.
**Example:**
    p 10 7
    p 63 222
    g 10
    g 15
    p 15 5
    g 15
**output:**
    7
    
    5
Here we first put (key:value) 10:7, then 63:222. The next instruction is a get for key 10, so the system outputs 7, the current value of key 10. Next we try to get key 15 but it is not included in the tree yet so a blank line is generated in the output. Next we put 15:5. Finally, we get key 15 again. This time it outputs 5 as the key exists at this point in the instructions.
##Range
The *range* command works in a similar way to get but looks for a sequence of keys rather than a single point.
**Syntax**
    r [INT1] [INT2]
The 'r' indicates that this is a *range* request for all the keys from `INT1` *inclusive* to `INT2` *exclusive*. If the range is completely empty than the output should be a blank line, otherwise the output should be a space delineated list of all of the values that exist in the tree. 
**Example:**
    p 10 7
    p 13 2
    p 17 99
    p 12 22
    r 10 12
    r 10 15
    r 14 17
    r 0 100
**output:**
    7
    7 22 2
    
    7 22 2 99
    
In this example, we first put a number of keys and then collect the values with the range command. The first range prints only the value 7 which is associated with key 10. Key 12's value is not printed because ranges are half open: `[lower_key, higher_key)`. The next range doesn't include key 17 but captures the others, The range `[14,17)` is empty so a blank line is output. Finally a large range captures all of the values.
<span style="color:red">**TF NOTE:** What do we want to do about output order? I'm somewhat of the opinion that the output order should be the same as the key order (so in a range 0 to 100 the value of key 10 comes before the value for key 20) however that may limit the approaches that students can take. So i'm also ok with order being ignored though it will be a little more of a pain for us to check.</span>
##Delete
The *delete* command will remove a single key-value pair from the LSM tree.
**Syntax**
    d [INT1]
The 'd' indicates that this is a delete command for key `INT1` and its associated value (whatever that may be).
**Example:**
    p 10 7
    p 12 5  
    g 10  
    d 10
    g 10
    g 12
**output:**
    7
     
    5
First the two pairs 10:7 and 12:5 are put into the tree. Next, we get key 10 which outputs the value 7. Key 10 is then deleted which generates no output. When we then try to get key 10 a second time we get a blank as the key has been deleted. Finally we get key 12 which is unaffected by the delete and therefore outputs the value 5. 
##Load
The *load* command is included as a way to insert many values into the tree without the need to read and parse individual ascii lines.
**Syntax**
    l "/path/to/file_name"
The 'l' command code is used for *load*. This command takes a single string argument. The string is a relative or absolute path to a binary file of integers that should be loaded into the tree. Note that there is no guarantee that the new values being inserted are absent from the current tree or unrepeated in the file, only that all of the pairs are puts.
**Example:**
    l "~/load_file.bin"
This command will load the key value pairs contained in `load_file.bin` in the home directory of the user running the workload. The layout of the binary file is 
    KEYvalueKEYvalueKEYvalue...
    
There are no other characters in the binary file. The first 4 bytes are the first key, the next 4 bytes are the first value and so on. Thus, a file of 32768 bytes has 4096 key value pairs to be loaded (though there could be fewer than 4096 pairs once loaded due to duplicated keys).
