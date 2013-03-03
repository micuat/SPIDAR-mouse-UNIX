/*******************************************************
 SPIDAR-mouse-server for Linux, FreeBSD and Mac OS X
 Copyright (c) 2012 Naoto Hieda
 June 11, 2012 
 Licensed under MIT license.
 https://github.com/micuat/SPIDAR-mouse-UNIX
 
 main.cpp
 
 ********************************************************/
#include <iostream>
#include <stdio.h>
#include <wchar.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

// Headers needed for sleeping.
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include "spidarMouse.h"

#define SET_FREE		0
#define SET_CONTINUOUS	1
#define SET_IMPULSE		2

#define PORT			8080

using namespace std;

float Force_XScale,Force_YScale;
char finishFlag = 0, continuousFlag = 0, impulseFlag = 0;
int nowTime = 0, waitTime = 0;
int uDelay = 4; //4ms

spidarMouse sMouse;

void *TimeProcControl(void *);

void *TimeProcControl(void *arg) {
	while( 1 ) {
		if( finishFlag == 1 ) {
			break;
		}
		switch ( continuousFlag ) {
			case SET_IMPULSE:
				continuousFlag = SET_FREE;
				break;
				
			case SET_CONTINUOUS:
				if( nowTime >= waitTime ) {
					Force_XScale = 0;
					Force_YScale = 0;
					continuousFlag = SET_FREE;
				}
				nowTime += uDelay;
				break;
				
			case SET_FREE:
			default:
				Force_XScale = 0;
				Force_YScale = 0;
				break;
		}
		
		sMouse.setForce(Force_XScale, Force_YScale);
		
		usleep(uDelay * 1000);
	}
	return 0;
}

int main(int argc, char* argv[])
{
	int res;
	char rbuf[256];
	int duration;
	int srcSocket, dstSocket;
	int numrcv;
	struct sockaddr_in srcAddr;
	struct sockaddr_in dstAddr;
	int dstAddrSize = sizeof(dstAddr);
	pthread_t pt;

	
#ifdef WIN32
	UNREFERENCED_PARAMETER(argc);
	UNREFERENCED_PARAMETER(argv);
#endif
	
	if( sMouse.init() != 0 ) {
		cout << "cannot find SPIDAR-mouse device" << endl;
		return 1;
	}
	
	if( sMouse.open() != 0 ) {
		cout << "cannot open SPIDAR-mouse connection" << endl;
		return 1;
	}
	
	// create thread for USB communicaton
	pthread_create(&pt, NULL, &TimeProcControl, NULL);
	
	// open socket
	memset(&srcAddr, 0, sizeof(srcAddr));
	srcAddr.sin_port = htons(PORT);
	srcAddr.sin_family = AF_INET;
	srcAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	
	srcSocket = socket(AF_INET, SOCK_STREAM, 0);
	
	bind(srcSocket, (struct sockaddr *)&srcAddr, sizeof(srcAddr));
	listen(srcSocket, 1);
	
	dstSocket = accept(srcSocket, (struct sockaddr *)&dstAddr, (socklen_t *)&dstAddrSize);
	
	while(1) {
		memset(rbuf, 0, sizeof(rbuf));
		numrcv = recv(dstSocket, rbuf, 256, 0);
		if( numrcv <= 0 ) {
			break;
		} else {
			printf("%s\n", rbuf);
		}
		
		// Accept TCP/IP connection from Adobe Flash
		if( strcmp(rbuf, "<policy-file-request/>") == 0 ) {
			strcpy(rbuf, "<?xml version=\"1.0\" encoding=\"UTF-8\"?><cross-domain-policy xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:noNamespaceSchemaLocation=\"http://www.adobe.com/xml/schemas/PolicyFileSocket.xsd\"><allow-access-from domain=\"*\" to-ports=\"*\" secure=\"false\" /><site-control permitted-cross-domain-policies=\"master-only\" /></cross-domain-policy>");
			send(dstSocket, rbuf, strlen(rbuf), 0);
			close(dstSocket);
			dstSocket = accept(srcSocket, (struct sockaddr *)&dstAddr, (socklen_t *)&dstAddrSize);
		}
		
		
		sscanf(rbuf, "%f,%f,%d", &Force_XScale, &Force_YScale, &duration);
		
		if (duration <= 0) {
			continuousFlag = SET_IMPULSE;
		} else {
			continuousFlag = SET_CONTINUOUS;
			nowTime = 0;
			waitTime = duration;
		}
	}
	
	close(dstSocket);
	
	finishFlag = 1;
	
	while( pthread_join(pt, NULL) ) {
		usleep(10000);
	}
	
	sMouse.close();
	
#ifdef WIN32
	system("pause");
#endif
	
	return 0;
}
