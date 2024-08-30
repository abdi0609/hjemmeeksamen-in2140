/* ======================================================================
 * YOU CAN MODIFY THIS FILE.
 * ====================================================================== */

#ifndef D2_LOOKUP_MOD_H
#define D2_LOOKUP_MOD_H

#include "d2_lookup.h" // Include d2_lookup.h to access the NetNode struct definition
#include "d1_udp.h"


//#define MAX_NODES 100  // Maksimalt antall noder som kan lagres
struct D2Client
{
    D1Peer* peer;
};

typedef struct D2Client D2Client;

struct LocalTreeStore
{
    struct NetNode* netnodes; // Pointer to an array of NetNode structures
    int number_of_nodes;
};

typedef struct LocalTreeStore LocalTreeStore;

#endif /* D2_LOOKUP_MOD_H */

