////////////////////////////////////////////////////////////////////////////////
//
//  File          : hdd_client.c
//  Description   : This is the client side of the CRUD communication protocol.
//
//   Author       : Prateek Chandra
//  Last Modified : Thu Oct 30 06:59:59 EDT 2014
//

// Include Files
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <stdint.h>

// Project Include Files
#include <hdd_network.h>
#include <cmpsc311_log.h>
#include <cmpsc311_util.h>
#include <hdd_driver.h>

//Global Variable
int socket_fd = -1;

////////////////////////////////////////////////////////////////////////////////
//
// Function     : hdd_client_operation
// Description  : This the client operation that sends a request to the CRUD
//                server.   It will:
//
//                1) if INIT make a connection to the server
//                2) send any request to the server, returning results
//                3) if CLOSE, will close the connection
//
// Inputs       : cmd - the request opcode for the command
//                buf - the block to be read/written from (READ/WRITE)
// Outputs      : the response structure encoded as needed

HddBitResp hdd_client_operation(HddBitCmd cmd, void *buf) 
{
	//retrieving the flag to decided which operation to do
	uint8_t checkFlag = (uint8_t) ((cmd >> 33) & 7); 
	
	if (checkFlag == HDD_INIT)
	{
		//Step 1 : figure out the address/port to connect to
		struct sockaddr_in caddr;
		char *ip = HDD_DEFAULT_IP;

		caddr.sin_family = AF_INET;
		caddr.sin_port = htons(HDD_DEFAULT_PORT);

		if (inet_aton(ip, &(caddr.sin_addr))==0)
		{
			return(-1);
		}
		//Step 2 : create a socket
		socket_fd = socket(PF_INET, SOCK_STREAM, 0);
		if (socket_fd == -1)
		{
			printf("Error on socket creation [%s]\n", strerror(errno));
			return(-1);
		}
		//Step 3 : connect the socket to the remote server
		if (connect(socket_fd, (const struct sockaddr *)&caddr, sizeof(struct sockaddr)) == -1)
		{
			printf("Error on socket connect [%s]\n", strerror(errno));
			return(-1);
		}
	}

	//Step 4 : read() and write() using the socket
	
	//---------------------------------------send----------------------------------------------------------

	HddBitCmd clientCommand = 0 ; //malloc (sizeof(HddBitCmd));

	clientCommand = htonll64(cmd); 

	int writeCmd = write(socket_fd, &clientCommand, sizeof(HddBitCmd));
	
	//if the number of bytes written is still unequal, its an error
	if ( writeCmd != sizeof(HddBitCmd))
	{
		printf("Error writing network data [%s]\n", strerror(errno));
		return(-1);
	}

	//variables for writing the buffer
	int sizeOfBuffer = (uint32_t)((cmd >> 36) & 67108863);	// 2^(26) -1 -> recieving 26 bits
	int bufferOPtype = (uint8_t) ((cmd >> 62) & 3); 		// 2^2 - 1 	 -> recieving 2 bits
	
	// sends the buffer immediately after the 64-bit value (only on HDD_BLOCK_CREATE and HDD_BLOCK_OWERWRITE)
	if (bufferOPtype == HDD_BLOCK_CREATE|| bufferOPtype == HDD_BLOCK_OVERWRITE)
	{
		int writeBufferCmd = write(socket_fd, buf, sizeOfBuffer);
		
		if ( writeBufferCmd != sizeOfBuffer)
		{
			printf("Error writing network data [%s]\n", strerror(errno));
			return(-1);
		}
	}

	//----------------------------------------recieve--------------------------------------------------------

	HddBitResp clientResponse = 0;

	int readResp = read(socket_fd, &clientResponse, sizeof(HddBitResp));

	if(readResp != sizeof(HddBitResp))
	{
		printf("Error reading network data [%s]\n", strerror(errno));
		return(-1);
	}
	//convert it after recieving (reading) from the server
	HddBitResp clientResponseConverted = ntohll64(clientResponse);
	//free(clientResponse);
	//*clientResponse == NULL; 

	//variables for reading the buffer (Have to use the converted block size and the OP after reading)
	//they should be the same as in the cmd
	//they should be at the same position of the original cmd, this is why the bit operation is the same. 

	int responseSize = (uint32_t)((clientResponseConverted >> 36) & 67108863);	// 2^(26) -1 -> recieving 26 bits
	int responseOP = (uint32_t)((clientResponseConverted >> 62) & 3);			// 2^2 - 1 	 -> recieving 2 bits

	if (responseOP == HDD_BLOCK_READ )
	{
		int readBufferResp = read(socket_fd, buf, responseSize);

		while (responseSize > readBufferResp)
		{
			int newReadBufSize = responseSize - readBufferResp;
			readBufferResp = readBufferResp + read (socket_fd, &((char *)buf)[readBufferResp], newReadBufSize); 
		}

		if(responseSize != readBufferResp)
		{
			printf("Error reading network data [%s]\n", strerror(errno));
			return(-1);
		}
	}

	//Step 5 : Close the socket
	if (checkFlag == HDD_SAVE_AND_CLOSE)
	{
		close (socket_fd);
		socket_fd = -1;
	}

	return clientResponseConverted; 
}


