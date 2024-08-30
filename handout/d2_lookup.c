/* ======================================================================
 * YOU ARE EXPECTED TO MODIFY THIS FILE.
 * ====================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "d2_lookup.h"
//#include "net_node.h"  // Include the full definition of NetNode here
#define MAX_PAYLOAD_SIZE 1016

D2Client* d2_client_create(const char* server_name, uint16_t server_port)
{
    // Allocate memory for the D2Client structure
    D2Client* client = (D2Client*)malloc(sizeof(D2Client));
    if (client == NULL) {
        // Memory allocation failed
        return NULL;
    }

    // Create a D1Peer structure using d1_create_client
    D1Peer* temp_cli = d1_create_client();
    if (temp_cli == NULL) {
        // Error handling if creation of D1Peer failed
        free(client); // Free allocated memory for D2Client
        return NULL;
    }

    // Assign the created D1Peer to the peer member of D2Client
    client->peer = temp_cli;

    // Retrieve and store peer information
    if (!d1_get_peer_info(client->peer, server_name, server_port)) {
        // Error handling if getting peer info failed
        free(client->peer); // Free allocated memory for D1Peer
        free(client);       // Free allocated memory for D2Client
        return NULL;
    }

    // Return the created D2Client structure
    return client;
}


D2Client* d2_client_delete( D2Client* client )
{
   
    if (client!=NULL){
        d1_delete(client->peer);
        free(client);

    }
  
    return NULL;
}

int d2_send_request(D2Client* client, uint32_t id) {
    if (client == NULL || client->peer == NULL) {
        // Invalid client or peer
        return -1;
    }

    // Create a PacketRequest struct
    PacketRequest request;

    // Set request type
    request.type = htons(TYPE_REQUEST); // Convert to network byte order

    // Set request id
    request.id = htonl(id); // Convert to network byte order

    // Create packet array
    char packet[sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint32_t)];
    
    // Copy request type to packet
    memcpy(packet, &request.type, sizeof(uint16_t));

    // Set next 16 bits to 0
    uint16_t zero_bits = 0;
    memcpy(packet + sizeof(uint16_t), &zero_bits, sizeof(uint16_t));

    // Copy request id to packet
    memcpy(packet + sizeof(uint16_t) + sizeof(uint16_t), &request.id, sizeof(uint32_t));

    // Send the packet to the server
    int sc = d1_send_data(client->peer, packet, sizeof(packet));
    if (sc == -1) {
    perror("Error sending data to the server");
    return -1;
}

    return sc;
}
int d2_recv_response_size( D2Client* client )

{
    
   
    if (client == NULL || client->peer == NULL) {
        // Invalid client or peer
        return -1;
    }

 

    // Read the PacketResponseSize packet from the server
    char packet[sizeof(PacketResponseSize)];
    int res= d1_recv_data(client->peer, packet,sizeof(PacketResponseSize));

    if (res < 0) {
        // Error reading packet
        return res;
    }
   

    // Convert type and size to host byte order
   PacketResponseSize *response_size = (PacketResponseSize *)packet;

    uint16_t size =ntohs(response_size->size);
  

    // Return the size field from the received PacketResponseSize packet
    return size;
}
    


int d2_recv_response(D2Client *client, char *buffer, size_t sz)
{
    /* implement this */

    // Prepare buffer to receive the PacketResponse packet altså oppretter et midlertidig buffer med samme størrelse som det som skal brukes
    char response_buffer[sz];

    // kaller på d1 recv datat som fyller dette bufferet med pakken

    // Receive the PacketResponse packet from the server
    int ret = d1_recv_data(client->peer, response_buffer, sz);
    if (ret < 0)
    {
        fprintf(stderr, "Error: Failed to receive response\n");
        return ret;
    }



    // Extract the payload size from the received packet
    //caster bufferet til packetresponse struct og henter ut payload size i host byte order ettersom vi fikk det i network byte order
    PacketResponse *response = (PacketResponse *)response_buffer;
    uint16_t payload_size = ntohs(response->payload_size);

    // Check if the provided buffer size is sufficient
    //hvis det vi fikk er større enn det vi har maks plass til 
    if (payload_size > sz)
    {
        fprintf(stderr, "Error: Buffer size insufficient for response\n");
        return -1;
    }

    // Copy the received payload (including the PacketResponse header) to the provided buffer
    memcpy(buffer, response_buffer, payload_size);

    return payload_size;
}

LocalTreeStore* d2_alloc_local_tree(int num_nodes) {
    // Allocate memory for the LocalTreeStore structure
    LocalTreeStore* tree_store = (LocalTreeStore*)malloc(sizeof(LocalTreeStore));
    if (tree_store == NULL) {
        // Memory allocation failed
        return NULL;
    }

    // Allocate memory for the array of NetNode structures
    tree_store->netnodes = (NetNode*)malloc(num_nodes * sizeof(NetNode));
    if (tree_store->netnodes == NULL) {
        // Memory allocation failed
        free(tree_store); // Free previously allocated memory
        return NULL;
    }

    // Initialize the number_of_nodes field
    tree_store->number_of_nodes = num_nodes;

    // Return a pointer to the allocated LocalTreeStore structure
    return tree_store;
}

void d2_free_local_tree(LocalTreeStore* nodes) {
    // Free any dynamically allocated memory within the LocalTreeStore structure
    // (if any)
    // Then free the LocalTreeStore structure itself
  
    if (nodes != NULL) {
        free(nodes->netnodes);
        free(nodes);
    }
    
}
int d2_add_to_local_tree(LocalTreeStore* nodes, int node_idx, char* buffer, int buflen) {
   

    // Anta at hver NetNode har minst 3 * 32 bits størrelse
    size_t min_node_size = 3 * sizeof(uint32_t);

    // Sjekk om det er nok plass i LocalTreeStore til å legge til de nye nodene
    while (buflen > 0 && node_idx < nodes->number_of_nodes) {
        // Sjekk om den tredje 32-bits blokken er lik null
        uint32_t* third_32_bits = (uint32_t*)(buffer + 2 * sizeof(uint32_t));
        size_t node_size;
        if (ntohl(*third_32_bits) == 0) {
            // Hvis den tredje 32-bits blokken er null, juster størrelsen av NetNode
            node_size = min_node_size;
        } else {
            // Hvis den tredje 32-bits blokken ikke er null, beregn størrelsen basert på antall barn
            // Anta at hver barn legger til 32 bits ekstra
            uint32_t* num_children = (uint32_t*)(buffer + 2 * sizeof(uint32_t));
            node_size = min_node_size + (ntohl(*num_children) * sizeof(uint32_t));
        }

        // Kopier data fra bufferet til node_buffer
        NetNode node_buffer;
        memcpy(&node_buffer, buffer, node_size);

        // Kopier node_buffer til riktig plass i arrayet
        nodes->netnodes[node_idx++] = node_buffer;

        // Oppdater buffer-pekeren og buflen for å hoppe til neste NetNode
        buffer += node_size;
        buflen -= node_size;
    }

    // Returner den nye totalen av noder som er lagt til
    return node_idx;
}





void print_node_recursive(NetNode* node, uint32_t depth, LocalTreeStore* nodes) {
    // Print indentation based on the depth of the node
    for (uint32_t i = 0; i < depth; i++) {
        printf("--");
    }

    // Print node information
    printf("id %u value %u children %u\n", ntohl(node->id), ntohl(node->value), ntohl(node->num_children));

    // Recursively print children nodes
    for (uint32_t j = 0; j < ntohl(node->num_children); j++) {
        print_node_recursive(&nodes->netnodes[ntohl(node->child_id[j])], depth + 1, nodes);
    }
}

void d2_print_tree(LocalTreeStore* nodes) {
    // Iterate over all nodes in the LocalTreeStore
    for (int i = 0; i < nodes->number_of_nodes; i++) {
        // If the current node has id 0, it is the root of the tree
        if (ntohl(nodes->netnodes[i].id) == 0) {
            // Print root node
            printf("id %u value %u children %u\n", 
                   ntohl(nodes->netnodes[i].id), 
                   ntohl(nodes->netnodes[i].value), 
                   ntohl(nodes->netnodes[i].num_children));

            // Print children of the root node recursively
            for (uint32_t j = 0; j < ntohl(nodes->netnodes[i].num_children); j++) {
                print_node_recursive(&nodes->netnodes[ntohl(nodes->netnodes[i].child_id[j])], 1, nodes);
            }
            break; // Stop iteration after printing the root node and its children
        }
    }
}








