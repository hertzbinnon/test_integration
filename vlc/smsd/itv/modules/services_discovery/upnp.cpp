/*****************************************************************************
 * upnp.cpp :  UPnP discovery module (libupnp)
 *****************************************************************************
 * Copyright (C) 2004-2011 the VideoLAN team
 * $Id$
 *
 * Authors: Rémi Denis-Courmont <rem # videolan.org> (original plugin)
 *          Christian Henz <henz # c-lab.de>
 *          Mirsal Ennaime <mirsal dot ennaime at gmail dot com>
 *          Hugo Beauzée-Luyssen <hugo@beauzee.fr>
 *
 * UPnP Plugin using the Intel SDK (libupnp) instead of CyberLink
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#define __STDC_CONSTANT_MACROS 1

#undef PACKAGE_NAME
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "upnp.hpp"

#include <vlc_access.h>
#include <vlc_plugin.h>
#include <vlc_services_discovery.h>
#include <vlc_url.h>

#include <assert.h>
#include <limits.h>
#include <algorithm>

/*
 * Constants
*/
const char* MEDIA_SERVER_DEVICE_TYPE = "urn:schemas-upnp-org:device:MediaServer:1";
const char* CONTENT_DIRECTORY_SERVICE_TYPE = "urn:schemas-upnp-org:service:ContentDirectory:1";

/*
 * VLC handle
 */
struct services_discovery_sys_t
{
    UpnpInstanceWrapper* p_upnp;
    SD::MediaServerList* p_server_list;
};

struct access_sys_t
{
    UpnpInstanceWrapper* p_upnp;
};

UpnpInstanceWrapper* UpnpInstanceWrapper::s_instance;
vlc_mutex_t UpnpInstanceWrapper::s_lock = VLC_STATIC_MUTEX;

/*
 * VLC callback prototypes
 */
namespace SD
{
    static int Open( vlc_object_t* );
    static void Close( vlc_object_t* );
}

namespace Access
{
    static int Open( vlc_object_t* );
    static void Close( vlc_object_t* );
}

VLC_SD_PROBE_HELPER( "upnp", "Universal Plug'n'Play", SD_CAT_LAN )

/*
 * Module descriptor
 */
vlc_module_begin()
    set_shortname( "UPnP" );
    set_description( N_( "Universal Plug'n'Play" ) );
    set_category( CAT_PLAYLIST );
    set_subcategory( SUBCAT_PLAYLIST_SD );
    set_capability( "services_discovery", 0 );
    set_callbacks( SD::Open, SD::Close );

    add_submodule()
        set_category( CAT_INPUT )
        set_subcategory( SUBCAT_INPUT_ACCESS )
        set_callbacks( Access::Open, Access::Close )
        set_capability( "access", 0 )

    VLC_SD_PROBE_SUBMODULE
vlc_module_end()


/*
 * Returns the value of a child element, or NULL on error
 */
const char* xml_getChildElementValue( IXML_Element* p_parent,
                                      const char*   psz_tag_name )
{
    assert( p_parent );
    assert( psz_tag_name );

    IXML_NodeList* p_node_list;
    p_node_list = ixmlElement_getElementsByTagName( p_parent, psz_tag_name );
    if ( !p_node_list ) return NULL;

    IXML_Node* p_element = ixmlNodeList_item( p_node_list, 0 );
    ixmlNodeList_free( p_node_list );
    if ( !p_element )   return NULL;

    IXML_Node* p_text_node = ixmlNode_getFirstChild( p_element );
    if ( !p_text_node ) return NULL;

    return ixmlNode_getNodeValue( p_text_node );
}

/*
 * Extracts the result document from a SOAP response
 */
IXML_Document* parseBrowseResult( IXML_Document* p_doc )
{
    assert( p_doc );

    // ixml*_getElementsByTagName will ultimately only case the pointer to a Node
    // pointer, and pass it to a private function. Don't bother have a IXML_Document
    // version of getChildElementValue
    const char* psz_raw_didl = xml_getChildElementValue( (IXML_Element*)p_doc, "Result" );

    if( !psz_raw_didl )
        return NULL;

    /* First, try parsing the buffer as is */
    IXML_Document* p_result_doc = ixmlParseBuffer( psz_raw_didl );
    if( !p_result_doc ) {
        /* Missing namespaces confuse the ixml parser. This is a very ugly
         * hack but it is needeed until devices start sending valid XML.
         *
         * It works that way:
         *
         * The DIDL document is extracted from the Result tag, then wrapped into
         * a valid XML header and a new root tag which contains missing namespace
         * definitions so the ixml parser understands it.
         *
         * If you know of a better workaround, please oh please fix it */
        const char* psz_xml_result_fmt = "<?xml version=\"1.0\" ?>"
            "<Result xmlns:sec=\"urn:samsung:metadata:2009\">%s</Result>";

        char* psz_xml_result_string = NULL;
        if( -1 == asprintf( &psz_xml_result_string,
                             psz_xml_result_fmt,
                             psz_raw_didl) )
            return NULL;

        p_result_doc = ixmlParseBuffer( psz_xml_result_string );
        free( psz_xml_result_string );
    }

    if( !p_result_doc )
        return NULL;

    IXML_NodeList *p_elems = ixmlDocument_getElementsByTagName( p_result_doc,
                                                                "DIDL-Lite" );

    IXML_Node *p_node = ixmlNodeList_item( p_elems, 0 );
    ixmlNodeList_free( p_elems );

    return (IXML_Document*)p_node;
}

namespace SD
{

/*
 * Initializes UPNP instance.
 */
static int Open( vlc_object_t *p_this )
{
    services_discovery_t *p_sd = ( services_discovery_t* )p_this;
    services_discovery_sys_t *p_sys  = ( services_discovery_sys_t * )
            calloc( 1, sizeof( services_discovery_sys_t ) );

    if( !( p_sd->p_sys = p_sys ) )
        return VLC_ENOMEM;

    p_sys->p_server_list = new(std::nothrow) SD::MediaServerList( p_sd );
    if ( unlikely( p_sys->p_server_list == NULL ) )
    {
        return VLC_ENOMEM;
    }

    p_sys->p_upnp = UpnpInstanceWrapper::get( p_this, SD::MediaServerList::Callback, p_sys->p_server_list );
    if ( !p_sys->p_upnp )
    {
        Close( p_this );
        return VLC_EGENERIC;
    }

    /* Search for media servers */
    int i_res = UpnpSearchAsync( p_sys->p_upnp->handle(), 5,
            MEDIA_SERVER_DEVICE_TYPE, p_sys->p_upnp );
    if( i_res != UPNP_E_SUCCESS )
    {
        msg_Err( p_sd, "Error sending search request: %s", UpnpGetErrorMessage( i_res ) );
        Close( p_this );
        return VLC_EGENERIC;
    }

    return VLC_SUCCESS;
}

/*
 * Releases resources.
 */
static void Close( vlc_object_t *p_this )
{
    services_discovery_t *p_sd = ( services_discovery_t* )p_this;
    services_discovery_sys_t *p_sys = p_sd->p_sys;

    if (p_sys->p_upnp)
        p_sys->p_upnp->release( true );
    delete p_sys->p_server_list;
    free( p_sys );
}

MediaServerDesc::MediaServerDesc(const std::string& udn, const std::string& fName, const std::string& loc)
    : UDN( udn )
    , friendlyName( fName )
    , location( loc )
    , inputItem( NULL )
{
}

MediaServerDesc::~MediaServerDesc()
{
    if (inputItem)
        vlc_gc_decref( inputItem );
}

/*
 * MediaServerList class
 */
MediaServerList::MediaServerList( services_discovery_t* p_sd )
    : p_sd_( p_sd )
{
    vlc_mutex_init( &lock_ );
}

MediaServerList::~MediaServerList()
{
    vlc_delete_all(list_);
    vlc_mutex_destroy( &lock_ );
}

bool MediaServerList::addServer( MediaServerDesc* desc )
{
    vlc_mutex_locker lock( &lock_ );
    input_item_t* p_input_item = NULL;
    if ( getServer( desc->UDN ) )
        return false;

    msg_Dbg( p_sd_, "Adding server '%s' with uuid '%s'", desc->friendlyName.c_str(), desc->UDN.c_str() );

    char* psz_mrl;
    if( asprintf(&psz_mrl, "upnp://%s?ObjectID=%s", desc->location.c_str(), desc->UDN.c_str() ) < 0 )
        return false;

    p_input_item = input_item_NewWithType( psz_mrl, desc->friendlyName.c_str(), 0,
                                           NULL, 0, -1, ITEM_TYPE_NODE );
    free( psz_mrl );
    if ( !p_input_item )
        return false;
    desc->inputItem = p_input_item;
    input_item_SetDescription( p_input_item, desc->UDN.c_str() );
    services_discovery_AddItem( p_sd_, p_input_item, NULL );
    list_.push_back( desc );
    return true;
}

MediaServerDesc* MediaServerList::getServer( const std::string& udn )
{
    std::vector<MediaServerDesc*>::const_iterator it = list_.begin();
    std::vector<MediaServerDesc*>::const_iterator ite = list_.end();

    for ( ; it != ite; ++it )
    {
        if( udn == (*it)->UDN )
        {
            return *it;
        }
    }
    return NULL;
}

void MediaServerList::parseNewServer( IXML_Document *doc, const std::string &location )
{
    if ( !doc )
    {
        msg_Err( p_sd_, "Null IXML_Document" );
        return;
    }

    if ( location.empty() )
    {
        msg_Err( p_sd_, "Empty location" );
        return;
    }

    const char* psz_base_url = location.c_str();

    /* Try to extract baseURL */
    IXML_NodeList* p_url_list = ixmlDocument_getElementsByTagName( doc, "URLBase" );
    if ( p_url_list )
    {
        if ( IXML_Node* p_url_node = ixmlNodeList_item( p_url_list, 0 ) )
        {
            IXML_Node* p_text_node = ixmlNode_getFirstChild( p_url_node );
            if ( p_text_node )
                psz_base_url = ixmlNode_getNodeValue( p_text_node );
        }
        ixmlNodeList_free( p_url_list );
    }

    /* Get devices */
    IXML_NodeList* p_device_list = ixmlDocument_getElementsByTagName( doc, "device" );

    if ( !p_device_list )
        return;
    for ( unsigned int i = 0; i < ixmlNodeList_length( p_device_list ); i++ )
    {
        IXML_Element* p_device_element = ( IXML_Element* ) ixmlNodeList_item( p_device_list, i );

        if( !p_device_element )
            continue;

        const char* psz_device_type = xml_getChildElementValue( p_device_element, "deviceType" );

        if ( !psz_device_type )
        {
            msg_Warn( p_sd_, "No deviceType found!" );
            continue;
        }

        if ( strncmp( MEDIA_SERVER_DEVICE_TYPE, psz_device_type,
                strlen( MEDIA_SERVER_DEVICE_TYPE ) - 1 ) )
            continue;

        const char* psz_udn = xml_getChildElementValue( p_device_element,
                                                        "UDN" );
        if ( !psz_udn )
        {
            msg_Warn( p_sd_, "No UDN!" );
            continue;
        }

        /* Check if server is already added */
        if ( p_sd_->p_sys->p_server_list->getServer( psz_udn ) )
        {
            msg_Warn( p_sd_, "Server with uuid '%s' already exists.", psz_udn );
            continue;
        }

        const char* psz_friendly_name =
                   xml_getChildElementValue( p_device_element,
                                             "friendlyName" );

        if ( !psz_friendly_name )
        {
            msg_Dbg( p_sd_, "No friendlyName!" );
            continue;
        }

        // We now have basic info, we need to get the content browsing url
        // so the access module can browse without fetching the manifest again

        /* Check for ContentDirectory service. */
        IXML_NodeList* p_service_list = ixmlElement_getElementsByTagName( p_device_element, "service" );
        if ( !p_service_list )
            continue;
        for ( unsigned int j = 0; j < ixmlNodeList_length( p_service_list ); j++ )
        {
            IXML_Element* p_service_element = (IXML_Element*)ixmlNodeList_item( p_service_list, j );

            const char* psz_service_type = xml_getChildElementValue( p_service_element, "serviceType" );
            if ( !psz_service_type )
            {
                msg_Warn( p_sd_, "No service type found." );
                continue;
            }

            int k = strlen( CONTENT_DIRECTORY_SERVICE_TYPE ) - 1;
            if ( strncmp( CONTENT_DIRECTORY_SERVICE_TYPE,
                        psz_service_type, k ) )
                continue;

            const char* psz_control_url = xml_getChildElementValue( p_service_element,
                                          "controlURL" );
            if ( !psz_control_url )
            {
                msg_Warn( p_sd_, "No control url found." );
                continue;
            }

            /* Try to browse content directory. */
            char* psz_url = ( char* ) malloc( strlen( psz_base_url ) + strlen( psz_control_url ) + 1 );
            if ( psz_url )
            {
                if ( UpnpResolveURL( psz_base_url, psz_control_url, psz_url ) == UPNP_E_SUCCESS )
                {
                    SD::MediaServerDesc* p_server = new(std::nothrow) SD::MediaServerDesc( psz_udn,
                            psz_friendly_name, psz_url );
                    free( psz_url );
                    if ( unlikely( !p_server ) )
                        break;

                    if ( !addServer( p_server ) )
                    {
                        delete p_server;
                        continue;
                    }
                }
                else
                    free( psz_url );
            }
        }
        ixmlNodeList_free( p_service_list );
    }
    ixmlNodeList_free( p_device_list );
}

void MediaServerList::removeServer( const std::string& udn )
{
    vlc_mutex_locker lock( &lock_ );

    MediaServerDesc* p_server = getServer( udn );
    if ( !p_server )
        return;

    msg_Dbg( p_sd_, "Removing server '%s'", p_server->friendlyName.c_str() );

    assert(p_server->inputItem);
    services_discovery_RemoveItem( p_sd_, p_server->inputItem );

    std::vector<MediaServerDesc*>::iterator it = std::find(list_.begin(), list_.end(), p_server);
    if (it != list_.end())
    {
        list_.erase( it );
    }
    delete p_server;
}

/*
 * Handles servers listing UPnP events
 */
int MediaServerList::Callback( Upnp_EventType event_type, void* p_event, void* p_user_data )
{
    MediaServerList* self = static_cast<MediaServerList*>( p_user_data );
    services_discovery_t* p_sd = self->p_sd_;

    switch( event_type )
    {
    case UPNP_DISCOVERY_ADVERTISEMENT_ALIVE:
    case UPNP_DISCOVERY_SEARCH_RESULT:
    {
        struct Upnp_Discovery* p_discovery = ( struct Upnp_Discovery* )p_event;

        IXML_Document *p_description_doc = NULL;

        int i_res;
        i_res = UpnpDownloadXmlDoc( p_discovery->Location, &p_description_doc );
        if ( i_res != UPNP_E_SUCCESS )
        {
            msg_Warn( p_sd, "Could not download device description! "
                            "Fetching data from %s failed: %s",
                            p_discovery->Location, UpnpGetErrorMessage( i_res ) );
            return i_res;
        }
        self->parseNewServer( p_description_doc, p_discovery->Location );
        ixmlDocument_free( p_description_doc );
    }
    break;

    case UPNP_DISCOVERY_ADVERTISEMENT_BYEBYE:
    {
        struct Upnp_Discovery* p_discovery = ( struct Upnp_Discovery* )p_event;

        self->removeServer( p_discovery->DeviceId );
    }
    break;

    case UPNP_EVENT_SUBSCRIBE_COMPLETE:
        msg_Warn( p_sd, "subscription complete" );
        break;

    case UPNP_DISCOVERY_SEARCH_TIMEOUT:
        msg_Warn( p_sd, "search timeout" );
        break;

    case UPNP_EVENT_RECEIVED:
    case UPNP_EVENT_AUTORENEWAL_FAILED:
    case UPNP_EVENT_SUBSCRIPTION_EXPIRED:
        // Those are for the access part
        break;

    default:
        msg_Err( p_sd, "Unhandled event, please report ( type=%d )", event_type );
        break;
    }

    return UPNP_E_SUCCESS;
}

}

namespace Access
{

MediaServer::MediaServer(const char *psz_url, access_t *p_access, input_item_node_t *node)
    : url_( psz_url )
    , access_( p_access )
    , node_( node )
{
}

void MediaServer::addItem(const char *objectID, const char *title )
{
    vlc_url_t url;
    vlc_UrlParse( &url, url_.c_str(), '?' );
    char* psz_url;

    if (asprintf( &psz_url, "upnp://%s://%s:%u%s?ObjectID=%s", url.psz_protocol,
                  url.psz_host, url.i_port ? url.i_port : 80, url.psz_path, objectID ) < 0 )
    {
        vlc_UrlClean( &url );
        return ;
    }
    vlc_UrlClean( &url );

    input_item_t* p_item = input_item_NewWithType( psz_url, title, 0, NULL,
                                                   0, -1, ITEM_TYPE_NODE );
    free( psz_url);
    if ( !p_item )
        return;
    input_item_CopyOptions( node_->p_item, p_item );
    input_item_node_AppendItem( node_, p_item );
    input_item_Release( p_item );
}

void MediaServer::addItem(const char* title, const char*, const char*,
                          mtime_t duration, const char* psz_url)
{
    input_item_t* p_item = input_item_NewExt( psz_url, title, 0, NULL, 0, duration );
    input_item_node_AppendItem( node_, p_item );
    input_item_Release( p_item );
}

/* Access part */
IXML_Document* MediaServer::_browseAction( const char* psz_object_id_,
                                           const char* psz_browser_flag_,
                                           const char* psz_filter_,
                                           const char* psz_requested_count_,
                                           const char* psz_sort_criteria_ )
{
    IXML_Document* p_action = NULL;
    IXML_Document* p_response = NULL;
    const char* psz_url = url_.c_str();

    if ( url_.empty() )
    {
        msg_Dbg( access_, "No subscription url set!" );
        return NULL;
    }

    int i_res;

    i_res = UpnpAddToAction( &p_action, "Browse",
            CONTENT_DIRECTORY_SERVICE_TYPE, "ObjectID", psz_object_id_ );

    if ( i_res != UPNP_E_SUCCESS )
    {
        msg_Dbg( access_, "AddToAction 'ObjectID' failed: %s",
                UpnpGetErrorMessage( i_res ) );
        goto browseActionCleanup;
    }

    i_res = UpnpAddToAction( &p_action, "Browse",
            CONTENT_DIRECTORY_SERVICE_TYPE, "StartingIndex", "0" );
    if ( i_res != UPNP_E_SUCCESS )
    {
        msg_Dbg( access_, "AddToAction 'StartingIndex' failed: %s",
                UpnpGetErrorMessage( i_res ) );
        goto browseActionCleanup;
    }

    i_res = UpnpAddToAction( &p_action, "Browse",
            CONTENT_DIRECTORY_SERVICE_TYPE, "BrowseFlag", psz_browser_flag_ );

    if ( i_res != UPNP_E_SUCCESS )
    {
        msg_Dbg( access_, "AddToAction 'BrowseFlag' failed: %s",
                UpnpGetErrorMessage( i_res ) );
        goto browseActionCleanup;
    }

    i_res = UpnpAddToAction( &p_action, "Browse",
            CONTENT_DIRECTORY_SERVICE_TYPE, "Filter", psz_filter_ );

    if ( i_res != UPNP_E_SUCCESS )
    {
        msg_Dbg( access_, "AddToAction 'Filter' failed: %s",
                UpnpGetErrorMessage( i_res ) );
        goto browseActionCleanup;
    }

    i_res = UpnpAddToAction( &p_action, "Browse",
            CONTENT_DIRECTORY_SERVICE_TYPE, "RequestedCount", psz_requested_count_ );

    if ( i_res != UPNP_E_SUCCESS )
    {
        msg_Dbg( access_, "AddToAction 'RequestedCount' failed: %s",
                UpnpGetErrorMessage( i_res ) );
        goto browseActionCleanup;
    }

    i_res = UpnpAddToAction( &p_action, "Browse",
            CONTENT_DIRECTORY_SERVICE_TYPE, "SortCriteria", psz_sort_criteria_ );

    if ( i_res != UPNP_E_SUCCESS )
    {
        msg_Dbg( access_, "AddToAction 'SortCriteria' failed: %s",
                UpnpGetErrorMessage( i_res ) );
        goto browseActionCleanup;
    }

    i_res = UpnpSendAction( access_->p_sys->p_upnp->handle(),
              psz_url,
              CONTENT_DIRECTORY_SERVICE_TYPE,
              NULL, /* ignored in SDK, must be NULL */
              p_action,
              &p_response );

    if ( i_res != UPNP_E_SUCCESS )
    {
        msg_Err( access_, "%s when trying the send() action with URL: %s",
                UpnpGetErrorMessage( i_res ), psz_url );

        ixmlDocument_free( p_response );
        p_response = NULL;
    }

browseActionCleanup:
    ixmlDocument_free( p_action );
    return p_response;
}

/*
 * Fetches and parses the UPNP response
 */
bool MediaServer::fetchContents()
{
    const char* objectID = "";
    vlc_url_t url;
    vlc_UrlParse( &url, access_->psz_location, '?');

    if ( url.psz_option && !strncmp( url.psz_option, "ObjectID=", strlen( "ObjectID=" ) ) )
    {
        objectID = &url.psz_option[strlen( "ObjectID=" )];
    }

    IXML_Document* p_response = _browseAction( objectID,
                                      "BrowseDirectChildren",
                                      "id,dc:title,res," /* Filter */
                                      "sec:CaptionInfo,sec:CaptionInfoEx,"
                                      "pv:subtitlefile",
                                      "0", /* RequestedCount */
                                      "" /* SortCriteria */
                                      );
    vlc_UrlClean( &url );
    if ( !p_response )
    {
        msg_Err( access_, "No response from browse() action" );
        return false;
    }

    IXML_Document* p_result = parseBrowseResult( p_response );

    ixmlDocument_free( p_response );

    if ( !p_result )
    {
        msg_Err( access_, "browse() response parsing failed" );
        return false;
    }

#ifndef NDEBUG
    msg_Dbg( access_, "Got DIDL document: %s", ixmlPrintDocument( p_result ) );
#endif

    IXML_NodeList* containerNodeList =
                ixmlDocument_getElementsByTagName( p_result, "container" );

    if ( containerNodeList )
    {
        for ( unsigned int i = 0; i < ixmlNodeList_length( containerNodeList ); i++ )
        {
            IXML_Element* containerElement = (IXML_Element*)ixmlNodeList_item( containerNodeList, i );

            const char* objectID = ixmlElement_getAttribute( containerElement,
                                                             "id" );
            if ( !objectID )
                continue;

            const char* title = xml_getChildElementValue( containerElement,
                                                          "dc:title" );
            if ( !title )
                continue;
            addItem(objectID, title);
        }
        ixmlNodeList_free( containerNodeList );
    }

    IXML_NodeList* itemNodeList = ixmlDocument_getElementsByTagName( p_result,
                                                                     "item" );
    if ( itemNodeList )
    {
        for ( unsigned int i = 0; i < ixmlNodeList_length( itemNodeList ); i++ )
        {
            IXML_Element* itemElement =
                        ( IXML_Element* )ixmlNodeList_item( itemNodeList, i );

            const char* objectID =
                        ixmlElement_getAttribute( itemElement, "id" );

            if ( !objectID )
                continue;

            const char* title =
                        xml_getChildElementValue( itemElement, "dc:title" );

            if ( !title )
                continue;

            const char* psz_subtitles = xml_getChildElementValue( itemElement,
                    "sec:CaptionInfo" );

            if ( !psz_subtitles )
                psz_subtitles = xml_getChildElementValue( itemElement,
                        "sec:CaptionInfoEx" );

            if ( !psz_subtitles )
                psz_subtitles = xml_getChildElementValue( itemElement,
                        "pv:subtitlefile" );

            /* Try to extract all resources in DIDL */
            IXML_NodeList* p_resource_list = ixmlDocument_getElementsByTagName( (IXML_Document*) itemElement, "res" );
            if ( p_resource_list )
            {
                int i_length = ixmlNodeList_length( p_resource_list );
                for ( int i = 0; i < i_length; i++ )
                {
                    mtime_t i_duration = -1;
                    int i_hours, i_minutes, i_seconds;
                    IXML_Element* p_resource = ( IXML_Element* ) ixmlNodeList_item( p_resource_list, i );
                    const char* psz_resource_url = xml_getChildElementValue( p_resource, "res" );
                    if( !psz_resource_url )
                        continue;
                    const char* psz_duration = ixmlElement_getAttribute( p_resource, "duration" );

                    if ( psz_duration )
                    {
                        if( sscanf( psz_duration, "%d:%02d:%02d",
                            &i_hours, &i_minutes, &i_seconds ) )
                            i_duration = INT64_C(1000000) * ( i_hours*3600 +
                                                              i_minutes*60 +
                                                              i_seconds );
                    }

                    addItem( title, objectID, psz_subtitles, i_duration, psz_resource_url );
                }
                ixmlNodeList_free( p_resource_list );
            }
            else
                continue;
        }
        ixmlNodeList_free( itemNodeList );
    }

    ixmlDocument_free( p_result );
    return true;
}

static int ReadDirectory( access_t *p_access, input_item_node_t* p_node )
{
    MediaServer server( p_access->psz_location, p_access, p_node );

    if ( !server.fetchContents() )
        return VLC_EGENERIC;
    return VLC_SUCCESS;
}

static int Control( access_t *, int i_query, va_list args )
{
    switch ( i_query )
    {
    case ACCESS_CAN_SEEK:
    case ACCESS_CAN_FASTSEEK:
    case ACCESS_CAN_PAUSE:
    case ACCESS_CAN_CONTROL_PACE:
        *va_arg( args, bool* ) = false;
        break;

    case ACCESS_GET_SIZE:
    {
        *va_arg( args, uint64_t * ) = 0;
        break;
    }
    case ACCESS_GET_PTS_DELAY:
        *va_arg( args, int64_t * ) = 0;
        break;

    case ACCESS_SET_PAUSE_STATE:
        /* Nothing to do */
        break;

    default:
        return VLC_EGENERIC;
    }
    return VLC_SUCCESS;
}

static int Open( vlc_object_t *p_this )
{
    access_t* p_access = (access_t*)p_this;
    access_sys_t* p_sys = new(std::nothrow) access_sys_t;
    if ( unlikely( !p_sys ) )
        return VLC_ENOMEM;

    p_access->p_sys = p_sys;
    p_sys->p_upnp = UpnpInstanceWrapper::get( p_this, NULL, NULL );
    if ( !p_sys->p_upnp )
    {
        delete p_sys;
        return VLC_EGENERIC;
    }

    p_access->pf_readdir = ReadDirectory;
    ACCESS_SET_CALLBACKS( NULL, NULL, Control, NULL );

    return VLC_SUCCESS;
}

static void Close( vlc_object_t* p_this )
{
    access_t* p_access = (access_t*)p_this;
    p_access->p_sys->p_upnp->release( false );
    delete p_access->p_sys;
}

}

UpnpInstanceWrapper::UpnpInstanceWrapper()
    : handle_( -1 )
    , opaque_( NULL )
    , callback_( NULL )
    , refcount_( 0 )
{
}

UpnpInstanceWrapper::~UpnpInstanceWrapper()
{
    UpnpUnRegisterClient( handle_ );
    UpnpFinish();
}

UpnpInstanceWrapper *UpnpInstanceWrapper::get(vlc_object_t *p_obj, Upnp_FunPtr callback, SD::MediaServerList *opaque)
{
    vlc_mutex_locker lock( &s_lock );
    if ( s_instance == NULL )
    {
        UpnpInstanceWrapper* instance = new(std::nothrow) UpnpInstanceWrapper;
        if ( unlikely( !instance ) )
            return NULL;

    #ifdef UPNP_ENABLE_IPV6
        char* psz_miface = var_InheritString( p_obj, "miface" );
        msg_Info( p_obj, "Initializing libupnp on '%s' interface", psz_miface );
        int i_res = UpnpInit2( psz_miface, 0 );
        free( psz_miface );
    #else
        /* If UpnpInit2 isnt available, initialize on first IPv4-capable interface */
        int i_res = UpnpInit( 0, 0 );
    #endif
        if( i_res != UPNP_E_SUCCESS )
        {
            msg_Err( p_obj, "Initialization failed: %s", UpnpGetErrorMessage( i_res ) );
            delete instance;
            return NULL;
        }

        ixmlRelaxParser( 1 );

        /* Register a control point */
        i_res = UpnpRegisterClient( Callback, instance, &instance->handle_ );
        if( i_res != UPNP_E_SUCCESS )
        {
            msg_Err( p_obj, "Client registration failed: %s", UpnpGetErrorMessage( i_res ) );
            delete instance;
            return NULL;
        }

        /* libupnp does not treat a maximum content length of 0 as unlimited
         * until 64dedf (~ pupnp v1.6.7) and provides no sane way to discriminate
         * between versions */
        if( (i_res = UpnpSetMaxContentLength( INT_MAX )) != UPNP_E_SUCCESS )
        {
            msg_Err( p_obj, "Failed to set maximum content length: %s",
                    UpnpGetErrorMessage( i_res ));
            delete instance;
            return NULL;
        }
        s_instance = instance;
    }
    s_instance->refcount_++;
    // This assumes a single UPNP SD instance
    if (callback && opaque)
    {
        assert(!s_instance->callback_ && !s_instance->opaque_);
        s_instance->opaque_ = opaque;
        s_instance->callback_ = callback;
    }
    return s_instance;
}

void UpnpInstanceWrapper::release(bool isSd)
{
    vlc_mutex_locker lock( &s_lock );
    if ( isSd )
    {
        callback_ = NULL;
        opaque_ = NULL;
    }
    if (--s_instance->refcount_ == 0)
    {
        delete s_instance;
        s_instance = NULL;
    }
}

UpnpClient_Handle UpnpInstanceWrapper::handle() const
{
    return handle_;
}

int UpnpInstanceWrapper::Callback(Upnp_EventType event_type, void *p_event, void *p_user_data)
{
    UpnpInstanceWrapper* self = static_cast<UpnpInstanceWrapper*>( p_user_data );
    vlc_mutex_locker lock( &self->s_lock );
    if ( !self->callback_ )
        return 0;
    self->callback_( event_type, p_event, self->opaque_ );
    return 0;
}
