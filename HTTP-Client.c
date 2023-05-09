#ifdef _WIN32
	#define _WIN32_WINNT _WIN32_WINNT_WIN7
	#include <winsock2.h> //for all socket programming
	#include <ws2tcpip.h> //for getaddrinfo, inet_pton, inet_ntop
	#include <stdio.h> //for fprintf, perror
	#include <unistd.h> //for close
	#include <stdlib.h> //for exit
	#include <string.h> //for memset
	void OSInit( void )
	{
		WSADATA wsaData;
		int WSAError = WSAStartup( MAKEWORD( 2, 0 ), &wsaData ); 
		if( WSAError != 0 )
		{
			fprintf( stderr, "WSAStartup errno = %d\n", WSAError );
			exit( -1 );
		}
	}
	void OSCleanup( void )
	{
		WSACleanup();
	}
	#define perror(string) fprintf( stderr, string ": WSA errno = %d\n", WSAGetLastError() )
#else
	#include <sys/socket.h> //for sockaddr, socket, socket
	#include <sys/types.h> //for size_t
	#include <netdb.h> //for getaddrinfo
	#include <netinet/in.h> //for sockaddr_in
	#include <arpa/inet.h> //for htons, htonl, inet_pton, inet_ntop
	#include <errno.h> //for errno
	#include <stdio.h> //for fprintf, perror
	#include <unistd.h> //for close
	#include <stdlib.h> //for exit
	#include <string.h> //for memset
	void OSInit( void ) {}
	void OSCleanup( void ) {}
#endif

// ******************* //
// additional librarys for windows// --> also need to work in linux --> RPI3
// ******************* //

#include <string.h>
#include <pthread.h>
//#include "curl/curl.h" 		// --> install library
#include <jansson.h>	// --> install library

// *************** //
// constant values //
// *************** //

#define API_URL "http://ip-api.com/json/"
#define filename "output.csv"

// **************** //
// global variables // 
// **************** //

char IP_LOOKUP[1000];
int fileBusy = 0;

// ******************** //
// initialize functions //
// ******************** //

int initialization();
int connection( int internet_socket );
void execution( int internet_socket );
void cleanup( int internet_socket, int client_internet_socket );
static size_t writeMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp);
char** getLocationData(char* ipAddress);
void delay(int number_of_seconds);

// ********************* //
// additional data types //
// ********************* //

typedef struct {
    char *data;
    size_t size;
} MemoryChunk;

int main( int argc, char * argv[] ) // should keep running for a long time ** at least multiple days ** 
{

	// read lookup data and put it in ARRAY

	OSInit();

	// limit max request to 45/min


	// make thread for every connection
		int internet_socket = initialization();

		int client_internet_socket = connection( internet_socket );

		execution( client_internet_socket );

		cleanup( internet_socket, client_internet_socket );
		// close thread if connection has been closed and data has been written

	OSCleanup();

	return 0;
}

int initialization()
{
	//Step 1.1
	struct addrinfo internet_address_setup;
	struct addrinfo * internet_address_result;
	memset( &internet_address_setup, 0, sizeof internet_address_setup );
	internet_address_setup.ai_family = AF_UNSPEC;
	internet_address_setup.ai_socktype = SOCK_STREAM;
	internet_address_setup.ai_flags = AI_PASSIVE;
	int getaddrinfo_return = getaddrinfo( NULL, "22", &internet_address_setup, &internet_address_result );
	if( getaddrinfo_return != 0 )
	{
		fprintf( stderr, "getaddrinfo: %s\n", gai_strerror( getaddrinfo_return ) );
		exit( 1 );
	}

	int internet_socket = -1;
	struct addrinfo * internet_address_result_iterator = internet_address_result;
	while( internet_address_result_iterator != NULL )
	{
		//Step 1.2
		internet_socket = socket( internet_address_result_iterator->ai_family, internet_address_result_iterator->ai_socktype, internet_address_result_iterator->ai_protocol );
		if( internet_socket == -1 )
		{
			perror( "socket" );
		}
		else
		{
			//Step 1.3
			int bind_return = bind( internet_socket, internet_address_result_iterator->ai_addr, internet_address_result_iterator->ai_addrlen );
			if( bind_return == -1 )
			{
				perror( "bind" );
				close( internet_socket );
			}
			else
			{
				//Step 1.4
				int listen_return = listen( internet_socket, 1 );
				if( listen_return == -1 )
				{
					close( internet_socket );
					perror( "listen" );
				}
				else
				{
					break;
				}
			}
		}
		internet_address_result_iterator = internet_address_result_iterator->ai_next;
	}

	freeaddrinfo( internet_address_result );

	if( internet_socket == -1 )
	{
		fprintf( stderr, "socket: no valid socket address found\n" );
		exit( 2 );
	}

	return internet_socket;
}

int connection( int internet_socket )
{
	//Step 2.1
	struct sockaddr_storage client_internet_address;
	socklen_t client_internet_address_length = sizeof client_internet_address;
	int client_socket = accept( internet_socket, (struct sockaddr *) &client_internet_address, &client_internet_address_length );
	if( client_socket == -1 )
	{
		perror( "accept" );
		close( internet_socket );
		exit( 3 );
	}
	return client_socket;
}

void execution( int internet_socket )
{
	// get current time
	time_t seconds;
    seconds = time(NULL);

	// get IP adress


	// ************************ //
	// Get info via geolocation //
	// ************************ //

	char outputText[1000];

	char** locationData = getLocationData(client_internet_address);					// get location data

    if (locationData == NULL) 														// check if locationData is correct
    {
        fprintf(stderr, "Failed to retrieve location data\n");						// send error
    }


	// ***************************** //
	// keep sending data to attacker //
	// ***************************** //

	int number_of_bytes_send = 0;
	int total_bytes_send = 0;
	char message[1000] = "(";

	strcat(message, locationData[6]);
	strcat(message, ";");
	strcat(message, locationData[7]);
	strcat(message, ")");


	while(number_of_bytes_send > 0) 												// loop until error
	{
		number_of_bytes_send = send( internet_socket, message, 16, 0 );				// send data

		total_bytes_send += number_of_bytes_send;									// add current bytes send to total bytes send
	}

	
    // ********************************* //
	// print info to buffer then to file //
	// ********************************* //

	outputText = dataConverter(locationData, total_bytes_send, seconds);			// convert data to readable format

	while(fileBusy == 1) 															// wait for file to be not busy
	{
		delay(1); 																	// delay 1 second before trying again
	}
	writeToFile(outputText); 														// print buffer in output file
}

int writeToFile(char* buffer[1000])
{
	fileBusy = 1; 																// file = busy
	FILE *fp;
	fp = fopen (filename, "a");													// open file in append mode
	fprintf(fp, buffer);														// append buffer to file
    fclose(fp);																	// close file
    fileBusy = 0; 																// file = not busy
}

void cleanup( int internet_socket, int client_internet_socket )
{
	//Step 4.2
	int shutdown_return = shutdown( client_internet_socket, SD_RECEIVE );
	if( shutdown_return == -1 )
	{
		perror( "shutdown" );
	}

	//Step 4.1
	close( client_internet_socket );
	close( internet_socket );
}

static size_t writeMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) 
{
    size_t realSize = size * nmemb;
    MemoryChunk *chunk = (MemoryChunk *)userp;
    char *p = realloc(chunk->data, chunk->size + realSize + 1);
    if (p == NULL) {
        fprintf(stderr, "Failed to allocate memory\n");
        return 0;
    }
    chunk->data = p;
    memcpy(&(chunk->data[chunk->size]), contents, realSize);
    chunk->size += realSize;
    chunk->data[chunk->size] = '\0';
    return realSize;
}

char** getLocationData(char* ipAddress) 
{
	// get location data

    char** locationData = (char**) malloc(4 * sizeof(char*));
    locationData[0] = strdup(json_string_value(json_object_get(root, "country"		)));
    locationData[1] = strdup(json_string_value(json_object_get(root, "regionName"	)));
    locationData[2] = strdup(json_string_value(json_object_get(root, "city"			)));
    locationData[3] = strdup(json_string_value(json_object_get(root, "zip"			)));
    locationData[4] = strdup(json_string_value(json_object_get(root, "query"		)));
    locationData[5] = strdup(json_string_value(json_object_get(root, "org"			)));
    locationData[6] = strdup(json_string_value(json_object_get(root, "lat"			)));
    locationData[7] = strdup(json_string_value(json_object_get(root, "lon"			)));

    json_decref(root);
    free(chunk.data);

    return locationData;
}

void delay(int number_of_seconds)
{   
    int milli_seconds = 1000 * number_of_seconds;		// Converting time into milli_seconds 

    clock_t start_time = clock();						// Storing start time

    while (clock() < start_time + milli_seconds){;}		// looping till required time is not achieved
}

char dataConverter(char** locationData, int dataSent, time_t time) 					// **TODO** add readAttempt variable to func **TODO**
{
																					// format: Time;IP;DataAmount;Country;region;City;ZIP;Org;RepeatAttempt -> CSV file
	char temp[100];
	char output[100];

	strftime(temp, 1000, "%Y-%m-%d %H:%M:%S", localtime(&time)); 					// convert int seconds to "Y-M-D-H-M-S"
	char data_time = strcat(time_string, ";");										// convert "Y-M-D-H-M-S" to "Y-M-D-H-M-S;"

	char data_ip = strcat(locationData[4], ";"); 									// convert "IP" to "IP;"

	sprintf(temp, "%i", checkRepeatAttempt(locationData[4]));						// convert int repeatAttempt to "bool"
	char data_repeat = strcat(temp, ";\n");											// convert "bool" to "bool;"

	strcat(output, data_time);														// add time to output string
	strcat(output, data_ip);														// add ip to output string
	// ...
	strcat(output, data_repeat);													// add repeatAttempt to output string + \n to go to next line
}

int checkRepeatAttempt(char ip)
{
	int ip_size = 0;																// **TODO** fill in ip size **TODO**
	int cntr = 0;																	// counter to count each IP in lookup table

	while(true)
	{
		temp[100]; 																	// temporary holds each value of lookup table

		if(IP_LOOKUP[cntr] == NULL)													// if lookup is empty return "no match found" = 0
		{
			return 0;
			break;
		}

		for (int i = 0 ; i != ip_size ; i++)										// loop over every character
		{
        	temp = strcat(temp, IP_LOOKUP[cntr * ip_size + i]) 						// add character to temp string
    	}

    	if(strcmp(temp,ip) == 0) 													// if item in list == ip return "match found" = 1
    	{
    		return 1;
    		break;
    	}
		

	}
}