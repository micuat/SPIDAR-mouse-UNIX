/*******************************************************
 SPIDAR-mouse-server for Linux, FreeBSD and Mac OS X
 Copyright (c) 2012 Naoto Hieda
 June 11, 2012 
 Licensed under MIT license.
 https://github.com/micuat/SPIDAR-mouse-UNIX
 
 main.cpp
 
 ********************************************************/

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

#include "hidapi.h"

// Headers needed for sleeping.
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#define SET_FREE		0
#define SET_CONTINUOUS	1
#define SET_IMPULSE		2

#define PORT			8080

unsigned char *MinForce_Bytes, *MaxForce_Bytes, *Fun_a_Bytes, *Fun_b_Bytes;
float interG_X=0.0;
float interG_Y=0.0;
float SetForce_Mag=0.0;
float SetForce_Dirunit_X=0.0;
float SetForce_Dirunit_Y=0.0;
float Force_XScale, Force_YScale;
float Force_X,Force_Y;
float Max_Force=1.5;
unsigned char *interG_X_Bytes;
unsigned char *interG_Y_Bytes;
unsigned char *Force_Mag_Bytes;
unsigned char *Force_Dirunit_X_Bytes;
unsigned char *Force_Dirunit_Y_Bytes;
char finishFlag = 0, continuousFlag = 0, impulseFlag = 0;
int nowTime = 0, waitTime = 0;
int uDelay = 4; //4ms

hid_device *handle;

void *TimeProcControl(void *);

void *TimeProcControl(void *arg) {
	int res;
	unsigned char buf[256];
	
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
					SetForce_Mag = 0;
					continuousFlag = SET_FREE;
				}
				nowTime += uDelay;
				break;
				
			case SET_FREE:
			default:
				SetForce_Mag = 0;
				break;
		}
		
		memset(buf,0x00,sizeof(buf));
		
		buf[0] = 0;
		buf[1] = 0x97;		 
		interG_X_Bytes = (unsigned char *)&interG_X;
		interG_Y_Bytes = (unsigned char *)&interG_Y;
		Force_Mag_Bytes = (unsigned char *)&SetForce_Mag;
		Force_Dirunit_X_Bytes = (unsigned char *)&SetForce_Dirunit_X;
		Force_Dirunit_Y_Bytes = (unsigned char *)&SetForce_Dirunit_Y;
		
		buf[2] = interG_X_Bytes[0];
		buf[3] = interG_X_Bytes[1];
		buf[4] = interG_X_Bytes[2];
		buf[5] = interG_X_Bytes[3];
		
		buf[6] = interG_Y_Bytes[0];
		buf[7] = interG_Y_Bytes[1];
		buf[8] = interG_Y_Bytes[2];
		buf[9] = interG_Y_Bytes[3];
		
		buf[10] = Force_Mag_Bytes[0] ;
		buf[11] = Force_Mag_Bytes[1] ;
		buf[12] = Force_Mag_Bytes[2] ;
		buf[13] = Force_Mag_Bytes[3] ;
		
		buf[14] = Force_Dirunit_X_Bytes[0] ;
		buf[15] = Force_Dirunit_X_Bytes[1] ;
		buf[16] = Force_Dirunit_X_Bytes[2] ;
		buf[17] = Force_Dirunit_X_Bytes[3] ;
		
		buf[18] = Force_Dirunit_Y_Bytes[0] ;
		buf[19] = Force_Dirunit_Y_Bytes[1] ;
		buf[20] = Force_Dirunit_Y_Bytes[2] ;
		buf[21] = Force_Dirunit_Y_Bytes[3] ;
		res = hid_write(handle, buf, 65);
		if (res < 0) {
			printf("Unable to make feedback: %ls\n", hid_error(handle));
		}
		usleep(uDelay * 1000);
	}
	return 0;
}

int main(int argc, char* argv[])
{
	int res;
	unsigned char buf[256];
	char rbuf[256];
#define MAX_STR 255
	wchar_t wstr[MAX_STR];
	int i;
	float conf_data[4];
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
	
	struct hid_device_info *devs, *cur_dev;
	
	devs = hid_enumerate(0x0, 0x0);
	cur_dev = devs;	
	while (cur_dev) {
		printf("Device Found\n  type: %04hx %04hx\n  path: %s\n  serial_number: %ls", cur_dev->vendor_id, cur_dev->product_id, cur_dev->path, cur_dev->serial_number);
		printf("\n");
		printf("  Manufacturer: %ls\n", cur_dev->manufacturer_string);
		printf("  Product:      %ls\n", cur_dev->product_string);
		printf("  Release:      %hx\n", cur_dev->release_number);
		printf("  Interface:    %d\n",  cur_dev->interface_number);
		printf("\n");
		cur_dev = cur_dev->next;
	}
	hid_free_enumeration(devs);
	
	// Set up the command buffer.
	memset(buf,0x00,sizeof(buf));
	buf[0] = 0x01;
	buf[1] = 0x81;
	
	
	// Open the device using the VID, PID,
	// and optionally the Serial number.
	////handle = hid_open(0x4d8, 0x3f, L"12345");
	handle = hid_open(0x4d8, 0x3f, NULL);
	if (!handle) {
		printf("unable to open device\n");
 		return 1;
	}
	
	// Read the Manufacturer String
	wstr[0] = 0x0000;
	res = hid_get_manufacturer_string(handle, wstr, MAX_STR);
	if (res < 0)
		printf("Unable to read manufacturer string\n");
	printf("Manufacturer String: %ls\n", wstr);
	
	// Read the Product String
	wstr[0] = 0x0000;
	res = hid_get_product_string(handle, wstr, MAX_STR);
	if (res < 0)
		printf("Unable to read product string\n");
	printf("Product String: %ls\n", wstr);
	
	// Read the Serial Number String
	wstr[0] = 0x0000;
	res = hid_get_serial_number_string(handle, wstr, MAX_STR);
	if (res < 0)
		printf("Unable to read serial number string\n");
	printf("Serial Number String: (%d) %ls", wstr[0], wstr);
	printf("\n");
	
	// Read Indexed String 1
	wstr[0] = 0x0000;
	res = hid_get_indexed_string(handle, 1, wstr, MAX_STR);
	if (res < 0)
		printf("Unable to read indexed string 1\n");
	printf("Indexed String 1: %ls\n", wstr);
	
	// Set the hid_read() function to be non-blocking.
	hid_set_nonblocking(handle, 1);
	
	// Try to read from the device. There shoud be no
	// data here, but execution should not block.
	res = hid_read(handle, buf, 17);
	
	// Send a Feature Report to the device
	buf[0] = 0x2;
	buf[1] = 0xa0;
	buf[2] = 0x0a;
	buf[3] = 0x00;
	buf[4] = 0x00;
	res = hid_send_feature_report(handle, buf, 17);
	if (res < 0) {
		printf("Unable to send a feature report.\n");
	}
	
	memset(buf,0,sizeof(buf));
	
	// Read a Feature Report from the device
	buf[0] = 0x2;
	res = hid_get_feature_report(handle, buf, sizeof(buf));
	if (res < 0) {
		printf("Unable to get a feature report.\n");
		printf("%ls\n", hid_error(handle));
	}
	else {
		// Print out the returned buffer.
		printf("Feature Report\n   ");
		for (i = 0; i < res; i++)
			printf("%02hhx ", buf[i]);
		printf("\n");
	}
	
	
	//////////////// Start SPIDAR-mouse
	
	memset(buf,0,sizeof(buf));
	
	buf[0] = 0x00;
	buf[1] = 0x82;
	res = hid_write(handle, buf, 65);
	if (res < 0) {
		printf("Unable to write()\n");
		printf("Error: %ls\n", hid_error(handle));
	}
	
	// Read requested state. hid_read() has been set to be
	// non-blocking by the call to hid_set_nonblocking() above.
	// This loop demonstrates the non-blocking nature of hid_read().
	res = 0;
	while (res == 0) {
		res = hid_read(handle, buf, sizeof(buf));
		if (res == 0)
			printf("waiting...\n");
		if (res < 0)
			printf("Unable to read()\n");
#ifdef WIN32
		Sleep(500);
#else
		usleep(500*1000);
#endif
	}
	
	printf("Data read:\n   ");
	// Print out the returned buffer.
	for (i = 0; i < res; i++)
		printf("%02hhx ", buf[i]);
	printf("\n");
	
	memset(buf,0,sizeof(buf));
	
	conf_data[0] = 0.3;
	conf_data[1] = 1.5;
	conf_data[2] = 47.9867;
	conf_data[3] = 13.1215;
	MinForce_Bytes = (unsigned char *)&conf_data[0];
	MaxForce_Bytes = (unsigned char *)&conf_data[1];
	Fun_a_Bytes = (unsigned char *)&conf_data[2];
	Fun_b_Bytes = (unsigned char *)&conf_data[3];
	
	buf[0] = 0;
	buf[1] = 0x80;
	buf[2] = MinForce_Bytes[0];
	buf[3] = MinForce_Bytes[1];
	buf[4] = MinForce_Bytes[2];
	buf[5] = MinForce_Bytes[3];
	
	buf[6] = MaxForce_Bytes[0];
	buf[7] = MaxForce_Bytes[1];
	buf[8] = MaxForce_Bytes[2];
	buf[9] = MaxForce_Bytes[3];
	
	buf[10] = Fun_a_Bytes[0];
	buf[11] = Fun_a_Bytes[1];
	buf[12] = Fun_a_Bytes[2];
	buf[13] = Fun_a_Bytes[3];
	
	buf[14] = Fun_b_Bytes[0];
	buf[15] = Fun_b_Bytes[1];
	buf[16] = Fun_b_Bytes[2];
	buf[17] = Fun_b_Bytes[3];
	res = hid_write(handle, buf, 65);
	if (res < 0) {
		printf("Unable to start SPIDAR-mouse: %ls\n", hid_error(handle));
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
		numrcv = recv(dstSocket, rbuf, 256, 0);
		if( numrcv <= 0 ) {
			break;
		} else {
			printf("%s\n", rbuf);
		}
		if( strcmp(rbuf, "<policy-file-request/>") == 0 ) {
			strcpy(rbuf, "<?xml version=\"1.0\" encoding=\"UTF-8\"?><cross-domain-policy xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:noNamespaceSchemaLocation=\"http://www.adobe.com/xml/schemas/PolicyFileSocket.xsd\"><allow-access-from domain=\"*\" to-ports=\"*\" secure=\"false\" /><site-control permitted-cross-domain-policies=\"master-only\" /></cross-domain-policy>");
			send(dstSocket, rbuf, strlen(rbuf), 0);
			close(dstSocket);
			dstSocket = accept(srcSocket, (struct sockaddr *)&dstAddr, (socklen_t *)&dstAddrSize);
		}
		
		sscanf(rbuf, "%f,%f,%d", &Force_XScale, &Force_YScale, &duration);
		if( fabsf(Force_XScale) > 1.0 ) Force_XScale /= fabsf(Force_XScale);
		if( fabsf(Force_YScale) > 1.0 ) Force_YScale /= fabsf(Force_YScale);
		
		Force_X = Force_XScale * Max_Force;
		Force_Y = Force_YScale * Max_Force;
		SetForce_Mag = sqrt(Force_X * Force_X + Force_Y * Force_Y);
		if ( SetForce_Mag > 0.0 ) {
			SetForce_Dirunit_X = Force_X / SetForce_Mag;
			SetForce_Dirunit_Y = Force_Y / SetForce_Mag;
		}
		
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
	
	buf[0] = 0;
	buf[1] = 0x99;
	res = hid_write(handle, buf, 65);
	if (res < 0) {
		printf("Unable to close SPIDAR-mouse: %d %ls\n", res, hid_error(handle));
	}
	
	hid_close(handle);
	
	/* Free static HIDAPI objects. */
	hid_exit();
	
#ifdef WIN32
	system("pause");
#endif
	
	return 0;
}
