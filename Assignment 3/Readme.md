# Assignment 3 Banking system using socket_programming

Compilation Instructions:
  server : gcc server.c -o server
  client : gcc client.c -o client

To create login_file:
	gcc add_new_user.c -o adding
	./adding
	Add new users by following instructions on screen

To start server
   Execute the server and pass a desired port no(not used by any well known apllication) as argument in place of "port_no" below
   ./server port_no
   
To start client
    Execute the client and pass server's ip address and server's port no respectively. For running on local machine use 127.0.0.1 as IP address.
    ./client server_ip server_port_no
    