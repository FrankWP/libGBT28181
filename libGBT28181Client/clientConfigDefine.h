#ifndef _CLIENT_CONFIG_DEFINE_H_
#define _CLIENT_CONFIG_DEFINE_H_

#include <stdbool.h>
#include <osip2\osip_mt.h>
#include <eXosip2\eXosip.h>
#include <Windows.h>

#ifdef _MSC_VER
#define snprintf _snprintf
#endif // _MSC_VER

typedef struct _client_configurations
{
    bool online;
    bool initialed;
    CRITICAL_SECTION critical_section;
    char* client_user_name;
    char* client_authentication_ID;
    char* client_password;
    char* client_IP;
    int client_port;
    char* server_ID;
    char* server_domain;
    char* server_IP;
    int server_port;
    int expiration_interval;
    int heartbeat_interval;
    int max_time_out_number;
    int address_family;
    int protocol;
    struct eXosip_t* exosip_context;
    osip_message_t* registration_message;
    int registration_ID;
    struct osip_thread* register_thread;
    struct osip_thread* event_thread;
    struct osip_thread* keepalive_thread;
    bool thread_loop;
    unsigned long long MANSCDP_SN;
}client_configurations;

typedef enum _MANSCDP_xml_type
{
    MANSCDP_xml_Unknown,
    MANSCDP_xml_Control,
    MANSCDP_xml_Query,
    MANSCDP_xml_Notify,
    MANSCDP_xml_Response
}MANSCDP_xml_type;

typedef enum _MANSCDP_command_type
{
    MANSCDP_command_type_unknown,
    MANSCDP_DeviceInfo,
    MANSCDP_DeviceStatus,
    MANSCDP_Catalog
}MANSCDP_command_type;

typedef struct _MANSCDP_xml
{
    MANSCDP_xml_type xml_type;
    MANSCDP_command_type command_type;
    unsigned long long MANSCDP_SN;
    char* DeviceID;
    client_configurations* p_client_configurations;
}MANSCDP_xml;

#endif // !_CLIENT_CONFIG_DEFINE_H_

