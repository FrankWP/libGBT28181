#include <eXosip2/eXosip.h>
#include <osip2/osip_mt.h>
#include <sys/types.h>          /* See NOTES */
#include <stdlib.h>

//#define LINUX
#ifdef LINUX
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#else
#include <Windows.h>
#endif
int do_keepalive();
int do_catalog();


char *server_id = "34020000002000000001";           /*SIP������ID*//*Ĭ��ֵ��34020000002000000001*/
char *server_region = "3402000000";
char *server_ip = "172.18.10.113";                   /*SIP������IP��ַ*//*Ĭ��ֵ��192.168.1.178*/
char *server_port = "5060" ;                        /*SIP������IP�˿�*//*Ĭ��ֵ��5060*/

char *ipc_id = "34020000001320000001";              /*ý����������ID*//*Ĭ��ֵ��34020000001320000001*/
char *ipc_region = "3402000000";                    /*ipc��*/
char *ipc_username = "00000000002";
char *ipc_pwd = "12345678";                         /*ý��������������*//*Ĭ��ֵ��12345678*/
char *ipc_ip = "172.18.10.36";                      /*ý����������IP��ַ*/
char *ipc_port ="5060";                             /*ý����������IP�˿�*/

char *device_name ="device_name";                       /*�豸/����/ϵͳ����*/
char *device_manufacturer = "device_manufacturer";      /*�豸����*/
char *device_model = "device_model";                    /*�豸�ͺ�*/
char *device_firmware = "device_firmware";              /*�豸�̼��汾*/
char *device_encode = "ON";                              /*�Ƿ����*//*ȡֵ��Χ��ON/OFF*/
char *device_record = "OFF";                            /*�Ƿ�¼��*//*ȡֵ��Χ��ON/OFF*/
//QQ 462096860
#define _SIP_EXPIRES_TIME 3600



void process_event()
{
    printf("process_event \n");
    eXosip_event *event;
    for(;;)
    {
        if (!(event = eXosip_event_wait(0, 50)))
            continue ;
        eXosip_lock ();
        eXosip_automatic_refresh();//�Զ�ˢ��ע��
        eXosip_unlock ();
        printf("event type = %d\n",event->type);

        switch (event->type)
        {
        case EXOSIP_REGISTRATION_NEW:
            printf("received new registration\n");
            break;
        case EXOSIP_REGISTRATION_SUCCESS:
        {
            printf("EXOSIP_REGISTRATION_SUCCESS\n");
            int ret = do_keepalive();
             printf("ret = %d\n",ret);
        }
          //  on_process_register_success(event);

            break;
        case EXOSIP_REGISTRATION_FAILURE:
            if(event->response && event->response->status_code==401)
            {

                printf("status_code = 401\n");
                osip_message_t *reg = NULL;
                eXosip_lock();
                int ret =eXosip_clear_authentication_info();
                printf("ret1 = %d\n",ret);
              //  char *realm = NULL;
               // osip_www_authenticate_t *www_authenticate;
               // osip_message_get_www_authenticate(event->response,0,&www_authenticate);

               // osip_www_authenticate_to_str(www_authenticate,&realm);

                eXosip_add_authentication_info(ipc_username, ipc_username, ipc_pwd, "MD5", NULL);

                eXosip_register_build_register(event->rid, _SIP_EXPIRES_TIME, &reg);//

               // osip_message_set_authorization(reg,realm);

                eXosip_register_send_register(event->rid,reg);

               // osip_free(realm);
                eXosip_unlock();
            }
            break;
        case EXOSIP_REGISTRATION_TERMINATED:
            printf("Registration terminated\n");
            break;
        case EXOSIP_CALL_INVITE://��Ƶ����
            break;
        case EXOSIP_CALL_REINVITE:
            printf("EXOSIP_CALL_REINVIT\n");
            break;
        case EXOSIP_CALL_PROCEEDING:
            break;
        case EXOSIP_CALL_RINGING:
            break;
        case EXOSIP_CALL_ANSWERED:
            break;
        case EXOSIP_CALL_REDIRECTED:
            break;
        case EXOSIP_CALL_REQUESTFAILURE:
            break;
        case EXOSIP_CALL_SERVERFAILURE:
            break;
        case EXOSIP_CALL_GLOBALFAILURE:
            break;
        case EXOSIP_CALL_ACK://15
            printf("EXOSIP_CALL_ACK: ACK received for 200ok to INVITE\n");
            break;
        case EXOSIP_CALL_CANCELLED:
            break;
        case EXOSIP_CALL_TIMEOUT:
            break;
        case EXOSIP_CALL_MESSAGE_NEW:
            break;
        case EXOSIP_CALL_MESSAGE_PROCEEDING:
            break;
        case EXOSIP_CALL_MESSAGE_ANSWERED:
            break;
        case EXOSIP_CALL_MESSAGE_REDIRECTED:
            break;
        case EXOSIP_CALL_MESSAGE_REQUESTFAILURE:
            break;
        case EXOSIP_CALL_MESSAGE_SERVERFAILURE:
            break;
        case EXOSIP_CALL_MESSAGE_GLOBALFAILURE:
            break;
        case EXOSIP_CALL_CLOSED://25 ��Ƶ�ر�
           // on_process_call_close(event);
            break;
        case EXOSIP_CALL_RELEASED:
            printf("(�Է����������æ!)\n");
            break;
        case EXOSIP_MESSAGE_NEW://27 ��Ϣ����
             eXosip_message_send_answer (event->tid, 200,NULL);
             do_catalog();
            break;
        case EXOSIP_MESSAGE_PROCEEDING:
            break;
        case EXOSIP_MESSAGE_ANSWERED:	//29��ƵACK����
            break;
        case EXOSIP_MESSAGE_REDIRECTED:
            break;
        case EXOSIP_MESSAGE_REQUESTFAILURE:
            break;
        case EXOSIP_MESSAGE_SERVERFAILURE:
            break;
        case EXOSIP_MESSAGE_GLOBALFAILURE:
            break;
        case EXOSIP_SUBSCRIPTION_UPDATE:
            /////////////////
            break;
        case EXOSIP_SUBSCRIPTION_CLOSED:
            break;
        case EXOSIP_SUBSCRIPTION_NOANSWER:
            break;
        case EXOSIP_SUBSCRIPTION_PROCEEDING:
            break;
        case EXOSIP_SUBSCRIPTION_ANSWERED:
            break;
        case EXOSIP_SUBSCRIPTION_REDIRECTED:
            break;
        case EXOSIP_SUBSCRIPTION_REQUESTFAILURE:
            break;
        case EXOSIP_SUBSCRIPTION_SERVERFAILURE:
            break;
        case EXOSIP_SUBSCRIPTION_GLOBALFAILURE:
            break;
        case EXOSIP_SUBSCRIPTION_NOTIFY:
            break;
        case EXOSIP_SUBSCRIPTION_RELEASED:
            break;
        case EXOSIP_IN_SUBSCRIPTION_NEW:
            /////////////
            break;
        case EXOSIP_IN_SUBSCRIPTION_RELEASED:
            break;
        case EXOSIP_NOTIFICATION_NOANSWER:
            break;
        case EXOSIP_NOTIFICATION_PROCEEDING:
            break;
        case EXOSIP_NOTIFICATION_ANSWERED:
            break;
        case EXOSIP_NOTIFICATION_REDIRECTED:
            break;
        case EXOSIP_NOTIFICATION_REQUESTFAILURE:
            break;
        case EXOSIP_NOTIFICATION_SERVERFAILURE:
            break;
        case EXOSIP_NOTIFICATION_GLOBALFAILURE:
            break;
        case EXOSIP_EVENT_COUNT:
            break;
        default:
            break;
        }

        eXosip_event_free(event);
    }

}
int do_register()
{
    printf("do_register  \n");
    char fromuser[256];
    char proxy[256];
    char route[256];
    sprintf(fromuser,"sip:%s@%s",ipc_id,ipc_region);
    sprintf(proxy,"sip:%s@%s",server_id,server_region);
    sprintf(route,"<sip:%s:%s;lr>",server_ip,server_port);
    eXosip_clear_authentication_info();
    eXosip_add_authentication_info(ipc_username, ipc_username, ipc_pwd, NULL, NULL);
    osip_message_t *reg = NULL;
    int regid =eXosip_register_build_initial_register(fromuser, proxy, NULL,_SIP_EXPIRES_TIME,&reg);
    osip_message_set_route(reg,route);


    if(eXosip_register_send_register(regid, reg)!=0)
        return -1;
    printf( "do_register success \n");

    return 0;
}

osip_message_t *build_message()
{
    osip_message_t *message= NULL;
    char fromuser[256];
    char proxy[256];
    char route[256];
    memset(&route,0,sizeof(route));

/*    sprintf(fromuser,"sip:%s@%s",ipc_id,ipc_ip);
    sprintf(proxy,"sip:%s:%s",server_ip,server_ip);		*/	//������URL
    sprintf(fromuser,"sip:%s@%s",ipc_id,ipc_region);
    sprintf(proxy,"sip:%s@%s",server_id,server_region);
    sprintf(route,"<sip:%s:%s;lr>",server_ip,server_port);
   int ret= eXosip_message_build_request(&message,"MESSAGE",proxy,fromuser,route);

       printf("ret is %d \n",ret);

   return message;

}


int do_keepalive()
{
    osip_message_t *message = build_message();
    if(message == NULL)
    {
         printf("message = NULL\n");
        return -1;
    }
    char tmp[4096];
    snprintf (tmp, 4096,
              "<?xml version=\"1.0\"?>\n"
              "<Notify>\n"
              "<CmdType>Keepalive</CmdType>\n"
              "<SN>43</SN>\n"
              "<DeviceID>%s</DeviceID>\n"
              "<Status>OK</Status>\n"
              "</Notify>\n",ipc_id);
    char route[256];
    sprintf(route,"<sip:%s:%s;lr>",server_ip,server_port);
    osip_message_set_route(message,route);
    osip_message_set_body (message, tmp, strlen(tmp));
    osip_message_set_content_type (message,"Application/MANSCDP+xml");
    return eXosip_message_send_request(message);
}

int main()
{
    int ret = 0;

   // eXosip_guess_localip (AF_INET, localip, 128); // ��ȡ����IP
    ret = eXosip_init();/*��ʼ��osip��eXosipЭ��ջ*/
    if (0 != ret)
    {
        printf("Couldn't initialize eXosip!\r\n");
        return -1;
    }
    printf("eXosip_init success!\r\n");

    ret = eXosip_listen_addr(IPPROTO_UDP, NULL, 5060, AF_INET, 0);
    if (0 != ret)/*������ʼ��ʧ��*/
    {
        eXosip_quit();
        printf("eXosip_listen_addr error!\r\n");
        return -1;
    }


    printf("eXosip_listen_addr success!\r\n");

    do_register();
    process_event();


    eXosip_quit();

    return 0;

}


int do_catalog()
{
    osip_message_t *message= build_message();
    if(message == NULL)
        return -1;
    char tmp[4096];
    snprintf (tmp, 4096,
            "<?xml version=\"1.0\"?>\n"
            "<Response>\n"
            "<CmdType>Catalog</CmdType>\n"
            "<SN>%s</SN>\n"
            "<SumNum>1</SumNum>\n"
            "<DeviceID>%s</DeviceID>\n"
            "<DeviceList Num='1'>\n"
            "<Item>\n"
            "<DeviceID>%s</DeviceID>\n"
            "<Name>IPC</Name>\n"
            "<Manufacturer>Manufacturer</Manufacturer>\n"
            "<Model>Model</Model>\n"
            "<Owner>Owner</Owner>\n"
            "<CivilCode>CivilCode</CivilCode>\n"
            "<Address>%s</Address>\n"
            "<SafetyWay>1</SafetyWay>\n"//<!--  ע�᷽ʽ����ѡ��ȱʡΪ1�� 1�� ����sip3261��׼����֤ע��ģʽ��2�� ���ڿ����˫����֤ע��ģʽ��3�� ��������֤���˫����֤ע��ģʽ-->
            "<Secrecy>0</Secrecy>\n"//<!--  �������ԣ���ѡ��ȱʡΪ0��0�������ܣ�1������-->
            "<Status>OK</Status>\n"
            "</Item>\n"
            "</DeviceList>\n"
              "</Response>","248",ipc_id,ipc_id,"Address");

    osip_message_set_body (message, tmp, strlen(tmp));
    osip_message_set_content_type (message,"Application/MANSCDP+xml");
    int ret = eXosip_message_send_request(message);
    return ret;


}

