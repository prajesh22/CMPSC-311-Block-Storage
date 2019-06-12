////////////////////////////////////////////////////////////////////////////////
//
//  File           : hdd_file_io.h
//  Description    : This is the implementation of the standardized IO functions
//                   for used to access the HDD storage system.
//  Author         : Prateek Chandra
//  PennState Email: pbc5080@psu.edu
//	PennState ID   : 983272256
//	
//	I AM NOT AN HONORS STUDENT AND I DID NOT DO THE BONUS.
//
//  Last Modified  : 11:45AM 10/10/2017
//
////////////////////////////////////////////////////////////////////////////////
//
// STUDENTS MUST ADD COMMENTS BELOW and FILL IN THE CODE FUNCTIONS

// Includes
#include <malloc.h>
#include <string.h>

// Project Includes
#include <hdd_file_io.h>
#include <hdd_driver.h>
#include <cmpsc311_log.h>
#include <cmpsc311_util.h>
#include <hdd_network.h>

// Defines
#define CIO_UNIT_TEST_MAX_WRITE_SIZE 1024
#define HDD_IO_UNIT_TEST_ITERATIONS 10240


// Type for UNIT test interface
typedef enum {
	CIO_UNIT_TEST_READ   = 0,
	CIO_UNIT_TEST_WRITE  = 1,
	CIO_UNIT_TEST_APPEND = 2,
	CIO_UNIT_TEST_SEEK   = 3,
} HDD_UNIT_TEST_TYPE;

char *cio_utest_buffer = NULL;  // Unit test buffer

// Implementation

//global variables
int checkHddInitialize = 0; //checks whether hdd initialize is called 

// Implementation of the metadata of the file
typedef struct {
	HddBlockID blockID;
	uint32_t seek_location;
	char fileName[MAX_FILENAME_LENGTH]; 
	uint8_t checkOpen;	//to check if the file is open or not
	uint8_t checkBlockCreated; // to check if the block is created
	uint32_t blockSize;
} META_DATA;

//global array of structs (maximum number of file is 1024)
META_DATA data_file_struct [1024];

////////////////////////////////////////////////////////////////////////////////
//
// Function     : encodeSubDivisions
// Description  : This function converts the data into 5 seperate fields (Encodes it into different sections)
// Inputs       : 1. blockID : this is where the data is stored
//				  2. resultCode : not sure what this does
//				  3. flags : unused for this assignment (set to 0)
//				  4. blockSize : length of the block user requests to 
//				  5. op : operator (0, 1, 2)
// Outputs      : 64 bit value
//
HddBitCmd encodeSubDivisions (HddBlockID blockID, uint32_t resultCode, uint32_t flags, uint32_t blockSize, uint32_t op)
{
	HddBitCmd returnValue = 0;

	//casting it to 64 bit so that we can operate bit commands
	returnValue = (HddBitCmd)blockID<<0;						//since the blockID is the first, nothing needs to be shifted
	returnValue = returnValue | ((HddBitCmd)resultCode << 32);	//result code comes after 32 bits as blockID is 0-31 bits
	returnValue = returnValue | ((HddBitCmd)flags << 33); 		//result code is just 1 bit long after blockID, thus 33 bits
	returnValue = returnValue | ((HddBitCmd)blockSize << 36);  	// flags is 3 bits long, right after result code, thus 36 bits
	returnValue = returnValue | ((HddBitCmd)op << 62); 			// blockSize is 26 bits long, right after flags, thus 62 bits

	return returnValue;
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : decodeSubDivisions
// Description  : this is an array function which returns the the blockID, resultCode, flags, op, seek_location and fileName
// Inputs       : input is a 64 bit unsigned int
// Outputs      : returns the the blockID, resultCode, flags, op, seek_location and fileName as seperate elements of an array
//

uint32_t *decodeSubDivisions (HddBitResp input)
{
	
	static uint32_t returnValue[5]; 

	//AND the returnValue with the maximum number that the following number of bytes can hold
	returnValue[0] = (uint32_t)((input >> 0) & 4294967295); // 2^(32)-1 	-> recieving 32 bits
	returnValue[1] = (uint8_t) ((input >> 32) & 1); 		// 2^1 - 1 		-> recieving 1 bit
	returnValue[2] = (uint8_t) ((input >> 33) & 7); 		// 2^3 - 1 		-> recieving 3 bits
	returnValue[3] = (uint32_t)((input >> 36) & 67108863);	// 2^(26) -1 	-> recieving 26 bits
	returnValue[4] = (uint8_t) ((input >> 62) & 3); 		// 2^2 - 1 		-> recieving 2 bits
 
	return returnValue;
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : hdd_format
// Description  : sends a format request to the device which will delete all the blocks
//
// Inputs       : nothing
// Outputs      : Returns 0 on success and -1 on failure
//
uint16_t hdd_format(void)
{
	//initialize the device if not already done
	if(checkHddInitialize == 0)
	{
		HddBitCmd initialize;
		HddBitResp initializeResp;
		uint32_t *initializeBuffer;

		//parameters: HddBlockID blockID, uint32_t resultCode, uint32_t flags, uint32_t blockSize, uint32_t op
		initialize = encodeSubDivisions(0, 0, HDD_INIT, 0, HDD_DEVICE);
		initializeResp = hdd_client_operation(initialize, NULL);
		initializeBuffer = decodeSubDivisions (initializeResp);
		//hddInitializeReturnValue = hdd_initialize(); //initialize

		if (initializeBuffer[1]==1) //hdd_initialize returns 1 on success and -1 on failure.
		{
			return -1; //error occured
		}
		else
		{
			checkHddInitialize = 1;
		}
	}
	//declaring variables for formating the device
	HddBitCmd format;																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																								
	HddBitResp formatResponse; 
	uint32_t *formatBuffer;

	//declaring variables for creating the metablock
	HddBitCmd create;
	HddBitResp response;
	uint32_t *metaBlockBuffer; 

	//sends a format request to the device which will delete all the blocks
	//parameters: HddBlockID blockID, uint32_t resultCode, uint32_t flags, uint32_t blockSize, uint32_t op
	format = encodeSubDivisions(0, 0, HDD_FORMAT, 0, HDD_DEVICE);
	formatResponse = hdd_client_operation(format, NULL);
	formatBuffer = decodeSubDivisions (formatResponse);

	//checking if the format was successful
	if (formatBuffer[1]==1)
	{		
		return -1;
	}

	//size of the metablock is the size of the data_file_struct
	int sizeOfMetaBlock = 1024 * sizeof(META_DATA);
	
	//creates the meta-block
	//parameters: HddBlockID blockID, uint32_t resultCode, uint32_t flags, uint32_t blockSize, uint32_t op
	create = encodeSubDivisions(0, 0, HDD_META_BLOCK, sizeOfMetaBlock, HDD_BLOCK_CREATE);
	response = hdd_client_operation(create, (void*)data_file_struct);
	metaBlockBuffer = decodeSubDivisions (response);

	//checking if a new block was created successfully
	if (metaBlockBuffer[1] == 1)	//metaBlockBuffer[1] is the R value
	{
		return -1;
	} 
	return 0;
}


////////////////////////////////////////////////////////////////////////////////
//
// Function     : hdd_mount
// Description  : reads from the meta block to populate global data structure with previously saved values
//
// Inputs       : nothing
// Outputs      : returns 0 on success and -1 on failure
//
uint16_t hdd_mount(void) 
{
	//initialize the device if not already done
	if(checkHddInitialize == 0)
	{
		HddBitCmd initialize;
		HddBitResp initializeResp;
		uint32_t *initializeBuffer;

		//parameters: HddBlockID blockID, uint32_t resultCode, uint32_t flags, uint32_t blockSize, uint32_t op
		initialize = encodeSubDivisions(0, 0, HDD_INIT, 0, HDD_DEVICE);
		initializeResp = hdd_client_operation(initialize, NULL);
		initializeBuffer = decodeSubDivisions (initializeResp);
		//hddInitializeReturnValue = hdd_initialize(); //initialize

		if (initializeBuffer[1]==1) //hdd_initialize returns 1 on success and -1 on failure.
		{
			return -1; //error occured
		}
		else
		{
			checkHddInitialize = 1;
		}
	}
	//declaring variables for reading the device
	HddBitCmd read;																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																								
	HddBitResp readResponse; 
	uint32_t *readBuffer;

	//size of the metablock is the size of the data_file_struct
	int sizeOfMetaBlock = 1024 * sizeof(META_DATA);
	
	//reads from the metablock to populate the global data structure
	//parameters: HddBlockID blockID, uint32_t resultCode, uint32_t flags, uint32_t blockSize, uint32_t op
	read = encodeSubDivisions(0, 0, HDD_META_BLOCK, sizeOfMetaBlock, HDD_BLOCK_READ);
	readResponse = hdd_client_operation(read, (void*)data_file_struct);
	readBuffer = decodeSubDivisions (readResponse);

	//checking for the r value
	if (readBuffer[1] == 1)	//readBuffer[1] is the R value
	{
		return -1;
	} 
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : hdd_unmount
// Description  : saves the current state of the global data structure to the meta block
//				  sends a save and close request to the device which will create the update
// Inputs       : none
// Outputs      : returns 0 on success and -1 on failure
//
uint16_t hdd_unmount(void) 
{
	if (checkHddInitialize == 0)
	{
		return -1;
	}
	else
	{
		//declaring variables for saving the current state to the meta block
		HddBitCmd save;																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																								
		HddBitResp saveResponse; 
		uint32_t *saveBuffer;

		//declaring variables for save and close request
		HddBitCmd saveAndClose;																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																								
		HddBitResp saveAndCloseResponse; 
		uint32_t *saveAndCloseBuffer;

		//size of the metablock is the size of the data_file_struct
		int sizeOfMetaBlock = 1024 * sizeof(META_DATA);

		//reads from the metablock to populate the global data structure
		//parameters: HddBlockID blockID, uint32_t resultCode, uint32_t flags, uint32_t blockSize, uint32_t op
		save = encodeSubDivisions(0, 0, HDD_META_BLOCK, sizeOfMetaBlock, HDD_BLOCK_OVERWRITE);
		saveResponse = hdd_client_operation(save, (void*)data_file_struct);
		saveBuffer = decodeSubDivisions (saveResponse);
		
		//checking the r value
		if (saveBuffer[1] == 1)	//saveBuffer[1] is the R value
		{
			return -1;
		} 

		//parameters: HddBlockID blockID, uint32_t resultCode, uint32_t flags, uint32_t blockSize, uint32_t op
		saveAndClose = encodeSubDivisions(0, 0, HDD_SAVE_AND_CLOSE, 0, HDD_DEVICE);
		saveAndCloseResponse = hdd_client_operation(saveAndClose, NULL);
		saveAndCloseBuffer = decodeSubDivisions (saveAndCloseResponse);
		
		//checking the r value
		if (saveAndCloseBuffer[1] == 1)	//saveAndClose[1] is the R value
		{
			return -1;
		} 
		//unintializing the hdd block
		checkHddInitialize = 0;
		//successful return
		return 0;
	}

}


////////////////////////////////////////////////////////////////////////////////
//
// Function     : int16_t hdd_open (char *path)
// Description  : 1. This call opens a file(i.e., sets any initially needed metadatain your data structure) 
//				  2. Initialize the device here (but only the first time this function is called i.e. singleton pattern).
//				  3. For this assignment, the file can is assumed to be non-existent on the device 
// Inputs       : name of the file in char pointer (AKA String)
// Outputs      : The function returns 1 on success and -1 on failure
//
int16_t hdd_open(char *path) 
{
	int fh = 0; //filehandler which is in the struct which is acting like a counter
	//making sure if the inputs are valid and within the range
	if(strlen(path) < 0 || strlen(path) > MAX_FILENAME_LENGTH )
	{
		return -1;
	}
	else if (checkHddInitialize==0) //singleton pattern (hdd_initialize is called only once)
	{
		HddBitCmd initialize;
		HddBitResp initializeResp;
		uint32_t *initializeBuffer;

		//parameters: HddBlockID blockID, uint32_t resultCode, uint32_t flags, uint32_t blockSize, uint32_t op
		initialize = encodeSubDivisions(0, 0, HDD_INIT, 0, HDD_DEVICE);
		initializeResp = hdd_client_operation(initialize, NULL);
		initializeBuffer = decodeSubDivisions (initializeResp);
		//hddInitializeReturnValue = hdd_initialize(); //initialize

		if (initializeBuffer[1]==1) //hdd_initialize returns 1 on success and -1 on failure.
		{
			return -1; //error occured
		}
		else
		{
			checkHddInitialize = 1;
		}
	}
		
	//using iteration to find the file. Checking if the file exits in the file data table
	while (strcmp(data_file_struct[fh].fileName, path) != 0 && fh < 1024 )
    {
        fh++;
    }
    //if the file handle is 1024, that means the file does not exist, else the file exists
    if (fh != 1024)	//if the file already exists
    {
    	//checking if the file created already opened before before
    	if (data_file_struct[fh].checkOpen ==1)
    	{
    		return fh;
    	}

    	//seek location is set to the begining of the file
    	data_file_struct[fh].seek_location = 0;
    	//setting the file to be opened
    	data_file_struct[fh].checkOpen = 1;
    	return fh;
    }
    else //if the file does not exist
    {
    	//if the file does not exist, we will have to loop through the entire file_struct again...
    	// ...in order to find the available position (this is why fh is set to 0 to loop again)

    	fh = 0;

    	//file does not exist. Therefore checking if file is opened is NOT required

    	//looking for the next space available. AKA file name of next space is blank
    	while (strcmp(data_file_struct[fh].fileName,"") != 0)
    	{
    		fh++;
    	}
    	//setting the filename into the struct
    	strcpy (data_file_struct[fh].fileName, path);
    	//initializing the values in the struct
    	data_file_struct[fh].blockID = 0;
    	data_file_struct[fh].seek_location = 0;
    	data_file_struct[fh].checkOpen = 1;
    	return fh;
    }
	return fh;
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : hdd_close
// Description  : 1. This call closes the file referenced by the file handle
//				  2. Deletes all the contents of the block
// Inputs       : fh : file handle is the input
// Outputs      : the funtion returns -1 on failure and 0 on success
//
int16_t hdd_close(int16_t fh) {

	// checking for fh automatically tests if HDD_initialize() is called as function hdd_open returns fh = -1
	//checks if block is previously created
	if (fh<0||fh>1023||data_file_struct[fh].checkBlockCreated == 0|| data_file_struct[fh].checkOpen == 0 || checkHddInitialize == 0) 
	{
		return -1;
	}
	else
	{
		//DOES NOT DELETE THE BLOCK
		/*
		if (hdd_delete_block(data_file_struct[fh].blockID) == -1)
		{
			return -1;
		}
		*/
		
		//closes the file
		data_file_struct[fh].checkOpen = 0;

	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : hdd_read
// Description  : This call reads a count number of bytes from the current position in the file and places them into the data buffer
// Inputs       : 1. fh (file handle)
//				  2. data   
//				  3. count (number of bytes being read)
// Outputs      : this function returns -1 on failure, and and number of bytes read if succesful
//

int32_t hdd_read(int16_t fh, void * data, int32_t count) 
{
	//checking if inputs are valid
	// checking fh automatically test if HDD_initialize() is called as function hdd_open returns fh = -1
	if (fh < 0 || fh > 1023 || data == NULL || count < 0 || data_file_struct[fh].checkBlockCreated == 0 || data_file_struct[fh].checkOpen == 0|| checkHddInitialize == 0)
	{
		return -1;
	}
	else
	{
		//declaring variables
		int32_t previousBlockSize;
		HddBitCmd readFile;																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																								
		HddBitResp inputResponse; //response of datalane
		uint32_t *decodeResponseBuffer;	//array used to recieve partitioned data
		char *dataBuffer;	//temporary memory
		int no_Bytes;	//store the number of bytes as return value;

		//retrieve the size of the block
		previousBlockSize = data_file_struct[fh].blockSize;

		//allocate the memory for the temporary memory
		dataBuffer = malloc (previousBlockSize);

		//since we have to "read" for both the cases, read the data before the if statements

		//use the read command to transfer the data from the present block to the data buffer
		readFile = encodeSubDivisions(data_file_struct[fh].blockID, 0, 0, previousBlockSize, HDD_BLOCK_READ);
		inputResponse = hdd_client_operation (readFile, dataBuffer);
		decodeResponseBuffer = decodeSubDivisions (inputResponse);

		//checking if a HDD_BLOCK_READ was successful
		if (decodeResponseBuffer[1]==1)
		{		
			free(dataBuffer);
			dataBuffer = NULL;
			return -1;
		}

		// if the bytes in the file that are left is less than the number of bytes that neads to be read
		if (previousBlockSize - data_file_struct[fh].seek_location > count)
		{
			//copying memory from the seek locatiio by "count" bytes
			memcpy(data, &dataBuffer[data_file_struct[fh].seek_location], count);
			// after the "count" bytes are read, increment the seek location by "count"
			data_file_struct[fh].seek_location += count;
			//freeing the memory
			free(dataBuffer);
			dataBuffer = NULL;
			//return value
			no_Bytes = count;

			return no_Bytes;
		}
		// if the bytes in the file that are left is sufficient for the number of bytes that neads to be read
		else
		{
			//
			memcpy(data, &dataBuffer[data_file_struct[fh].seek_location], previousBlockSize - data_file_struct[fh].seek_location);
			//no of bytes that have been written
			no_Bytes = previousBlockSize - data_file_struct[fh].seek_location;
			//setting the seek location back to the size of the block size
			data_file_struct[fh].seek_location = previousBlockSize;
			//freeing the memory
			free(dataBuffer);
			dataBuffer = NULL;

			return no_Bytes;
		}
	} // if/else end (checking parameters of function)
} //function end

////////////////////////////////////////////////////////////////////////////////
//
// Function     : hdd_write
// Description  : This  call  writes  a  count number  of  bytes  at  the  current  position  in  the  file  associated with the file handle fh from the buffer data.  
// 				  The function returns -1 on failure, or the number of written read if successful.
// 				  When number of bytes to written extends beyond the size of the block, a new block of a larger size should be created to hold the file. 
// Inputs       : 1. fh: file handler
//			,	  2. void* data: that data that needs to be written which is similar to a char pointer
//				  3. count: the size of the data input
// Outputs      : 1 on success and -1 on failure
//

int32_t hdd_write(int16_t fh, void *data, int32_t count) 
{
	
	// checking if inputs are valid
	// checking fh automatically test if HDD_initialize() is called as function hdd_open returns fh = -1
	if (fh == -1 || data == NULL || count<0)
	{
		return -1;
	}
	else 
	{
		//if no block exist
		if (data_file_struct[fh].checkBlockCreated == 0) 
		{
			//declaring variables
			char *dataBuffer;
			HddBitCmd create;
			HddBitResp response;
			uint32_t *newFile;

			//allocating memory to the first block
			dataBuffer = malloc(count);
			//copies the data into the databuffer for future manipulation
			memcpy (dataBuffer, data, count);
			//creates a new block
			create = encodeSubDivisions(0, 0, 0, count, HDD_BLOCK_CREATE);
			response = hdd_client_operation(create, dataBuffer);
			newFile = decodeSubDivisions (response);

			//free the buffer
			free (dataBuffer);
			dataBuffer = NULL;

			//checking if a new block was created successfully
			if (newFile[1] == 1)	//newFile[1] is the R value
			{
				return -1;
			} 
			
			//updating the information of the struct
			data_file_struct[fh].blockID = newFile[0]; // setting the block ID
			data_file_struct[fh].checkBlockCreated = 1; // set the block created variable to 1 (which means block is created)
			data_file_struct[fh].seek_location = count; // setting the seek location
			data_file_struct[fh].blockSize = count; //setting the size of the block

			return count;
		}
		else // a block exist
		{	
			//declaring variables
			int32_t previousBlockSize;
			char *dataBuffer;
			HddBitCmd readFile;
			HddBitResp inputResponse;
			uint32_t*decodeResponse;

			//read the size of the data that was stored earlier
			previousBlockSize = data_file_struct[fh].blockSize;

			//initializing where the data buffer is stored
			dataBuffer = malloc(previousBlockSize);

			//use the read command to transfer the data from the present block to the data buffer
			readFile = encodeSubDivisions(data_file_struct[fh].blockID, 0, 0, previousBlockSize, HDD_BLOCK_READ);
			inputResponse = hdd_client_operation (readFile, dataBuffer); //using datalane to retrieve the data
			decodeResponse = decodeSubDivisions (inputResponse); 

			//checking if the read was successful
			if (decodeResponse[1] == 1)
			{
				free(dataBuffer);
				dataBuffer = NULL;
				return -1;
			}

			//declaring variables
			char *newDataBuffer; // new temporary memory
			HddBitResp response;
			HddBitCmd command;
			uint32_t *newArray;

			// the second case where we write at the end of the file
			if(data_file_struct[fh].seek_location + count > previousBlockSize)
			{

				//this allocates new memory (buffer) with a increased size in a new data buffer
				newDataBuffer = malloc(data_file_struct[fh].seek_location + count); 

				//i do memcpy twice because the memory allocation of the previous data is the size of the previous block
				//not sure if we can allocate more memory to the previous allocated memory

				//this copies the previous data into the new buffer
				memcpy (newDataBuffer, dataBuffer, previousBlockSize);
				//this copies the new data into the new buffer memory to the correct position 
				memcpy (&newDataBuffer[data_file_struct[fh].seek_location], data, count);

				//variables to delete the block
				HddBitCmd delete;
				HddBitResp deleteResp;
				uint32_t *deleteBuffer;

				//deletes the old block
				//parameters: HddBlockID blockID, uint32_t resultCode, uint32_t flags, uint32_t blockSize, uint32_t op
				delete = encodeSubDivisions(data_file_struct[fh].blockID, 0, 0, 0, HDD_BLOCK_DELETE);
				deleteResp = hdd_client_operation(delete, NULL);
				deleteBuffer = decodeSubDivisions (deleteResp);

				if (deleteBuffer[1]==1) //hdd_initialize returns 1 on success and -1 on failure.
				{
					return -1; //error occured
				}

				//Creates a new 64bit object with the new size
				command = encodeSubDivisions(0, 0, 0, data_file_struct[fh].seek_location + count, HDD_BLOCK_CREATE);
				response = hdd_client_operation(command, newDataBuffer);
				newArray = decodeSubDivisions (response);

				//frees the memory
				free (dataBuffer);
				dataBuffer = NULL;
				free(newDataBuffer);
				newDataBuffer = NULL;

				//checking if a new block was created successfully
				if (newArray[1] == 1)
				{
					return -1;
				}
				
				//now we have to update the information of the new block
				data_file_struct[fh].blockID = newArray[0];
				//update the size of the block
				data_file_struct[fh].blockSize = data_file_struct[fh].seek_location + count;
			}
			else //the size is unchanged in this case
			{
				// copy the data to the seek_location
				memcpy (&dataBuffer[data_file_struct[fh].seek_location], data, count);

				//now owerwrite the block with the new data
				command = encodeSubDivisions(data_file_struct[fh].blockID, 0, 0, previousBlockSize, HDD_BLOCK_OVERWRITE);
				response = hdd_client_operation(command, dataBuffer);
				newArray = decodeSubDivisions (response);
				
				//free the bytes
				free (dataBuffer);
				dataBuffer = NULL;

				//check if the owerwrite was successful or not
				if (newArray[1] == 1)
				{
					return -1;
				}
			}

			//updating the seek location
			data_file_struct[fh].seek_location += count;

			//return the number of bytes that were written
			return count;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : hdd_seek
// Description  : This call changesthe current seek position of the file associated with the file handle fh to the position loc
// Inputs       : 1. fh: File Handle
//				  2. loc: location the data needs to be "seeked"
// Outputs      : returns -1 on failure, and 0 on success
//
int32_t hdd_seek(int16_t fh, uint32_t loc) 
{
	//retrieve the block size for testing
	int32_t previousBlockSize = data_file_struct[fh].blockSize;

	// this automatically test if HDD_initialize() is called as function hdd_open returns fh = -1
	if (fh == -1 || loc < 0 || loc > previousBlockSize || data_file_struct[fh].checkBlockCreated==0 || previousBlockSize == -1 || data_file_struct[fh].checkOpen==0)
	{
		return -1;
	}
	else
	{
		//sets the postion to the desired location
		data_file_struct[fh].seek_location = loc;
		return 0;
	}
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : hddIOUnitTest
// Description  : Perform a test of the HDD IO implementation
//
// Inputs       : None
// Outputs      : 0 if successful or -1 if failure

int hddIOUnitTest(void) {

	// Local variables
	uint8_t ch;
	int16_t fh, i;
	int32_t cio_utest_length, cio_utest_position, count, bytes, expected;
	char *cio_utest_buffer, *tbuf;
	HDD_UNIT_TEST_TYPE cmd;
	char lstr[1024];

	// Setup some operating buffers, zero out the mirrored file contents
	cio_utest_buffer = malloc(HDD_MAX_BLOCK_SIZE);
	tbuf = malloc(HDD_MAX_BLOCK_SIZE);
	memset(cio_utest_buffer, 0x0, HDD_MAX_BLOCK_SIZE);
	cio_utest_length = 0;
	cio_utest_position = 0;

	// Format and mount the file system
	if (hdd_format() || hdd_mount()) {
		logMessage(LOG_ERROR_LEVEL, "HDD_IO_UNIT_TEST : Failure on format or mount operation.");
		return(-1);
	}

	// Start by opening a file
	fh = hdd_open("temp_file.txt");
	if (fh == -1) {
		logMessage(LOG_ERROR_LEVEL, "HDD_IO_UNIT_TEST : Failure open operation.");
		return(-1);
	}

	// Now do a bunch of operations
	for (i=0; i<HDD_IO_UNIT_TEST_ITERATIONS; i++) {

		// Pick a random command
		if (cio_utest_length == 0) {
			cmd = CIO_UNIT_TEST_WRITE;
		} else {
			cmd = getRandomValue(CIO_UNIT_TEST_READ, CIO_UNIT_TEST_SEEK);
		}
		logMessage(LOG_INFO_LEVEL, "----------");

		// Execute the command
		switch (cmd) {

		case CIO_UNIT_TEST_READ: // read a random set of data
			count = getRandomValue(0, cio_utest_length);
			logMessage(LOG_INFO_LEVEL, "HDD_IO_UNIT_TEST : read %d at position %d", count, cio_utest_position);
			bytes = hdd_read(fh, tbuf, count);
			if (bytes == -1) {
				logMessage(LOG_ERROR_LEVEL, "HDD_IO_UNIT_TEST : Read failure.");
				return(-1);
			}

			// Compare to what we expected
			if (cio_utest_position+count > cio_utest_length) {
				expected = cio_utest_length-cio_utest_position;
			} else {
				expected = count;
			}
			if (bytes != expected) {
				logMessage(LOG_ERROR_LEVEL, "HDD_IO_UNIT_TEST : short/long read of [%d!=%d]", bytes, expected);
				return(-1);
			}
			if ( (bytes > 0) && (memcmp(&cio_utest_buffer[cio_utest_position], tbuf, bytes)) ) {

				bufToString((unsigned char *)tbuf, bytes, (unsigned char *)lstr, 1024 );
				logMessage(LOG_INFO_LEVEL, "CIO_UTEST R: %s", lstr);
				bufToString((unsigned char *)&cio_utest_buffer[cio_utest_position], bytes, (unsigned char *)lstr, 1024 );
				logMessage(LOG_INFO_LEVEL, "CIO_UTEST U: %s", lstr);

				logMessage(LOG_ERROR_LEVEL, "HDD_IO_UNIT_TEST : read data mismatch (%d)", bytes);
				return(-1);
			}
			logMessage(LOG_INFO_LEVEL, "HDD_IO_UNIT_TEST : read %d match", bytes);


			// update the position pointer
			cio_utest_position += bytes;
			break;

		case CIO_UNIT_TEST_APPEND: // Append data onto the end of the file
			// Create random block, check to make sure that the write is not too large
			ch = getRandomValue(0, 0xff);
			count =  getRandomValue(1, CIO_UNIT_TEST_MAX_WRITE_SIZE);
			if (cio_utest_length+count >= HDD_MAX_BLOCK_SIZE) {

				// Log, seek to end of file, create random value
				logMessage(LOG_INFO_LEVEL, "HDD_IO_UNIT_TEST : append of %d bytes [%x]", count, ch);
				logMessage(LOG_INFO_LEVEL, "HDD_IO_UNIT_TEST : seek to position %d", cio_utest_length);
				if (hdd_seek(fh, cio_utest_length)) {
					logMessage(LOG_ERROR_LEVEL, "HDD_IO_UNIT_TEST : seek failed [%d].", cio_utest_length);
					return(-1);
				}
				cio_utest_position = cio_utest_length;
				memset(&cio_utest_buffer[cio_utest_position], ch, count);

				// Now write
				bytes = hdd_write(fh, &cio_utest_buffer[cio_utest_position], count);
				if (bytes != count) {
					logMessage(LOG_ERROR_LEVEL, "HDD_IO_UNIT_TEST : append failed [%d].", count);
					return(-1);
				}
				cio_utest_length = cio_utest_position += bytes;
			}
			break;

		case CIO_UNIT_TEST_WRITE: // Write random block to the file
			ch = getRandomValue(0, 0xff);
			count =  getRandomValue(1, CIO_UNIT_TEST_MAX_WRITE_SIZE);
			// Check to make sure that the write is not too large
			if (cio_utest_length+count < HDD_MAX_BLOCK_SIZE) {
				// Log the write, perform it
				logMessage(LOG_INFO_LEVEL, "HDD_IO_UNIT_TEST : write of %d bytes [%x]", count, ch);
				memset(&cio_utest_buffer[cio_utest_position], ch, count);
				bytes = hdd_write(fh, &cio_utest_buffer[cio_utest_position], count);
				if (bytes!=count) {
					logMessage(LOG_ERROR_LEVEL, "HDD_IO_UNIT_TEST : write failed [%d].", count);
					return(-1);
				}
				cio_utest_position += bytes;
				if (cio_utest_position > cio_utest_length) {
					cio_utest_length = cio_utest_position;
				}
			}
			break;

		case CIO_UNIT_TEST_SEEK:
			count = getRandomValue(0, cio_utest_length);
			logMessage(LOG_INFO_LEVEL, "HDD_IO_UNIT_TEST : seek to position %d", count);
			if (hdd_seek(fh, count)) {
				logMessage(LOG_ERROR_LEVEL, "HDD_IO_UNIT_TEST : seek failed [%d].", count);
				return(-1);
			}
			cio_utest_position = count;
			break;

		default: // This should never happen
			CMPSC_ASSERT0(0, "HDD_IO_UNIT_TEST : illegal test command.");
			break;

		}

	}

	// Close the files and cleanup buffers, assert on failure
	if (hdd_close(fh)) {
		logMessage(LOG_ERROR_LEVEL, "HDD_IO_UNIT_TEST : Failure close close.", fh);
		return(-1);
	}
	free(cio_utest_buffer);
	free(tbuf);

	// Format and mount the file system
	if (hdd_unmount()) {
		logMessage(LOG_ERROR_LEVEL, "HDD_IO_UNIT_TEST : Failure on unmount operation.");
		return(-1);
	}

	// Return successfully
	return(0);
}
