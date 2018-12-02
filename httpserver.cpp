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
#include <ctime>
#include <cstring>

struct info{
	int clientsocket;
	char *dir;
	bool usefile;
	char *wfile;
};

void* handleclient(void* arg) {
	struct info arginfo = *(struct info*)arg;
	int clientsocket = arginfo.clientsocket;
	char *dir = arginfo.dir;
	bool usefile = arginfo.usefile;
	char *wfile = arginfo.wfile;
	
	char line[5000];
	
	int rsize;
	int counter;
	const char *head;
    	const char *tail;

	std::string ext;

	bool e501;

	std::string parse;
	std::string type;
	std::string file;
	std::string connection;
	std::string version;


	std::string sendb;
	char message[18];

	time_t rawtime;
	struct tm * timeinfo;
  	char date[80];
	
	struct stat result;

	while(1){
		e501 = false;
		//usleep(500);
		rsize = recv(clientsocket, line, 5000, 0);
		std::cout << "------------------\n" << std::endl;

		if(rsize == 0){
            		pthread_exit(0);
			return 0;
		}

		head = line;
		tail = line;
		
		counter = 0;
		while(*tail != '\0'){
			while (*tail != '\0' && *tail != ' ' && *tail != '\n' ){
				parse = std::string(head, tail+1);
				++tail;
			}

			if(counter==0){
				type = parse;
				if(strcmp(type.c_str(),"GET")!=0){
					std::cout << "ERROR 501" << std::endl;
					e501 = true;
				}
				counter++;	
				std::cout<<"TYPE: " << type <<std::endl;
			}else if(counter==1){
				file = parse;
				counter++;		
				std::cout<<"FILE: " << file <<std::endl;	
			}else if(counter==2){
				version = parse;
				counter++;		
				std::cout<<"VERSION: " << version <<std::endl;
			}else if(counter==24){ 
				connection = parse; // 15 in data comms on 11/27
				counter++;
				std::cout<<"CONNECTION: " << connection <<std::endl;
			}else{
				counter++;
			}

			//std::cout<<parse<<std::endl;
			tail++;		
			head=tail;
		}
		connection = "keep-alive";
		
		ext = file.substr(file.find("."),file.size());
		std::cout<<"FILE EXT: " << ext <<std::endl;
		if(ext == ".html"){
			ext = "text/html; charset=utf-8";
		}else if(ext == ".txt"){
			ext = "text/plain; charset=utf-8";
		}else if(ext == ".jpg"){
			ext = "image/jpeg";
		}else if(ext == ".pdf"){
			ext = "application/pdf";
		}else if(ext == "ico"){
			continue;
		}else{
			std::cout << "ERROR 501" << std::endl;
			e501 = true;
		}

		time (&rawtime);
  		timeinfo = localtime(&rawtime);
		strftime(date,sizeof(date),"%d-%m-%Y %H:%M:%S",timeinfo);
		std::cout<<"DATE: " << date <<std::endl;

		if(e501){
			sendb = "";
			strcpy(message,"<h1>501 Error</h1>");
			
			sendb = version + " 501 Not Implemented\nDate: " + date + "\nContent-Type: text/html; charset=utf-8\nContent-Length: 18\n\n" + message;  
			send(clientsocket,sendb.c_str(), sendb.length(), 0);
			break;
		}

		char fname[50];
		strcpy(fname,file.c_str());
		std::FILE* myfile; 
		char fpath[20] = "";
		strcpy(fpath,dir);
		
		if((myfile = std::fopen(strcat(fpath,fname),"r")) == NULL){
			sendb = "";
			strcpy(message,"<h1>404 Error</h1>");
			
			sendb = version + " 404 Not Found\nDate: " + date + "\nContent-Type: text/html; charset=utf-8\nContent-Length: 18\n\n" + message;  
			send(clientsocket,sendb.c_str(), sendb.length(), 0);
			break;
		}
		
		std::cout << "\nFULLPATH: " << fpath << std::endl;

		time_t mod_time;
		if(stat(fpath, &result)==0){
			mod_time = result.st_mtime;
		}

		char* lmodtime = std::ctime(&mod_time);
		std::cout<<"MODTIME: " << lmodtime << std::endl;
		
		sendb = "";
		strcpy(message, "OK");

		std::fseek(myfile, 0, SEEK_END);
		long filesize = std::ftell(myfile);
		rewind(myfile);

		char *body = (char *)malloc(filesize);

		size_t read = fread(body, 1, filesize, myfile);

		char str[256];
		//usleep(250);
		snprintf(str, sizeof str, "%zu", filesize);

		sendb = version + " 200 " + message + "\n"
			+ "Connection: " + connection + "\n"
			+ "Content-Type: " + ext + "\n" 
			+ "Content-Length: " + str  + "\n"
			+ "Date: " + date + "\n"
			+ "Last-Modified: " + lmodtime + "\n\n"
			+ body;

		std::cout << "Total Size: " << sendb.length() << std::endl;
		std::cout << "Body Size: " << str << std::endl;

		send(clientsocket, sendb.c_str(), (sendb.length() + filesize - 4), 0);

		std::cout << "File Sent" << std::endl;
		free(body);
		fclose(myfile);
		pthread_exit(0);
		//break;
		// SEND CODE 200 With FIle Contents
		// WRITE TO TXT FILE / STDOUT
		// 304 ERROR CODE
		// WHEN SENDING JPG, sendb.length() not right size
	}
	//close(clientsocket);
	pthread_exit(0);
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

	listen(sockfd,20);

	struct info clientinfo;
	clientinfo.dir = dir;
	clientinfo.usefile = usefile;
	clientinfo.wfile = file;  

	while(1){
		
        int len = sizeof(clientaddr);
        int clientsocket = accept(sockfd, (struct sockaddr*)&clientaddr, (socklen_t*)&len);
        
		clientinfo.clientsocket = clientsocket; 

        pthread_t child;
        pthread_create(&child,NULL,handleclient,&clientinfo);
        pthread_detach(child);
	
    }
	
	std::cout << "End" << std::endl ;
	

	return 0;
}
