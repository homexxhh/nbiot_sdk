﻿/**
 * Copyright (c) 2017 China Mobile IOT.
 * All rights reserved.
 * Reference:
 *  wakaama - https://github.com/eclipse/wakaama
**/

#include "internals.h"

/* dataP array length is assumed to be 1. */
static size_t prv_textSerialize( lwm2m_data_t * dataP,
                                 uint8_t ** bufferP )
{
    switch ( dataP->type )
    {
        case LWM2M_TYPE_STRING:
        *bufferP = (uint8_t *)nbiot_malloc( dataP->value.asBuffer.length );
        if ( *bufferP == NULL ) return 0;
        nbiot_memmove( *bufferP, dataP->value.asBuffer.buffer, dataP->value.asBuffer.length );
        return dataP->value.asBuffer.length;

        case LWM2M_TYPE_INTEGER:
        return utils_int64ToPlainText( dataP->value.asInteger, bufferP );

        case LWM2M_TYPE_FLOAT:
        return utils_float64ToPlainText( dataP->value.asFloat, bufferP );

        case LWM2M_TYPE_BOOLEAN:
        return utils_boolToPlainText( dataP->value.asBoolean, bufferP );

        case LWM2M_TYPE_OBJECT_LINK:
        {
            char stringBuffer[20];
            int len = nbiot_snprintf( stringBuffer, 20, "%d:%d",
                                      dataP->value.asObjLink.objectId,
                                      dataP->value.asObjLink.objectInstanceId );
            *bufferP = (uint8_t *)nbiot_malloc( len );
            if ( *bufferP == NULL ) return 0;
            nbiot_memmove( *bufferP, stringBuffer, len );
            return len;
        }
        case LWM2M_TYPE_OPAQUE:
        case LWM2M_TYPE_UNDEFINED:
        default:
        return 0;
    }
}

static int prv_setBuffer( lwm2m_data_t * dataP,
                          uint8_t * buffer,
                          size_t bufferLen )
{
    dataP->value.asBuffer.buffer = (uint8_t *)nbiot_malloc( bufferLen );
    if ( dataP->value.asBuffer.buffer == NULL )
    {
        lwm2m_data_free( 1, dataP );
        return 0;
    }
    dataP->value.asBuffer.length = bufferLen;
    nbiot_memmove( dataP->value.asBuffer.buffer, buffer, bufferLen );

    return 1;
}

lwm2m_data_t * lwm2m_data_new( int size )
{
    lwm2m_data_t * dataP;

    LOG_ARG( "size: %d", size );
    if ( size <= 0 ) return NULL;

    dataP = (lwm2m_data_t *)nbiot_malloc( size * sizeof(lwm2m_data_t) );

    if ( dataP != NULL )
    {
        nbiot_memzero( dataP, size * sizeof(lwm2m_data_t) );
    }

    return dataP;
}

void lwm2m_data_free( int size,
                      lwm2m_data_t * dataP )
{
    int i;

    LOG_ARG( "size: %d", size );
    if ( size == 0 || dataP == NULL ) return;

    for ( i = 0; i < size; i++ )
    {
        switch ( dataP[i].type )
        {
            case LWM2M_TYPE_MULTIPLE_RESOURCE:
            case LWM2M_TYPE_OBJECT_INSTANCE:
            case LWM2M_TYPE_OBJECT:
            lwm2m_data_free( dataP[i].value.asChildren.count, dataP[i].value.asChildren.array );
            break;

            case LWM2M_TYPE_STRING:
            case LWM2M_TYPE_OPAQUE:
            if ( dataP[i].value.asBuffer.buffer != NULL )
            {
                nbiot_free( dataP[i].value.asBuffer.buffer );
            }

            default:
            /* do nothing */
            break;
        }
    }
    nbiot_free( dataP );
}

void lwm2m_data_encode_string( const char * string,
                               lwm2m_data_t * dataP )
{
    size_t len;
    int res;

    LOG_ARG( "\"%s\"", string );
    if ( string == NULL )
    {
        len = 0;
    }
    else
    {
        for ( len = 0; string[len] != 0; len++ );
    }

    if ( len == 0 )
    {
        dataP->value.asBuffer.length = 0;
        dataP->value.asBuffer.buffer = NULL;
        res = 1;
    }
    else
    {
        res = prv_setBuffer( dataP, (uint8_t *)string, len );
    }

    if ( res == 1 )
    {
        dataP->type = LWM2M_TYPE_STRING;
    }
    else
    {
        dataP->type = LWM2M_TYPE_UNDEFINED;
    }
}

void lwm2m_data_encode_opaque( uint8_t * buffer,
                               size_t length,
                               lwm2m_data_t * dataP )
{
    int res;

    LOG_ARG( "length: %d", length );
    if ( length == 0 )
    {
        dataP->value.asBuffer.length = 0;
        dataP->value.asBuffer.buffer = NULL;
        res = 1;
    }
    else
    {
        res = prv_setBuffer( dataP, buffer, length );
    }

    if ( res == 1 )
    {
        dataP->type = LWM2M_TYPE_OPAQUE;
    }
    else
    {
        dataP->type = LWM2M_TYPE_UNDEFINED;
    }
}

void lwm2m_data_encode_nstring( const char * string,
                                size_t length,
                                lwm2m_data_t * dataP )
{
    LOG_ARG( "length: %d, string: \"%s\"", length, string );
    lwm2m_data_encode_opaque( (uint8_t *)string, length, dataP );

    if ( dataP->type == LWM2M_TYPE_OPAQUE )
    {
        dataP->type = LWM2M_TYPE_STRING;
    }
}

void lwm2m_data_encode_int( int64_t value,
                            lwm2m_data_t * dataP )
{
    LOG_ARG( "value: %" PRId64 "", value );
    dataP->type = LWM2M_TYPE_INTEGER;
    dataP->value.asInteger = value;
}

int lwm2m_data_decode_int( const lwm2m_data_t * dataP,
                           int64_t * valueP )
{
    int result;

    LOG( "Entering" );
    switch ( dataP->type )
    {
        case LWM2M_TYPE_INTEGER:
        *valueP = dataP->value.asInteger;
        result = 1;
        break;

        case LWM2M_TYPE_STRING:
        result = utils_plainTextToInt64( dataP->value.asBuffer.buffer, dataP->value.asBuffer.length, valueP );
        break;

        case LWM2M_TYPE_OPAQUE:
        result = utils_opaqueToInt( dataP->value.asBuffer.buffer, dataP->value.asBuffer.length, valueP );
        if ( result == (int)dataP->value.asBuffer.length )
        {
            result = 1;
        }
        else
        {
            result = 0;
        }
        break;

        default:
        return 0;
    }
    LOG_ARG( "result: %d, value: %" PRId64, result, *valueP );

    return result;
}

void lwm2m_data_encode_float( double value,
                              lwm2m_data_t * dataP )
{
    LOG_ARG( "value: %f", value );
    dataP->type = LWM2M_TYPE_FLOAT;
    dataP->value.asFloat = value;
}

int lwm2m_data_decode_float( const lwm2m_data_t * dataP,
                             double * valueP )
{
    int result;

    LOG( "Entering" );
    switch ( dataP->type )
    {
        case LWM2M_TYPE_FLOAT:
        *valueP = dataP->value.asFloat;
        result = 1;
        break;

        case LWM2M_TYPE_INTEGER:
        *valueP = (double)dataP->value.asInteger;
        result = 1;
        break;

        case LWM2M_TYPE_STRING:
        result = utils_plainTextToFloat64( dataP->value.asBuffer.buffer, dataP->value.asBuffer.length, valueP );
        break;

        case LWM2M_TYPE_OPAQUE:
        result = utils_opaqueToFloat( dataP->value.asBuffer.buffer, dataP->value.asBuffer.length, valueP );
        if ( result == (int)dataP->value.asBuffer.length )
        {
            result = 1;
        }
        else
        {
            result = 0;
        }
        break;

        default:
        return 0;
    }

    LOG_ARG( "result: %d, value: %f", result, *valueP );

    return result;
}

void lwm2m_data_encode_bool( bool value,
                             lwm2m_data_t * dataP )
{
    LOG_ARG( "value: %s", value ? "true" : "false" );
    dataP->type = LWM2M_TYPE_BOOLEAN;
    dataP->value.asBoolean = value;
}

int lwm2m_data_decode_bool( const lwm2m_data_t * dataP,
                            bool * valueP )
{
    int result;

    LOG( "Entering" );
    switch ( dataP->type )
    {
        case LWM2M_TYPE_BOOLEAN:
        *valueP = dataP->value.asBoolean;
        result = 1;
        break;

        case LWM2M_TYPE_STRING:
        if ( dataP->value.asBuffer.length != 1 ) return 0;

        switch ( dataP->value.asBuffer.buffer[0] )
        {
            case '0':
            *valueP = false;
            result = 1;
            break;
            case '1':
            *valueP = true;
            result = 1;
            break;
            default:
            result = 0;
            break;
        }
        break;

        case LWM2M_TYPE_OPAQUE:
        if ( dataP->value.asBuffer.length != 1 ) return 0;

        switch ( dataP->value.asBuffer.buffer[0] )
        {
            case 0:
            *valueP = false;
            result = 1;
            break;
            case 1:
            *valueP = true;
            result = 1;
            break;
            default:
            result = 0;
            break;
        }
        break;

        default:
        result = 0;
        break;
    }

    LOG_ARG( "result: %d, value: %s", result, *valueP ? "true" : "false" );

    return result;
}

void lwm2m_data_encode_objlink( uint16_t objectId,
                                uint16_t objectInstanceId,
                                lwm2m_data_t * dataP )
{
    LOG_ARG( "value: %d/%d", objectId, objectInstanceId );
    dataP->type = LWM2M_TYPE_OBJECT_LINK;
    dataP->value.asObjLink.objectId = objectId;
    dataP->value.asObjLink.objectInstanceId = objectInstanceId;
}

void lwm2m_data_include( lwm2m_data_t * subDataP,
                         size_t count,
                         lwm2m_data_t * dataP )
{
    LOG_ARG( "count: %d", count );
    if ( subDataP == NULL || count == 0 ) return;

    switch ( subDataP[0].type )
    {
        case LWM2M_TYPE_STRING:
        case LWM2M_TYPE_OPAQUE:
        case LWM2M_TYPE_INTEGER:
        case LWM2M_TYPE_FLOAT:
        case LWM2M_TYPE_BOOLEAN:
        case LWM2M_TYPE_OBJECT_LINK:
        case LWM2M_TYPE_MULTIPLE_RESOURCE:
        dataP->type = LWM2M_TYPE_OBJECT_INSTANCE;
        break;
        case LWM2M_TYPE_OBJECT_INSTANCE:
        dataP->type = LWM2M_TYPE_OBJECT;
        break;
        default:
        return;
    }
    dataP->value.asChildren.count = count;
    dataP->value.asChildren.array = subDataP;
}

void lwm2m_data_encode_instances( lwm2m_data_t * subDataP,
                                  size_t count,
                                  lwm2m_data_t * dataP )
{
    LOG_ARG( "count: %d", count );
    lwm2m_data_include( subDataP, count, dataP );
    dataP->type = LWM2M_TYPE_MULTIPLE_RESOURCE;
}

int lwm2m_data_parse( lwm2m_uri_t * uriP,
                      uint8_t * buffer,
                      size_t bufferLen,
                      lwm2m_media_type_t format,
                      lwm2m_data_t ** dataP )
{
    LOG_ARG( "format: %s, bufferLen: %d", STR_MEDIA_TYPE( format ), bufferLen );
    LOG_URI( uriP );
    switch ( format )
    {
        case LWM2M_CONTENT_TEXT:
        if ( !LWM2M_URI_IS_SET_RESOURCE( uriP ) ) return 0;
        *dataP = lwm2m_data_new( 1 );
        if ( *dataP == NULL ) return 0;
        (*dataP)->id = uriP->resourceId;
        (*dataP)->type = LWM2M_TYPE_STRING;
        return prv_setBuffer( *dataP, buffer, bufferLen );

        case LWM2M_CONTENT_OPAQUE:
        if ( !LWM2M_URI_IS_SET_RESOURCE( uriP ) ) return 0;
        *dataP = lwm2m_data_new( 1 );
        if ( *dataP == NULL ) return 0;
        (*dataP)->id = uriP->resourceId;
        (*dataP)->type = LWM2M_TYPE_OPAQUE;
        return prv_setBuffer( *dataP, buffer, bufferLen );

        case LWM2M_CONTENT_TLV:
        return tlv_parse( buffer, bufferLen, dataP );

        default:
        return 0;
    }
}

size_t lwm2m_data_serialize( lwm2m_uri_t * uriP,
                             int size,
                             lwm2m_data_t * dataP,
                             lwm2m_media_type_t * formatP,
                             uint8_t ** bufferP )
{

    LOG_URI( uriP );
    LOG_ARG( "size: %d, formatP: %s", size, STR_MEDIA_TYPE( *formatP ) );

    /* Check format */
    if ( *formatP == LWM2M_CONTENT_TEXT
         || *formatP == LWM2M_CONTENT_OPAQUE )
    {
        if ( size != 1
             || !LWM2M_URI_IS_SET_RESOURCE( uriP )
             || dataP->type == LWM2M_TYPE_OBJECT
             || dataP->type == LWM2M_TYPE_OBJECT_INSTANCE
             || dataP->type == LWM2M_TYPE_MULTIPLE_RESOURCE )
        {
            *formatP = LWM2M_CONTENT_TLV;
        }
    }

    if ( *formatP == LWM2M_CONTENT_TEXT
         && dataP->type == LWM2M_TYPE_OPAQUE )
    {
        *formatP = LWM2M_CONTENT_OPAQUE;
    }
    LOG_ARG( "Final format: %s", STR_MEDIA_TYPE( *formatP ) );

    switch ( *formatP )
    {
        case LWM2M_CONTENT_TEXT:
        return prv_textSerialize( dataP, bufferP );

        case LWM2M_CONTENT_OPAQUE:
        *bufferP = (uint8_t *)nbiot_malloc( dataP->value.asBuffer.length );
        if ( *bufferP == NULL ) return 0;
        nbiot_memmove( *bufferP, dataP->value.asBuffer.buffer, dataP->value.asBuffer.length );
        return dataP->value.asBuffer.length;

        case LWM2M_CONTENT_TLV:
        {
            uint8_t baseUriStr[URI_MAX_STRING_LEN];
            size_t baseUriLen;
            uri_depth_t rootLevel;
            bool isResourceInstance;

            baseUriLen = uri_toString( uriP, baseUriStr, URI_MAX_STRING_LEN, &rootLevel );
            if ( baseUriLen <= 0 ) return 0;
            if ( rootLevel == URI_DEPTH_RESOURCE_INSTANCE )
            {
                isResourceInstance = true;
            }
            else
            {
                isResourceInstance = false;
            }
            return tlv_serialize( isResourceInstance, size, dataP, bufferP );
        }

        case LWM2M_CONTENT_LINK:
        return discover_serialize( NULL, uriP, size, dataP, bufferP );

        default:
        return 0;
    }
}
