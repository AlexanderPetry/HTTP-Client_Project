#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <arpa/inet.h>

#pragma comment(lib, "ws2_32.lib")

#define API_HOST "ip-api.com"
#define API_PATH "/json/%s"

//gcc IP-API_test.c -l ws2_32 -o test.exe


char* get_location_from_ip(const char* ip_address) {
  char buffer[1024];
  SOCKET sockfd;
  struct sockaddr_in serv_addr;
  int portno, n;

  WSADATA wsaData;
  if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
    printf("error: could not initialize Winsock\n");
    return NULL;
  }

  portno = 80;

  sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sockfd == INVALID_SOCKET) {
    printf("error opening socket: %d\n", WSAGetLastError());
    WSACleanup();
    return NULL;
  }

  struct in_addr ip_addr;
  if (inet_pton(AF_INET, ip_address, &ip_addr) != 1) {
    printf("error: invalid IP address\n");
    closesocket(sockfd);
    WSACleanup();
    return NULL;
  }

  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = ip_addr.s_addr;
  serv_addr.sin_port = htons(portno);

  if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
    printf("error connecting: %d\n", WSAGetLastError());
    closesocket(sockfd);
    WSACleanup();
    return NULL;
  }

  snprintf(buffer, sizeof(buffer), "GET " API_PATH " HTTP/1.1\r\nHost: " API_HOST "\r\nConnection: close\r\n\r\n", ip_address);

  n = send(sockfd, buffer, strlen(buffer), 0);
  if (n < 0) {
    printf("error writing to socket: %d\n", WSAGetLastError());
    closesocket(sockfd);
    WSACleanup();
    return NULL;
  }

  int content_length = -1;
  char* city = NULL;
  char* region_name = NULL;
  char* country = NULL;

  while ((n = recv(sockfd, buffer, sizeof(buffer), 0)) > 0) {
    char* p = buffer;
    while ((p = strstr(p, "\r\n")) != NULL) {
      *p = '\0';
      if (strncmp(buffer, "Content-Length:", 15) == 0) {
        content_length = atoi(buffer + 15);
      } else if (strncmp(buffer, "City:", 5) == 0) {
        city = strdup(buffer + 5);
      } else if (strncmp(buffer, "RegionName:", 11) == 0) {
        region_name = strdup(buffer + 11);
      } else if (strncmp(buffer, "Country:", 8) == 0) {
        country = strdup(buffer + 8);
      }
      p += 2;
      buffer[0] = '\0';
    }
  }

  closesocket(sockfd);
  WSACleanup();

  if (content_length < 0 || city == NULL || region_name == NULL || country == NULL) {
       printf("error: could not parse response\n");
    free(city);
    free(region_name);
    free(country);
    return NULL;
  }

  char* location = (char*) malloc(strlen(city) + strlen(region_name) + strlen(country) + 3);
  sprintf(location, "%s, %s, %s", city, region_name, country);
  free(city);
  free(region_name);
  free(country);
  return location;
}

int main(int argc, char** argv) {
  if (argc < 2) {
    printf("usage: %s <ip_address>\n", argv[0]);
    return 1;
  }

  char* location = get_location_from_ip(argv[1]);

  if (location) {
    printf("%s\n", location);
    free(location);
    return 0;
  } else {
    return 1;
  }
}
