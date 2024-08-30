/* ======================================================================
 * YOU ARE EXPECTED TO MODIFY THIS FILE.
 * ====================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdint.h>
#include <stdbool.h>

#include "d1_udp.h"
#include "d2_lookup.h"

#define MAX_BUFFER_SIZE 1016
#define MAX_PACKET_SIZE 1024
#define HEADER_SIZE 8

// Method to create a client. This function creates a socket and allocates memory on the heap using malloc to hold the client.
D1Peer* d1_create_client() {
    // Create a UDP socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    // Check if the socket creation was successful
    if (sockfd < 0) {
        // Print an appropriate error message if socket creation failed
        perror("Feil ved oppretting av socket");
        // Return NULL if the socket creation failed, as no memory has been allocated yet, there's no need to free or close the socket
        return NULL;
    }

    // Allocate memory for a D1Peer structure
    D1Peer* client = malloc(sizeof(D1Peer));
    // Check if memory allocation was successful
    if (client == NULL) {
        // Print an appropriate error message, close the socket, and return NULL if memory allocation failed
        perror("Feil ved allokeringsminne for D1Peer-struktur");
        close(sockfd);
        return NULL;
    }

    // If we have reached this point, we have successfully created a socket and allocated memory for a D1Peer structure
    // Clear the D1Peer structure, setting all bytes in the memory area to 0
    memset(client, 0, sizeof(D1Peer));

    // Set the socket descriptor
    // Set the client's socket to be sockfd
    client->socket = sockfd;

    // Return the client
    return client;
}


// This method takes a D1Peer structure as input and closes the socket if it is not null, then frees the memory allocated for the structure.
D1Peer* d1_delete(D1Peer* peer) {
    // Check if the D1Peer structure is valid
    if (peer == NULL) {
        return NULL;
    }

    // Close the socket if it is open
    // If the socket descriptor is valid, the socket exists, and we should close it
    if (peer->socket != -1) {
        close(peer->socket);
    }

    // Free the memory allocated for the D1Peer structure
    // It has been previously checked that memory has been allocated for this structure at the beginning of the method
    free(peer);

    // Return NULL to indicate that the D1Peer object has been deleted
    return NULL;
}






// This method retrieves address information for the specified peer using DNS lookup and populates the D1Peer structure.
// It takes a pointer to a D1Peer structure, the name of the peer (peername), and the port number of the server (server_port) as input.
int d1_get_peer_info(struct D1Peer* peer, const char* peername, uint16_t server_port) {
    // Null check for arguments, ensuring that both peer and peername are not NULL
    if (peer == NULL || peername == NULL) {
        return 0; // Return 0 if any of the arguments are NULL
    }
  
    // Create an addrinfo struct to store the result of getaddrinfo
    // Initialize hints to zero and specify IPv4, UDP socket, and UDP protocol
    struct addrinfo hints, *result;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // Only IPv4 addresses
    hints.ai_socktype = SOCK_DGRAM; // UDP socket
    hints.ai_protocol = IPPROTO_UDP; // UDP protocol

    // Perform DNS lookup to obtain address information, storing the result in result
    int ret = getaddrinfo(peername, NULL, &hints, &result);
    if (ret != 0) {
        fprintf(stderr, "getaddrinfo failed: %s\n", gai_strerror(ret));
        return 0; // Return 0 if getaddrinfo fails
    }

    // Copy the address information to the D1Peer structure
    struct sockaddr_in* addr = (struct sockaddr_in*)result->ai_addr;
    peer->addr.sin_family = AF_INET;
    peer->addr.sin_addr = addr->sin_addr;
    peer->addr.sin_port = htons(server_port);

    // Free the memory allocated by getaddrinfo
    freeaddrinfo(result);

    return 1; // Return 1 to indicate successful retrieval of address information
}




// This function computes a checksum for a given data buffer along with flags and size.
// It takes the data buffer (data), flags, size of the data buffer (size), and the actual size of data in the buffer (size_of_data) as input.
uint16_t compute_checksum(char *data, uint16_t flags, uint32_t size, size_t size_of_data) {
    uint16_t checksum = 0; // Initialize checksum to 0
    uint16_t temp; // Temporary variable to store intermediate results
    uint16_t size_part1 = (uint16_t)(size >> 16); // Extract the first 16 bits of the size
    uint16_t size_part2 = (uint16_t)(size & 0xFFFF); // Extract the last 16 bits of the size

    // If size is 8, compute checksum based on flags and the first 16 bits of size
    if (size == 8) {
        temp = flags ^ size_part1; // Combine flags and the first 16 bits of size
        temp = temp ^ size_part2;  // Combine flags and the last 16 bits of size
        checksum = temp; // Set the checksum to the combined result
        return checksum; // Return the computed checksum
    }

    // Compute checksum based on flags and the first 16 bits of size
    temp = flags ^ size_part1; 
    temp = temp ^ size_part2;

    // Calculate the length of the data buffer
    size_t len = size_of_data;

    // Check if the length of the data buffer is even
    if (len % 2 == 0) {
        // Iterate through the data buffer two characters at a time until only one character is left
        for (size_t i = 0; i < len; i += 2) {
            // Extract two characters from the data buffer
            char first_char = data[i];
            char second_char = data[i + 1];

            // Convert the two 8-bit characters to 16-bit integers before combining
            uint16_t first_16_bits = (uint16_t)first_char;
            uint16_t second_16_bits = (uint16_t)second_char;

            // Combine the two 16-bit integers
            uint16_t combined = (first_16_bits << 8) | (second_16_bits & 0xFF);

            // Perform XOR operation with the combined 16 bits
            temp = temp ^ combined;
        }
    }

    // If the length of the data buffer is odd, handle the last 8-bit block separately
    if (len % 2 == 1) {
        for (size_t i = 0; i < len - 1; i += 2) {
            // Extract two characters from the data buffer
            char first_char = data[i];
            char second_char = data[i + 1];

            // Convert the two 8-bit characters to 16-bit integers before combining
            uint16_t first_16_bits = (uint16_t)first_char;
            uint16_t second_16_bits = (uint16_t)second_char;

            // Combine the two 16-bit integers
            uint16_t combined = (first_16_bits << 8) | (second_16_bits & 0xFF);

            // Perform XOR operation with the combined 16 bits
            temp = temp ^ combined;
        }
        // Perform XOR operation with the last character separately
        temp = temp ^ ((uint16_t)data[len - 1] << 8);
    }

    // Set the checksum to the final computed value
    checksum = temp;

    return checksum; // Return the computed checksum
}





// This function receives data from a D1Peer over the network.
// It takes a D1Peer pointer (peer), a character buffer (buffer) to store the received data,
// and the size of the buffer (sz) as input parameters.
int d1_recv_data(struct D1Peer* peer, char* buffer, size_t sz) {
    // Calculate the total size of the packet to be received, including the header
    size_t total_size = sz + HEADER_SIZE;

    // Allocate memory to store the received packet
    char* recv_packet = malloc(total_size);
    if (recv_packet == NULL) {
        perror("malloc");
        return -1; // Return -1 to indicate failure in memory allocation
    }

    // Receive the packet from the socket and store it in recv_packet buffer
    ssize_t recv_packet_bytes = recv(peer->socket, recv_packet, total_size, 0);
    if (recv_packet_bytes == -1) {
        perror("recv");
        free(recv_packet);
        return -1; // Return -1 to indicate failure in receiving the packet
    }

    // Copy the header from the received packet
    D1Header recv_header;
    memcpy(&recv_header, recv_packet, sizeof(D1Header));

    // Convert the header fields to host byte order
    recv_header.flags = ntohs(recv_header.flags);
    recv_header.checksum = ntohs(recv_header.checksum);
    recv_header.size = ntohl(recv_header.size);
    
    // Extract the sequence number from the flags field of the header
    int seqno_bit = (recv_header.flags >> 7) & 1;

    // Calculate the size of the data buffer in the received packet
    int buffer_size = recv_packet_bytes - HEADER_SIZE;

    // Copy the data buffer from the received packet to the provided buffer
    memcpy(buffer, recv_packet + sizeof(D1Header), buffer_size);

    // Calculate the checksum for the received data buffer
    int my_checksum = compute_checksum(buffer, recv_header.flags, recv_header.size, buffer_size);

    // Compare the size of the received packet with the size specified in the header
    uint32_t pack_size = recv_packet_bytes;
    if (recv_header.size != pack_size) {
        printf("Received packet has incorrect size\n");
        // Toggle the sequence number bit
        seqno_bit = seqno_bit ^ 1;
        // Send an acknowledgment with the updated sequence number bit
        d1_send_ack(peer, seqno_bit);
        free(recv_packet);
        return -1; // Return -1 to indicate failure due to incorrect packet size
    }

    // Compare the computed checksum with the checksum in the header
    if (my_checksum != recv_header.checksum) {
        printf("Received packet has incorrect checksum\n");
        // Toggle the sequence number bit
        seqno_bit = seqno_bit ^ 1;
        // Send an acknowledgment with the updated sequence number bit
        d1_send_ack(peer, seqno_bit);
        free(recv_packet);
        return -1; // Return -1 to indicate failure due to incorrect checksum
    }

    // Send an acknowledgment with the current sequence number bit
    d1_send_ack(peer, seqno_bit);

    // Free the memory allocated for the received packet
    free(recv_packet);

    // Return the number of bytes received in the buffer
    return buffer_size;
}


// This function waits for an acknowledgment (ACK) from the specified D1Peer.
// It sleeps for 1 second to allow time for the ACK to be received.
// If an ACK is received, it checks if it has the correct sequence number.
// If the ACK has the correct sequence number, it updates the next sequence number for the peer.
// If the ACK has the wrong sequence number, it resends the data packet.
// It returns 0 upon successful reception of a correct ACK, and -1 otherwise.
int d1_wait_ack(D1Peer* peer, char* buffer, size_t sz) {
    // Sleep for 1 second to allow time for the ACK to be received
    sleep(1);

    // Initialize a D1Header structure to store the received ACK header
    D1Header ack_header;
    
    // Receive the ACK header from the peer
    ssize_t recv_bytes = recv(peer->socket, &ack_header, sizeof(D1Header), 0);
   
    if (recv_bytes == -1) {
        perror("recv");
        return -1; // Return -1 to indicate failure in receiving the ACK
    }

    // Convert the flags field of the received ACK header to host byte order
    uint16_t r_flags = ntohs(ack_header.flags);

    // Extract the ACK flag from the flags field
    bool is_ack_packet = (r_flags & (1 << 8)) != 0;

    // Extract the ACK number from the flags field
    int ack_number = (r_flags & (1 << 0)) >> 0; 

    // Check if the received message is an ACK
    if (!(is_ack_packet)) {
        printf("Received packet is not an ACK\n");
        return -1; // Return -1 to indicate failure due to incorrect packet type
    }

    // Check if the ACK has the correct sequence number
    if (ack_number != peer->next_seqno) {
        printf("Received ACK has wrong ack number\n");
        // Resend the data packet since the ACK has the wrong sequence number
        d1_send_data(peer, buffer, sz);
    } else {
        // Update the next sequence number for the peer
        peer->next_seqno = (peer->next_seqno == 0) ? 1 : 0;
        printf("Received correct ACK for sequence number %d\n", ack_number);
    }

    return 0; // Return 0 to indicate successful reception of a correct ACK
}
int d1_send_data(D1Peer* peer, char* buffer, size_t sz) {
    size_t size_of_payload = sz;
    // Check if peer and buffer are valid 
    if (peer == NULL || buffer == NULL) {
        return -1; // Return error if either peer or buffer is NULL
    }
    
    // Check if the size of payload exceeds the maximum buffer size
    if (size_of_payload > MAX_BUFFER_SIZE) {
        printf("Buffer size: (%ld) is greater than the maximum buffer size\n", size_of_payload);
        return -1; // Return error if the size of payload exceeds the maximum buffer size
    }

    // Create a D1Header
    D1Header header;
    ssize_t sent_bytes;

    // Calculate the size of the entire packet (header + payload)
    uint32_t size = HEADER_SIZE + size_of_payload;
    
    // Set the flags according to the next sequence number
    uint16_t result;
    if (peer->next_seqno != 0) {
        result = FLAG_DATA | SEQNO;
    } else {
        result = FLAG_DATA;
    }

    // Calculate the checksum for the packet
    uint16_t flags = result;
    uint16_t checksum = compute_checksum(buffer, flags, size, size_of_payload);

    // Fill out the D1Header fields with correct values and convert them to network byte order
    header.size = size;
    header.flags = flags;
    header.checksum = checksum;
    header.size = htonl(header.size);
    header.flags = htons(header.flags);
    header.checksum = htons(header.checksum);
    
    // Copy the header and buffer into a packet for sending
    char packet[MAX_PACKET_SIZE];
    memcpy(packet, &header, sizeof(D1Header));
    memcpy(packet + sizeof(D1Header), buffer, size_of_payload);

    // Send the packet via the socket to the connected peer
    sent_bytes = sendto(peer->socket, packet, ntohl(header.size), 0, (struct sockaddr*)&(peer->addr), sizeof(peer->addr));
    if (sent_bytes == -1) {
        perror("sendto");
        return -1; // Return error if sending fails
    }
    
    // Wait for an ACK from the peer
    d1_wait_ack(peer, buffer, size_of_payload);

    return sent_bytes; // Return the number of bytes sent
}







uint16_t compute_ack_checksum(uint16_t flags, uint32_t size) {
    uint16_t checksum = 0; // Initialize checksum variable
    uint16_t temp; // Temporary variable for XOR operations
    uint16_t size_part1 = (uint16_t)(size >> 16); // Extract the first 16 bits of the size
    uint16_t size_part2 = (uint16_t)(size & 0xFFFF); // Extract the last 16 bits of the size

    // If the size is 8, perform XOR operations with flags and the first 16 bits of size
    if (size == 8) {
        temp = flags ^ size_part1; // XOR operation between flags and the first 16 bits of size
        temp = temp ^ size_part2;  // XOR operation between the previous result and the last 16 bits of size
        checksum = temp; // Set the checksum to the final result of XOR operations
        return checksum; // Return the computed checksum
    }

    return checksum; // Return 0 if the size is not 8 (no checksum computation needed)
}


void d1_send_ack(struct D1Peer* peer, int seqno) {
    // Create a D1Header for the ACK packet
    D1Header ack_header;
    uint16_t flags;
    
    // Determine the flags based on the sequence number
    if(seqno == 0) {
        flags = FLAG_ACK; // Set flags to indicate an ACK with sequence number 0
    } else {
        flags = FLAG_ACK | ACKNO; // Set flags to indicate an ACK with sequence number 1
    }
    
    uint32_t size = HEADER_SIZE; // Size of the ACK packet

    ack_header.flags = flags; // Set flags to indicate the type of ACK packet and its sequence number
    ack_header.checksum = compute_ack_checksum(flags, size); // Compute the checksum for the ACK packet
    ack_header.size = size; // Set the size of the ACK packet
    ack_header.size = htonl(ack_header.size); // Convert size to network byte order
    ack_header.flags = htons(ack_header.flags); // Convert flags to network byte order
    ack_header.checksum = htons(ack_header.checksum); // Convert checksum to network byte order

    char packet[MAX_PACKET_SIZE];
    memcpy(packet, &ack_header, sizeof(D1Header)); // Copy the ACK header to the packet buffer
    
    // Send the ACK packet to the peer
    ssize_t sent_bytes = sendto(peer->socket, packet, ntohl(ack_header.size), 0, (struct sockaddr*)&(peer->addr), sizeof(peer->addr));
    if (sent_bytes == -1) {
        perror("sendto"); // Print an error message if sending fails
        return;
    }

    printf("Sent ACK for sequence number %d\n", seqno); // Print a confirmation message after sending the ACK
}





