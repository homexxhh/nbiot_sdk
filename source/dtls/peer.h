/**
 * Copyright (c) 2017 China Mobile IOT.
 * All rights reserved.
 * Reference:
 *  tinydtls - https://sourceforge.net/projects/tinydtls/?source=navbar
**/

#ifndef NBIOT_SOURCE_DTLS_PEER_H_
#define NBIOT_SOURCE_DTLS_PEER_H_

#include "state.h"
#include "global.h"
#include "crypto.h"
#include "session.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    DTLS_CLIENT=0,
    DTLS_SERVER 
} dtls_peer_type;

/** 
 * Holds security parameters, local state and the transport address
 * for each peer.
*/
typedef struct dtls_peer_t
{
    struct dtls_peer_t          *next;

    const session_t             *session; /**< peer address and local interface */

    dtls_peer_type               role;    /**< denotes if this host is DTLS_CLIENT or DTLS_SERVER */
    dtls_state_t                 state;   /**< DTLS engine state */

    dtls_security_parameters_t  *security_params[2];
    dtls_handshake_parameters_t *handshake_params;
} dtls_peer_t;

static inline dtls_security_parameters_t* dtls_security_params_epoch( dtls_peer_t *peer,
                                                                      uint16_t     epoch )
{
    if ( peer->security_params[0] && peer->security_params[0]->epoch == epoch )
    {
        return peer->security_params[0];
    }
    else if ( peer->security_params[1] && peer->security_params[1]->epoch == epoch )
    {
        return peer->security_params[1];
    }
    else
    {
        return NULL;
    }
}

static inline dtls_security_parameters_t* dtls_security_params( dtls_peer_t *peer )
{
    return peer->security_params[0];
}

static inline dtls_security_parameters_t* dtls_security_params_next( dtls_peer_t *peer )
{
    if ( peer->security_params[1] )
        dtls_security_free( peer->security_params[1] );

    peer->security_params[1] = dtls_security_new( );
    if ( !peer->security_params[1] )
    {
        return NULL;
    }
    peer->security_params[1]->epoch = peer->security_params[0]->epoch + 1;
    return peer->security_params[1];
}

static inline void dtls_security_params_free_other( dtls_peer_t *peer )
{
    dtls_security_parameters_t * security0 = peer->security_params[0];
    dtls_security_parameters_t * security1 = peer->security_params[1];

    if ( !security0 || !security1 || security0->epoch < security1->epoch )
        return;

    dtls_security_free( security1 );
    peer->security_params[1] = NULL;
}

static inline void dtls_security_params_switch( dtls_peer_t *peer )
{
    dtls_security_parameters_t * security = peer->security_params[1];

    peer->security_params[1] = peer->security_params[0];
    peer->security_params[0] = security;
}

void peer_init( );

/**
 * Creates a new peer for given @p session. The current configuration
 * is initialized with the cipher suite TLS_NULL_WITH_NULL_NULL (i.e.
 * no security at all). This function returns a pointer to the new
 * peer or NULL on error. The caller is responsible for releasing the
 * storage allocated for this peer using dtls_free_peer().
 *
 * @param session  The remote peer's address and local interface index.
 * @return A pointer to a newly created and initialized peer object
 * or NULL on error.
*/
dtls_peer_t *dtls_new_peer( const session_t *session );

/** Releases the storage allocated to @p peer. */
void dtls_free_peer( dtls_peer_t *peer );

/** Returns the current state of @p peer. */
static inline dtls_state_t dtls_peer_state( const dtls_peer_t *peer )
{
    return peer->state;
}

/**
* Checks if given @p peer is connected. This function returns
* @c 1 if connected, or @c 0 otherwise.
*/
static inline int dtls_peer_is_connected( const dtls_peer_t *peer )
{
    return peer->state == DTLS_STATE_CONNECTED;
}

#ifdef __cplusplus
} /* extern "C" { */
#endif

#endif /* NBIOT_SOURCE_DTLS_PEER_H_ */