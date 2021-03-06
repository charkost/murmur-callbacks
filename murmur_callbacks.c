/*
* Murmur's ICE userConnected/userDisconnected callbacks-notifications adder and listener.
* Works only with a Murmur that supports ICE's version >=3.4 and runs at localhost.
* Charalampos Kostas <root@charkost.gr>
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define LISTEN_PORT 65535
#define MURMUR_PORT 6502
#define ICE_ISA_REPLY_PACKET_SIZE 26
#define ADDCALLBACK_REPLY_PACKET_SIZE 25
#define VALIDATE_CONNECTION_PACKET_SIZE 14
#define READ_BUFFER_SIZE 5000

int callbacks_add(unsigned char *); /* Adds callbacks */
int callbacks_listen(uint16_t); /* Listens for callbacks */

int main(void)
{
	int r;
	uint16_t listener_port = htons(LISTEN_PORT);
	unsigned char *listener_port_bytes = (unsigned char *)&listener_port;

	if ((r = callbacks_add(listener_port_bytes)) > 0)
		return r;

	return callbacks_listen(listener_port);
}

int callbacks_add(unsigned char *listener_port_bytes)
{
	const unsigned char ice_isA_packet[] =
	{
		0x49, 0x63, 0x65, 0x50, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x38, 0x00, 0x00, 0x00, 0x01, 0x00,
		0x00, 0x00, 0x04, 0x4d, 0x65, 0x74, 0x61, 0x00, 0x00, 0x07, 0x69, 0x63, 0x65, 0x5f, 0x69, 0x73,
		0x41, 0x01, 0x00, 0x15, 0x00, 0x00, 0x00, 0x01, 0x00, 0x0e, 0x3a, 0x3a, 0x4d, 0x75, 0x72, 0x6d,
		0x75, 0x72, 0x3a, 0x3a, 0x4d, 0x65, 0x74, 0x61
	};
	const unsigned char addCallback_packet[] =
	{
		0x49, 0x63, 0x65, 0x50, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x70, 0x00, 0x00, 0x00, 0x02, 0x00,
		0x00, 0x00, 0x01, 0x31, 0x01, 0x73, 0x00, 0x0b, 0x61, 0x64, 0x64, 0x43, 0x61, 0x6c, 0x6c, 0x62,
		0x61, 0x63, 0x6b, 0x00, 0x00, 0x4b, 0x00, 0x00, 0x00, 0x01, 0x00, 0x24, 0x34, 0x45, 0x35, 0x42,
		0x38, 0x42, 0x31, 0x37, 0x2d, 0x44, 0x43, 0x33, 0x38, 0x2d, 0x34, 0x31, 0x42, 0x38, 0x2d, 0x39,
		0x45, 0x30, 0x45, 0x2d, 0x35, 0x44, 0x34, 0x41, 0x43, 0x43, 0x30, 0x42, 0x30, 0x37, 0x46, 0x46,
		0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x19, 0x00, 0x00, 0x00, 0x01, 0x00, 0x09, 0x31, 0x32,
		0x37, 0x2e, 0x30, 0x2e, 0x30, 0x2e, 0x31, listener_port_bytes[1], listener_port_bytes[0], 0x00,
		0x00, 0xff, 0xff, 0xff, 0xff, 0x00

	};
	unsigned char read_buffer[READ_BUFFER_SIZE];
	struct sockaddr_in serv_addr;
	int adder_sockfd = 0;

	printf("Creating callbacks... ");
	fflush(stdout);

	if ((adder_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		fprintf(stderr, "\nError: Could not create socket\n");
		return 1;
	}

	memset(&serv_addr, '0', sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(MURMUR_PORT);
	serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	if (connect(adder_sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
		fprintf(stderr, "\nError: Failed to connect to Murmur server\n");
		return 1;
	}
	if (read(adder_sockfd, read_buffer, READ_BUFFER_SIZE) != VALIDATE_CONNECTION_PACKET_SIZE) {
		fprintf(stderr, "\nError: Failed to receive validate_packet\n");
		return 1;
	}
	if (write(adder_sockfd, ice_isA_packet, sizeof(ice_isA_packet)) < 0) {
		fprintf(stderr, "\nError: Failed to send ice_isA_packet\n");
		return 1;
	}
	if (read(adder_sockfd, read_buffer, READ_BUFFER_SIZE) != ICE_ISA_REPLY_PACKET_SIZE) {
		fprintf(stderr, "\nError: Failed to receive ice_isA_packet success reply\n");
		return 1;
	}

	if (write(adder_sockfd, addCallback_packet, sizeof(addCallback_packet)) < 0) {
		fprintf(stderr, "\nError: Failed to send addCallback_packet\n");
		return 1;
	}

	if (read(adder_sockfd, read_buffer, READ_BUFFER_SIZE) != ADDCALLBACK_REPLY_PACKET_SIZE) {
		fprintf(stderr, "\nError: Failed to receive addCallback_packet success reply\n");
		return 1;
	}
	close(adder_sockfd);

	printf("Done\n");
	return 0;
}

int callbacks_listen(uint16_t listener_port)
{
	const unsigned char validate_packet[] =
	{
		0x49, 0x63, 0x65, 0x50, 0x01, 0x00, 0x01, 0x00, 0x03, 0x00, 0x0e, 0x00, 0x00, 0x00
	};
	struct sockaddr_in serv_addr;
	char read_buffer[READ_BUFFER_SIZE];
	char *username;
	int listener_sockfd = 0, listener_connfd = 0;

	listener_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	setsockopt(listener_sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

	memset(&serv_addr, '0', sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = listener_port;

	if (bind(listener_sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
		fprintf(stderr, "\nError: Failed to bind listener port %hu\n", ntohs(listener_port));
		return 1;
	}

	if (listen(listener_sockfd, 1) == -1) {
		fprintf(stderr, "\nError: Failed to listen\n");
		return 1;
	}

	printf("Listening for callbacks...\n");

	while (1) {
		listener_connfd = accept(listener_sockfd, (struct sockaddr*)NULL, NULL);

		if (write(listener_connfd, validate_packet, sizeof(validate_packet)) < 0)
			fprintf(stderr, "\nError: Failed to send validate_packet\n");

		while (read(listener_connfd, read_buffer, sizeof(read_buffer)) > 0) {
			/* Close connection when related packet received */
			if (read_buffer[8] == 0x4) {
				close(listener_connfd);
				listener_connfd = 0;
				break;
			}
			/* Determine if received packet represents userConnected or userDisconnected callback */
			if (read_buffer[62] == 'C') { /* Connected */
				username = read_buffer + 99;
				username[(unsigned)*(username - 1)] = '\0';
				printf("Murmur: %s connected\n", username);
			}
			else if (read_buffer[62] == 'D') { /* Disconnected */
				username = read_buffer + 102;
				username[(unsigned)*(username - 1)] = '\0';
				printf("Murmur: %s disconnected\n", username);
			}
		}
	}
	return 0;
}
