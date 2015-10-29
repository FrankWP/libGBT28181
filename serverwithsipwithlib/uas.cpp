// uas.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <eXosip2/eXosip.h>
#include <stdio.h>
#include <stdlib.h>
#include <Winsock2.h>
/*
��include <netinet/in.h>
��include <sys/socket.h>
��include <sys/types.h>*/
#pragma comment(lib, "osip2.lib")
#pragma comment(lib, "osipparser2.lib")
#pragma comment(lib, "eXosip.lib")
#pragma comment(lib, "Iphlpapi.lib")
#pragma comment(lib, "Dnsapi.lib")
#pragma comment(lib, "ws2_32.lib")
int
main(int argc, char *argv[])
{
    eXosip_event_t *je = NULL;
    osip_message_t *ack = NULL;
    osip_message_t *invite = NULL;
    osip_message_t *answer = NULL;
    sdp_message_t *remote_sdp = NULL;
    int call_id, dialog_id;
    int i, j;
    int id;
    char *sour_call = "sip:24@172.17.0.36";
    char *dest_call = "sip:24@172.17.0.36:5061";//client ip
    char command;
    char tmp[4096];
    char localip[128];
    int pos = 0;
    eXosip_t* p_eXosip_t = eXosip_malloc();
    //��ʼ��sip
    i = eXosip_init(p_eXosip_t);
    if(i != 0)
    {
        printf("Can't initialize eXosip!\n");
        return -1;
    }
    else
    {
        printf("eXosip_init successfully!\n");
    }
    i = eXosip_listen_addr(p_eXosip_t, IPPROTO_UDP, NULL, 5060, AF_INET, 0);
    if(i != 0)
    {
        eXosip_quit(p_eXosip_t);
        fprintf(stderr, "eXosip_listen_addr error!\nCouldn't initialize transport layer!\n");
    }
    for(;;)
    {
        //�����Ƿ�����Ϣ����
        je = eXosip_event_wait(p_eXosip_t,0, 50);
        //Э��ջ���д����,��������δ֪
        eXosip_lock(p_eXosip_t);
        eXosip_default_action(p_eXosip_t,je);
        eXosip_automatic_refresh(p_eXosip_t);
        eXosip_unlock(p_eXosip_t);
        if(je == NULL)//û�н��յ���Ϣ
      continue;
        // printf ("the cid is %s, did is %s\n", je->did, je->cid);
        switch(je->type)
        {
            case EXOSIP_MESSAGE_NEW://�µ���Ϣ����
                printf(" EXOSIP_MESSAGE_NEW!\n");
                if(MSG_IS_MESSAGE(je->request))//������ܵ�����Ϣ������MESSAGE
                {
                    {
                        osip_body_t *body;
                        osip_message_get_body(je->request, 0, &body);
                        printf("I get the msg is: %s\n", body->body);
                        //printf ("the cid is %s, did is %s\n", je->did, je->cid);
                    }
                    //���չ�����Ҫ�ظ�OK��Ϣ
                    eXosip_message_build_answer(p_eXosip_t,je->tid, 200, &answer);
                    eXosip_message_send_answer(p_eXosip_t,je->tid, 200, answer);
                }
                break;
            case EXOSIP_CALL_INVITE:
                //�õ����յ���Ϣ�ľ�����Ϣ
                printf("Received a INVITE msg from %s:%s, UserName is %s, password is %s\n", je->request->req_uri->host,
              je->request->req_uri->port, je->request->req_uri->username, je->request->req_uri->password);
                //�õ���Ϣ��,��Ϊ����Ϣ����SDP��ʽ.
                remote_sdp = eXosip_get_remote_sdp(p_eXosip_t,je->did);
                call_id = je->cid;
                dialog_id = je->did;

                eXosip_lock(p_eXosip_t);
                eXosip_call_send_answer(p_eXosip_t,je->tid, 180, NULL);
                i = eXosip_call_build_answer(p_eXosip_t,je->tid, 200, &answer);
                if(i != 0)
                {
                    printf("This request msg is invalid!Cann't response!\n");
                    eXosip_call_send_answer(p_eXosip_t,je->tid, 400, NULL);
                }
                else
                {
                    _snprintf(tmp, 4096,
                         "v=0\r\n"
                         "o=anonymous 0 0 IN IP4 0.0.0.0\r\n"
                         "t=1 10\r\n"
                         "a=username:rainfish\r\n"
                         "a=password:123\r\n");

                    //���ûظ���SDP��Ϣ��,��һ���ƻ�������Ϣ��
                    //û�з�����Ϣ�壬ֱ�ӻظ�ԭ������Ϣ����һ�����Ĳ��á�
                    osip_message_set_body(answer, tmp, strlen(tmp));
                    osip_message_set_content_type(answer, "application/sdp");

                    eXosip_call_send_answer(p_eXosip_t,je->tid, 200, answer);
                    printf("send 200 over!\n");
                }
                eXosip_unlock(p_eXosip_t);

                //��ʾ����sdp��Ϣ���е�attribute ������,����ƻ�������ǵ���Ϣ
                printf("the INFO is :\n");
                while(!osip_list_eol(&(remote_sdp->a_attributes), pos))
                {
                    sdp_attribute_t *at;

                    at = (sdp_attribute_t *)osip_list_get(&remote_sdp->a_attributes, pos);
                    printf("%s : %s\n", at->a_att_field, at->a_att_value);//���������Ϊʲô��SDP��Ϣ��������a�����ű���������

                    pos++;
                }
                break;
            case EXOSIP_CALL_ACK:
                printf("ACK recieved!\n");
                // printf ("the cid is %s, did is %s\n", je->did, je->cid); 
                break;
            case EXOSIP_CALL_CLOSED:
                printf("the remote hold the session!\n");
                // eXosip_call_build_ack(dialog_id, &ack);
                //eXosip_call_send_ack(dialog_id, ack); 
                i = eXosip_call_build_answer(p_eXosip_t,je->tid, 200, &answer);
                if(i != 0)
                {
                    printf("This request msg is invalid!Cann't response!\n");
                    eXosip_call_send_answer(p_eXosip_t, je->tid, 400, NULL);

                }
                else
                {
                    eXosip_call_send_answer(p_eXosip_t, je->tid, 200, answer);
                    printf("bye send 200 over!\n");
                }
                break;
            case EXOSIP_CALL_MESSAGE_NEW://���ڸ����ͺ�EXOSIP_MESSAGE_NEW������Դ������ô���͵�
                /*
                // request related events within calls (except INVITE)
                EXOSIP_CALL_MESSAGE_NEW,            < announce new incoming request.
                // response received for request outside calls
                EXOSIP_MESSAGE_NEW,            < announce new incoming request.
                ��Ҳ���Ǻ����ף�����ǣ�EXOSIP_CALL_MESSAGE_NEW��һ�������е��µ���Ϣ����������ring trying���㣬�����ڽ��ܵ�������ж�
                ����Ϣ���ͣ�EXOSIP_MESSAGE_NEW���Ǳ�ʾ���Ǻ����ڵ���Ϣ������
                �ý����в��׵ط��������ο���
                */
                printf(" EXOSIP_CALL_MESSAGE_NEW\n");
                if(MSG_IS_INFO(je->request)) //����������INFO����
                {
                    eXosip_lock(p_eXosip_t);
                    i = eXosip_call_build_answer(p_eXosip_t, je->tid, 200, &answer);
                    if(i == 0)
                    {
                        eXosip_call_send_answer(p_eXosip_t, je->tid, 200, answer);
                    }
                    eXosip_unlock(p_eXosip_t);
                    {
                        osip_body_t *body;
                        osip_message_get_body(je->request, 0, &body);
                        printf("the body is %s\n", body->body);
                    }
                }
                break;
            default:
                printf("Could not parse the msg!\n");
        }
    }

    osip_free(p_eXosip_t);
}

