#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <map>
#include <fstream>
#include <pthread.h>
#include <cstdio>
#include <bits/stdc++.h>
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

void* handleclient(void* arg) {
	int clientsocket = *(int*)arg;
	char line[5000];
	int rsize;
	char *command;
	while(1){
		rsize = recv(clientsocket, line, 5000, 0);
		if(rsize == 0){
			EVP_cleanup();
            ERR_free_strings();
            pthread_exit(0);
			return 0;
		}

		command = strtok(line," ");
		if(strcmp(command, "GET") != 0){
			std::cout << "ERROR 501" << std::endl;
		}
		std::cout << "recieved: " << line << std::endl;
		
			

	}
	return 0;
}



int main(int arc, char** argv) {

	int port = 8080;
	char dir[100] = ".";
	bool usefile = false;
	char file[100] = "";

	for(int i=0; i < arc; i++){
		if(strcmp(argv[i],"-p") == 0){
			port = atoi(argv[i+1]);
			//std::cout << "Got Port" << std::endl;
		}else if (strcmp(argv[i],"-docroot") == 0){
			strcpy(dir,argv[i+1]);			
			//std::cout << "Got docroot" << std::endl;
		}else if (strcmp(argv[i],"-logfile")==0){
			usefile = true;
			strcpy(file,argv[i+1]);
			//std::cout << "Got logfile" << std::endl;
		}
	}

	if(port < 1 || port > 65535){
        std::cout << "Bad Port Number\n";
        exit(1);
	}

	struct stat info;

	if( stat( dir, &info ) != 0 ){
		std::cout << "cannot access the directory " << dir << std::endl;
		return 0;
	}else if( info.st_mode & S_IFDIR ){  // S_ISDIR() doesn't exist on my windows 
		std::cout << dir << " is the directory to be searched"  << std::endl;
	}else{
		std::cout << dir << " is not a directory"  << std::endl;
		return 0;
	}
	//std::cout << "Passed directory check" << std::endl;

	if(usefile){
		std::FILE* myfile = std::fopen(file,"w");
		if(myfile == NULL){
			std::cout << "ERROR opening / creating file"  << std::endl;
        	return 1;
		}
	}

	int sockfd = socket(AF_INET,SOCK_STREAM,0);

    if (sockfd<0) {
        std::cout << "Problem creating socket\n";
        return 1;
    }

	struct sockaddr_in serveraddr, clientaddr;
    serveraddr.sin_family=AF_INET;
	std::cout << "Port: " << port << std::endl;
    serveraddr.sin_port=htons(port);
	serveraddr.sin_addr.s_addr=INADDR_ANY;

	int b = bind(sockfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr));

    if(b<0) {
        std::cout << "Bind error\n";
        return 3;
    }

	listen(sockfd,10);

	while(1){
        int len = sizeof(clientaddr);
        int clientsocket = accept(sockfd, (struct sockaddr*)&clientaddr, (socklen_t*)&len);
        
        pthread_t child;
        pthread_create(&child,NULL,handleclient,&clientsocket);
        pthread_detach(child);
	
    }
	EVP_cleanup();
  	ERR_free_strings();
	
	std::cout << "End" << std::endl ;
	

	return 0;
}
