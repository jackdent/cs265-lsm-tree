# Generator #


```
#!bash

cd generator;

make clean; make;

./generator --help


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
                                                                          
-------------------------------------------------------------------------
                    W O R K L O A D   G E N E R A T O R                  
-------------------------------------------------------------------------
Usage: ./generator
        --puts [number of put operations]
        --puts-batches [number of put batches]
        --gets [number of get operations]
        --gets-skewness [skewness (0-1) of get operations]
        --gets-misses-ratio [empty result queries ratio]
        --ranges [number of range operations]
        --deletes [number of delete operations]
        --external-puts * (flag) store puts in external binary files *
        --seed [random number generator seed]
        --help



```