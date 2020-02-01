/*test machine: CSELAB_KH-1250-17
* date: 12/05/2019
* name: Ansh Sikka
* x500: sikka008
*/

# Socket Programming MapReduce
Implementation of MapReduce, a tool/algorithm that uses multiple processes large datasets and synchronize output of key-value pairs. Uses producer-consumer multithreading model.
## Purpose
The purpose of this project is to keep large datasets in sync by using mapper and reducer processes. The mapper process
is done on the client side and data is sent through sockets to the reducer process on the multithreaded server side.

## How to compile

For the client: 
- Make sure you have the Testcase folder, the number of mappers, the IP address, and the port.

Before executing all recipes, do this: 
`make clean`
`make`

Then,
`./client <Folder Path> <Number of Mappers> <IP Address> <Port>`

For the server: 
- Make sure you have the port the server is listening on

Before executing all recipes, do this: 
`make clean`
`make`

Then,
`./server <Port>`




## Assumptions
* Can use atoi function from standard string library
* Can utilize some global variables
* Can use switch statement

## Team Names and x500 + Contributions
Ansh Sikka -sikka008

I did have a partner: Jacob Isler: isle0011
We did work on the project individually. However we did use each other to
bounce off each others ideas on how to move forward with the product. There 
was little to no overlap of code. The general strategies were similar between the projects.

## Extra Credit
Extra credit was attempted and it does work ON MY MACHINE. Somehow on the CSELABMACHINES it doesn't properly work (server data is reset?)
 

