/******************** (C) COPYRIGHT 2013 CSENN *********************************
* File Name          : csenn_eXosip.c
* Author             : www.csenn.com
* Date               : 2013/05/08
**************************************************************START OF FILE****/
#include "csenn_eXosip.h"

int g_register_id = 0;/*ע��ID/��������ע���ȡ��ע��*/
int g_call_id = 0;/*INVITE����ID/�����ֱ治ͬ��INVITE���ӣ�ÿ��ʱ��ֻ������һ��INVITE����*/
int g_did_realPlay = 0;/*�ỰID/�����ֱ治ͬ�ĻỰ��ʵʱ����Ƶ�㲥*/
int g_did_backPlay = 0;/*�ỰID/�����ֱ治ͬ�ĻỰ����ʷ����Ƶ�ط�*/
int g_did_fileDown = 0;/*�ỰID/�����ֱ治ͬ�ĻỰ������Ƶ�ļ�����*/
struct eXosip_t* g_p_eXosip_context = NULL;

int  csenn_eXosip_init(void);/*��ʼ��eXosip*/
int  csenn_eXosip_register(int expires);/*ע��eXosip���ֶ��������������ص�401״̬*/
int  csenn_eXosip_unregister(void);/*ע��eXosip*/
void csenn_eXosip_sendEventAlarm(char *alarm_time);/*�����¼�֪ͨ�ͷַ������ͱ���֪ͨ*/
void csenn_eXosip_sendFileEnd(void);/*��ʷ����Ƶ�طţ������ļ�����*/
void csenn_eXosip_paraseMsgBody(eXosip_event_t *p_event);/*����MESSAGE��XML��Ϣ��*/
void csenn_eXosip_paraseInviteBody(eXosip_event_t *p_event);/*����INVITE��SDP��Ϣ�壬ͬʱ����ȫ��INVITE����ID��ȫ�ֻỰID*/
void csenn_eXosip_paraseInfoBody(eXosip_event_t *p_event);/*����INFO��RTSP��Ϣ��*/
void csenn_eXosip_printEvent(eXosip_event_t *p_event);/*��Ⲣ��ӡ�¼�*/
void csenn_eXosip_processEvent(void);/*��Ϣѭ������*/
void csenn_eXosip_launch(void);/*������ע��eXosip*/

/*��ʼ��eXosip*/
int csenn_eXosip_init(void)
{
    int ret = 0;
    g_p_eXosip_context = eXosip_malloc();
    if(NULL == g_p_eXosip_context)
    {
        return -1;
    }

    ret = eXosip_init(g_p_eXosip_context);/*��ʼ��eXosip��osipЭ��ջ*/
    if(0 != ret)
    {
        printf("Couldn't initialize eXosip!\r\n");
        return -1;
    }
    printf("eXosip_init success!\r\n");

    ret = eXosip_listen_addr(g_p_eXosip_context, IPPROTO_UDP, NULL, 5060, AF_INET, 0);
    if(0 != ret)/*������ʼ��ʧ��*/
    {
        eXosip_quit(g_p_eXosip_context);
        osip_free(g_p_eXosip_context);
        printf("eXosip_listen_addr error!\r\n");
        return -1;
    }
    printf("eXosip_listen_addr success!\r\n");

    return 0;
}

/*ע��eXosip���ֶ��������������ص�401״̬*/
int csenn_eXosip_register(int expires)/*expires/ע����Ϣ����ʱ�䣬��λΪ��*/
{
    int ret = 0;
    eXosip_event_t *je = NULL;
    osip_message_t *reg = NULL;
    char from[100];/*sip:�����û���@����IP��ַ*/
    char proxy[100];/*sip:����IP��ַ:����IP�˿�*/

    memset(from, 0, 100);
    memset(proxy, 0, 100);
    sprintf(from, "sip:%s@%s", device_info.ipc_id, device_info.server_ip);
    sprintf(proxy, "sip:%s:%s", device_info.server_ip, device_info.server_port);

    /*���Ͳ�����֤��Ϣ��ע������*/
retry:
    eXosip_lock(g_p_eXosip_context);
    g_register_id = eXosip_register_build_initial_register(g_p_eXosip_context, from, proxy, NULL, expires, &reg);
    osip_message_set_authorization(reg, "Capability algorithm=\"H:MD5\"");
    if(0 > g_register_id)
    {
        eXosip_lock(g_p_eXosip_context);
        printf("eXosip_register_build_initial_register error!\r\n");
        return -1;
    }
    printf("eXosip_register_build_initial_register success!\r\n");

    ret = eXosip_register_send_register(g_p_eXosip_context, g_register_id, reg);
    eXosip_unlock(g_p_eXosip_context);
    if(0 != ret)
    {
        printf("eXosip_register_send_register no authorization error!\r\n");
        return -1;
    }
    printf("eXosip_register_send_register no authorization success!\r\n");

    printf("g_register_id=%d\r\n", g_register_id);

    for(;;)
    {
        je = eXosip_event_wait(g_p_eXosip_context, 0, 50);/*������Ϣ�ĵ���*/
        if(NULL == je)/*û�н��յ���Ϣ*/
        {
            continue;
        }
        if(EXOSIP_REGISTRATION_FAILURE == je->type)/*ע��ʧ��*/
        {
            printf("<EXOSIP_REGISTRATION_FAILURE>\r\n");
            printf("je->rid=%d\r\n", je->rid);
            /*�յ����������ص�ע��ʧ��/401δ��֤״̬*/
            if((NULL != je->response) && (401 == je->response->status_code))
            {
                reg = NULL;
                /*����Я����֤��Ϣ��ע������*/
                eXosip_lock(g_p_eXosip_context);
                eXosip_clear_authentication_info(g_p_eXosip_context);/*�����֤��Ϣ*/
                eXosip_add_authentication_info(g_p_eXosip_context,device_info.ipc_id, device_info.ipc_id, device_info.ipc_pwd, "MD5", NULL);/*���������û�����֤��Ϣ*/
                eXosip_register_build_register(g_p_eXosip_context,je->rid, expires, &reg);
                ret = eXosip_register_send_register(g_p_eXosip_context,je->rid, reg);
                eXosip_unlock(g_p_eXosip_context);
                if(0 != ret)
                {
                    printf("eXosip_register_send_register authorization error!\r\n");
                    return -1;
                }
                printf("eXosip_register_send_register authorization success!\r\n");
            }
            else/*������ע��ʧ��*/
            {
                printf("EXOSIP_REGISTRATION_FAILURE error!\r\n");
                goto retry;/*����ע��*/
            }
        }
        else if(EXOSIP_REGISTRATION_SUCCESS == je->type)
        {
            /*�յ����������ص�ע��ɹ�*/
            printf("<EXOSIP_REGISTRATION_SUCCESS>\r\n");
            g_register_id = je->rid;/*����ע��ɹ���ע��ID*/
            printf("g_register_id=%d\r\n", g_register_id);
            break;
        }
    }

    return 0;
}
/*ע��eXosip*/
int csenn_eXosip_unregister(void)
{
    return csenn_eXosip_register(0);
}

/*�����¼�֪ͨ�ͷַ������ͱ���֪ͨ*/
void csenn_eXosip_sendEventAlarm(char *alarm_time)
{
    if(0 == strcmp(device_status.status_guard, "OFFDUTY"))/*��ǰ����״̬Ϊ��OFFDUTY�������ͱ�����Ϣ*/
    {
        printf("device_status.status_guard=OFFDUTY\r\n");
    }
    else
    {
        osip_message_t *rqt_msg = NULL;
        char to[100];/*sip:�����û���@����IP��ַ*/
        char from[100];/*sip:����IP��ַ:����IP�˿�*/
        char xml_body[4096];

        memset(to, 0, 100);
        memset(from, 0, 100);
        memset(xml_body, 0, 4096);

        sprintf(to, "sip:%s@%s", device_info.ipc_id, device_info.server_ip);
        sprintf(from, "sip:%s:%s", device_info.server_ip, device_info.server_port);
        eXosip_message_build_request(g_p_eXosip_context,&rqt_msg, "MESSAGE", to, from, NULL);/*����"MESSAGE"����*/
        snprintf(xml_body, 4096, "<?xml version=\"1.0\"?>\r\n"
                                 "<Notify>\r\n"
                                 "<CmdType>Alarm</CmdType>\r\n"/*��������*/
                                 "<SN>1</SN>\r\n"/*�������к�*/
                                 "<DeviceID>%s</DeviceID>\r\n"/*�����豸����/�������ı���*/
                                 "<AlarmPriority>3</AlarmPriority>\r\n"/*��������/1Ϊһ������/2Ϊ��������/3Ϊ��������/4Ϊ�ļ�����*/
                                 "<AlarmMethod>2</AlarmMethod>\r\n"/*������ʽ/1Ϊ�绰����/2Ϊ�豸����/3Ϊ���ű���/4ΪGPS����/5Ϊ��Ƶ����/6Ϊ�豸���ϱ���/7��������*/
                                 "<AlarmTime>%s</AlarmTime>\r\n"/*����ʱ��/��ʽ��2012-12-18T16:23:32*/
                                 "</Notify>\r\n",
                                 device_info.ipc_id,
                                 alarm_time);
        osip_message_set_body(rqt_msg, xml_body, strlen(xml_body));
        osip_message_set_content_type(rqt_msg, "Application/MANSCDP+xml");
        eXosip_message_send_request(g_p_eXosip_context,rqt_msg);/*�ظ�"MESSAGE"����*/
        printf("csenn_eXosip_sendEventAlarm success!\r\n");

        strcpy(device_status.status_guard, "ALARM");/*���ò���״̬Ϊ"ALARM"*/
    }
}
/*��ʷ����Ƶ�طţ������ļ�����*/
void csenn_eXosip_sendFileEnd(void)
{
    if(0 != g_did_backPlay)/*��ǰ�ỰΪ����ʷ����Ƶ�ط�*/
    {
        osip_message_t *rqt_msg = NULL;
        char to[100];/*sip:�����û���@����IP��ַ*/
        char from[100];/*sip:����IP��ַ:����IP�˿�*/
        char xml_body[4096];

        memset(to, 0, 100);
        memset(from, 0, 100);
        memset(xml_body, 0, 4096);

        sprintf(to, "sip:%s@%s", device_info.ipc_id, device_info.server_ip);
        sprintf(from, "sip:%s:%s", device_info.server_ip, device_info.server_port);
        eXosip_message_build_request(g_p_eXosip_context,&rqt_msg, "MESSAGE", to, from, NULL);/*����"MESSAGE"����*/
        snprintf(xml_body, 4096, "<?xml version=\"1.0\"?>\r\n"
                                 "<Notify>\r\n"
                                 "<CmdType>MediaStatus</CmdType>\r\n"
                                 "<SN>8</SN>\r\n"
                                 "<DeviceID>%s</DeviceID>\r\n"
                                 "<NotifyType>121</NotifyType>\r\n"
                                 "</Notify>\r\n",
                                 device_info.ipc_id);
        osip_message_set_body(rqt_msg, xml_body, strlen(xml_body));
        osip_message_set_content_type(rqt_msg, "Application/MANSCDP+xml");
        eXosip_message_send_request(g_p_eXosip_context,rqt_msg);/*�ظ�"MESSAGE"����*/
        printf("csenn_eXosip_sendFileEnd success!\r\n");
    }
}

/*����MESSAGE��XML��Ϣ��*/
void csenn_eXosip_paraseMsgBody(eXosip_event_t *p_event)
{
    /*��������صı���*/
    osip_body_t *p_rqt_body = NULL;
    char *p_xml_body = NULL;
    char *p_str_begin = NULL;
    char *p_str_end = NULL;
    char xml_cmd_type[20];
    char xml_cmd_sn[10];
    char xml_device_id[30];
    char xml_command[30];
    /*��ظ���صı���*/
    osip_message_t *rsp_msg = NULL;
    char to[100];/*sip:�����û���@����IP��ַ*/
    char from[100];/*sip:����IP��ַ:����IP�˿�*/
    char rsp_xml_body[4096];

    memset(xml_cmd_type, 0, 20);
    memset(xml_cmd_sn, 0, 10);
    memset(xml_device_id, 0, 30);
    memset(xml_command, 0, 30);
    memset(to, 0, 100);
    memset(from, 0, 100);
    memset(rsp_xml_body, 0, 4096);

    sprintf(to, "sip:%s@%s", device_info.ipc_id, device_info.server_ip);
    sprintf(from, "sip:%s:%s", device_info.server_ip, device_info.server_port);
    eXosip_message_build_request(g_p_eXosip_context,&rsp_msg, "MESSAGE", to, from, NULL);/*����"MESSAGE"����*/

    osip_message_get_body(p_event->request, 0, &p_rqt_body);/*��ȡ���յ������XML��Ϣ��*/
    if(NULL == p_rqt_body)
    {
        printf("osip_message_get_body null!\r\n");
        return;
    }
    p_xml_body = p_rqt_body->body;
    printf("osip_message_get_body success!\r\n");

    printf("**********CMD START**********\r\n");
    p_str_begin = strstr(p_xml_body, "<CmdType>");/*�����ַ���"<CmdType>"*/
    p_str_end = strstr(p_xml_body, "</CmdType>");
    memcpy(xml_cmd_type, p_str_begin + 9, p_str_end - p_str_begin - 9);/*����<CmdType>��xml_cmd_type*/
    printf("<CmdType>:%s\r\n", xml_cmd_type);

    p_str_begin = strstr(p_xml_body, "<SN>");/*�����ַ���"<SN>"*/
    p_str_end = strstr(p_xml_body, "</SN>");
    memcpy(xml_cmd_sn, p_str_begin + 4, p_str_end - p_str_begin - 4);/*����<SN>��xml_cmd_sn*/
    printf("<SN>:%s\r\n", xml_cmd_sn);

    p_str_begin = strstr(p_xml_body, "<DeviceID>");/*�����ַ���"<DeviceID>"*/
    p_str_end = strstr(p_xml_body, "</DeviceID>");
    memcpy(xml_device_id, p_str_begin + 10, p_str_end - p_str_begin - 10);/*����<DeviceID>��xml_device_id*/
    printf("<DeviceID>:%s\r\n", xml_device_id);
    printf("***********CMD END***********\r\n");

    if(0 == strcmp(xml_cmd_type, "DeviceControl"))/*�豸����*/
    {
        printf("**********CONTROL START**********\r\n");
        /*�������ҡ����ϡ����¡��Ŵ���С��ֹͣң��*/
        p_str_begin = strstr(p_xml_body, "<PTZCmd>");/*�����ַ���"<PTZCmd>"*/
        if(NULL != p_str_begin)
        {
            p_str_end = strstr(p_xml_body, "</PTZCmd>");
            memcpy(xml_command, p_str_begin + 8, p_str_end - p_str_begin - 8);/*����<PTZCmd>��xml_command*/
            printf("<PTZCmd>:%s\r\n", xml_command);
            goto DeviceControl_Next;
        }
        /*��ʼ�ֶ�¼��ֹͣ�ֶ�¼��*/
        p_str_begin = strstr(p_xml_body, "<RecordCmd>");/*�����ַ���"<RecordCmd>"*/
        if(NULL != p_str_begin)
        {
            p_str_end = strstr(p_xml_body, "</RecordCmd>");
            memcpy(xml_command, p_str_begin + 11, p_str_end - p_str_begin - 11);/*����<RecordCmd>��xml_command*/
            printf("<RecordCmd>:%s\r\n", xml_command);
            goto DeviceControl_Next;
        }
        /*����������*/
        p_str_begin = strstr(p_xml_body, "<GuardCmd>");/*�����ַ���"<GuardCmd>"*/
        if(NULL != p_str_begin)
        {
            p_str_end = strstr(p_xml_body, "</GuardCmd>");
            memcpy(xml_command, p_str_begin + 10, p_str_end - p_str_begin - 10);/*����<GuardCmd>��xml_command*/
            printf("<GuardCmd>:%s\r\n", xml_command);
            goto DeviceControl_Next;
        }
        /*������λ��30���ڲ��ٴ�������*/
        p_str_begin = strstr(p_xml_body, "<AlarmCmd>");/*�����ַ���"<AlarmCmd>"*/
        if(NULL != p_str_begin)
        {
            p_str_end = strstr(p_xml_body, "</AlarmCmd>");
            memcpy(xml_command, p_str_begin + 10, p_str_end - p_str_begin - 10);/*����<AlarmCmd>��xml_command*/
            printf("<AlarmCmd>:%s\r\n", xml_command);
            goto DeviceControl_Next;
        }
        /*�豸Զ������*/
        p_str_begin = strstr(p_xml_body, "<TeleBoot>");/*�����ַ���"<TeleBoot>"*/
        if(NULL != p_str_begin)
        {
            p_str_end = strstr(p_xml_body, "</TeleBoot>");
            memcpy(xml_command, p_str_begin + 10, p_str_end - p_str_begin - 10);/*����<TeleBoot>��xml_command*/
            printf("<TeleBoot>:%s\r\n", xml_command);
            goto DeviceControl_Next;
        }
    DeviceControl_Next:
        printf("***********CONTROL END***********\r\n");
        if(0 == strcmp(xml_command, "A50F01021F0000D6"))/*����*/
        {
            csenn_eXosip_callback.csenn_eXosip_deviceControl(EXOSIP_CTRL_RMT_LEFT);
        }
        else if(0 == strcmp(xml_command, "A50F01011F0000D5"))/*����*/
        {
            csenn_eXosip_callback.csenn_eXosip_deviceControl(EXOSIP_CTRL_RMT_RIGHT);
        }
        else if(0 == strcmp(xml_command, "A50F0108001F00DC"))/*����*/
        {
            csenn_eXosip_callback.csenn_eXosip_deviceControl(EXOSIP_CTRL_RMT_UP);
        }
        else if(0 == strcmp(xml_command, "A50F0104001F00D8"))/*����*/
        {
            csenn_eXosip_callback.csenn_eXosip_deviceControl(EXOSIP_CTRL_RMT_DOWN);
        }
        else if(0 == strcmp(xml_command, "A50F0110000010D5"))/*�Ŵ�*/
        {
            csenn_eXosip_callback.csenn_eXosip_deviceControl(EXOSIP_CTRL_RMT_LARGE);
        }
        else if(0 == strcmp(xml_command, "A50F0120000010E5"))/*��С*/
        {
            csenn_eXosip_callback.csenn_eXosip_deviceControl(EXOSIP_CTRL_RMT_SMALL);
        }
        else if(0 == strcmp(xml_command, "A50F0100000000B5"))/*ֹͣң��*/
        {
            csenn_eXosip_callback.csenn_eXosip_deviceControl(EXOSIP_CTRL_RMT_STOP);
        }
        else if(0 == strcmp(xml_command, "Record"))/*��ʼ�ֶ�¼��*/
        {
            csenn_eXosip_callback.csenn_eXosip_deviceControl(EXOSIP_CTRL_REC_START);
        }
        else if(0 == strcmp(xml_command, "StopRecord"))/*ֹͣ�ֶ�¼��*/
        {
            csenn_eXosip_callback.csenn_eXosip_deviceControl(EXOSIP_CTRL_REC_STOP);
        }
        else if(0 == strcmp(xml_command, "SetGuard"))/*����*/
        {
            csenn_eXosip_callback.csenn_eXosip_deviceControl(EXOSIP_CTRL_GUD_START);
            strcpy(device_status.status_guard, "ONDUTY");/*���ò���״̬Ϊ"ONDUTY"*/
        }
        else if(0 == strcmp(xml_command, "ResetGuard"))/*����*/
        {
            csenn_eXosip_callback.csenn_eXosip_deviceControl(EXOSIP_CTRL_GUD_STOP);
            strcpy(device_status.status_guard, "OFFDUTY");/*���ò���״̬Ϊ"OFFDUTY"*/
        }
        else if(0 == strcmp(xml_command, "ResetAlarm"))/*������λ*/
        {
            csenn_eXosip_callback.csenn_eXosip_deviceControl(EXOSIP_CTRL_ALM_RESET);
        }
        else if(0 == strcmp(xml_command, "Boot"))/*�豸Զ������*/
        {
            csenn_eXosip_callback.csenn_eXosip_deviceControl(EXOSIP_CTRL_TEL_BOOT);
        }
        else
        {
            printf("unknown device control command!\r\n");
        }
        snprintf(rsp_xml_body, 4096, "<?xml version=\"1.0\"?>\r\n"
                                     "<Response>\r\n"
                                     "<CmdType>DeviceControl</CmdType>\r\n"/*��������*/
                                     "<SN>%s</SN>\r\n"/*�������к�*/
                                     "<DeviceID>%s</DeviceID>\r\n"/*Ŀ���豸/����/ϵͳ����*/
                                     "<Result>OK</Result>\r\n"/*ִ�н����־*/
                                     "</Response>\r\n",
                                     xml_cmd_sn,
                                     xml_device_id);
    }
    else if(0 == strcmp(xml_cmd_type, "Alarm"))/*�����¼�֪ͨ�ͷַ�������֪ͨ��Ӧ*/
    {
        printf("**********ALARM START**********\r\n");
        /*����֪ͨ��Ӧ*/
        printf("local eventAlarm response success!\n");
        printf("***********ALARM END***********\r\n");
        return;
    }
    else if(0 == strcmp(xml_cmd_type, "Catalog"))/*�����豸��Ϣ��ѯ���豸Ŀ¼��ѯ*/
    {
        printf("**********CATALOG START**********\r\n");
        /*�豸Ŀ¼��ѯ*/
        printf("***********CATALOG END***********\r\n");
        snprintf(rsp_xml_body, 4096, "<?xml version=\"1.0\"?>\r\n"
                                     "<Response>\r\n"
                                     "<CmdType>Catalog</CmdType>\r\n"/*��������*/
                                     "<SN>%s</SN>\r\n"/*�������к�*/
                                     "<DeviceID>%s</DeviceID>\r\n"/*Ŀ���豸/����/ϵͳ�ı���*/
                                     "<SumNum>1</SumNum>\r\n"/*��ѯ�������*/
                                     "<DeviceList Num=\"1\">\r\n"/*�豸Ŀ¼���б�*/
                                     "<Item>\r\n"
                                     "<DeviceID>%s</DeviceID>\r\n"/*Ŀ���豸/����/ϵͳ�ı���*/
                                     "<Name>%s</Name>\r\n"/*�豸/����/ϵͳ����*/
                                     "<Manufacturer>%s</Manufacturer>\r\n"/*�豸����*/
                                     "<Model>%s</Model>\r\n"/*�豸�ͺ�*/
                                     "<Owner>Owner1</Owner>\r\n"/*�豸����*/
                                     "<CivilCode>CivilCode1</CivilCode>\r\n"/*��������*/
                                     "<Block>Block1</Block>\r\n"/*����*/
                                     "<Address>Address1</Address>\r\n"/*��װ��ַ*/
                                     "<Parental>0</Parental>\r\n"/*�Ƿ������豸*/
                                     "<ParentID>%s</ParentID>\r\n"/*���豸/����/ϵͳID*/
                                     "<SafetyWay>0</SafetyWay>\r\n"/*���ȫģʽ/0Ϊ������/2ΪS/MIMEǩ����ʽ/3ΪS/MIME����ǩ��ͬʱ���÷�ʽ/4Ϊ����ժҪ��ʽ*/
                                     "<RegisterWay>1</RegisterWay>\r\n"/*ע�᷽ʽ/1Ϊ����sip3261��׼����֤ע��ģʽ/2Ϊ���ڿ����˫����֤ע��ģʽ/3Ϊ��������֤���˫����֤ע��ģʽ*/
                                     "<CertNum>CertNum1</CertNum>\r\n"/*֤�����к�*/
                                     "<Certifiable>0</Certifiable>\r\n"/*֤����Ч��ʶ/0Ϊ��Ч/1Ϊ��Ч*/
                                     "<ErrCode>400</ErrCode>\r\n"/*��Чԭ����*/
                                     "<EndTime>2099-12-31T23:59:59</EndTime>\r\n"/*֤����ֹ��Ч��*/
                                     "<Secrecy>0</Secrecy>\r\n"/*��������/0Ϊ������/1Ϊ����*/
                                     "<IPAddress>%s</IPAddress>\r\n"/*�豸/����/ϵͳIP��ַ*/
                                     "<Port>%s</Port>\r\n"/*�豸/����/ϵͳ�˿�*/
                                     "<Password>Password1</Password>\r\n"/*�豸����*/
                                     "<Status>OK</Status>\r\n"/*�豸״̬*/
                                     "<Longitude>171.3</Longitude>\r\n"/*����*/
                                     "<Latitude>34.2</Latitude>\r\n"/*γ��*/
                                     "</Item>\r\n"
                                     "</DeviceList>\r\n"
                                     "</Response>\r\n",
                                     xml_cmd_sn,
                                     xml_device_id,
                                     xml_device_id,
                                     device_info.device_name,
                                     device_info.device_manufacturer,
                                     device_info.device_model,
                                     xml_device_id,
                                     device_info.ipc_ip,
                                     device_info.ipc_port);
    }
    else if(0 == strcmp(xml_cmd_type, "DeviceInfo"))/*�����豸��Ϣ��ѯ���豸��Ϣ��ѯ*/
    {
        printf("**********DEVICE INFO START**********\r\n");
        /*�豸��Ϣ��ѯ*/
        printf("***********DEVICE INFO END***********\r\n");
        snprintf(rsp_xml_body, 4096, "<?xml version=\"1.0\"?>\r\n"
                                     "<Response>\r\n"
                                     "<CmdType>DeviceInfo</CmdType>\r\n"/*��������*/
                                     "<SN>%s</SN>\r\n"/*�������к�*/
                                     "<DeviceID>%s</DeviceID>\r\n"/*Ŀ���豸/����/ϵͳ�ı���*/
                                     "<Result>OK</Result>\r\n"/*��ѯ���*/
                                     "<Manufacturer>%s</Manufacturer>\r\n"/*�豸������*/
                                     "<Model>%s</Model>\r\n"/*�豸�ͺ�*/
                                     "<Firmware>%s</Firmware>\r\n"/*�豸�̼��汾*/
                                     "</Response>\r\n",
                                     xml_cmd_sn,
                                     xml_device_id,
                                     device_info.device_manufacturer,
                                     device_info.device_model,
                                     device_info.device_firmware);
    }
    else if(0 == strcmp(xml_cmd_type, "DeviceStatus"))/*�����豸��Ϣ��ѯ���豸״̬��ѯ*/
    {
        printf("**********DEVICE STATUS START**********\r\n");
        /*�豸״̬��ѯ*/
        printf("***********DEVICE STATUS END***********\r\n");
        char xml_status_guard[10];
        strcpy(xml_status_guard, device_status.status_guard);/*���浱ǰ����״̬*/
        csenn_eXosip_callback.csenn_eXosip_getDeviceStatus(&device_status);/*��ȡ�豸��ǰ״̬*/
        strcpy(device_status.status_guard, xml_status_guard);/*�ָ���ǰ����״̬*/
        snprintf(rsp_xml_body, 4096, "<?xml version=\"1.0\"?>\r\n"
                                     "<Response>\r\n"
                                     "<CmdType>DeviceStatus</CmdType>\r\n"/*��������*/
                                     "<SN>%s</SN>\r\n"/*�������к�*/
                                     "<DeviceID>%s</DeviceID>\r\n"/*Ŀ���豸/����/ϵͳ�ı���*/
                                     "<Result>OK</Result>\r\n"/*��ѯ�����־*/
                                     "<Online>%s</Online>\r\n"/*�Ƿ�����/ONLINE/OFFLINE*/
                                     "<Status>%s</Status>\r\n"/*�Ƿ���������*/
                                     "<Encode>%s</Encode>\r\n"/*�Ƿ����*/
                                     "<Record>%s</Record>\r\n"/*�Ƿ�¼��*/
                                     "<DeviceTime>%s</DeviceTime>\r\n"/*�豸ʱ�������*/
                                     "<Alarmstatus Num=\"1\">\r\n"/*�����豸״̬�б�*/
                                     "<Item>\r\n"
                                     "<DeviceID>%s</DeviceID>\r\n"/*�����豸����*/
                                     "<DutyStatus>%s</DutyStatus>\r\n"/*�����豸״̬*/
                                     "</Item>\r\n"
                                     "</Alarmstatus>\r\n"
                                     "</Response>\r\n",
                                     xml_cmd_sn,
                                     xml_device_id,
                                     device_status.status_online,
                                     device_status.status_ok,
                                     device_info.device_encode,
                                     device_info.device_record,
                                     device_status.status_time,
                                     xml_device_id,
                                     device_status.status_guard);
    }
    else if(0 == strcmp(xml_cmd_type, "RecordInfo"))/*�豸����Ƶ�ļ�����*/
    {
        /*¼���ļ�����*/
        char xml_file_path[30];
        char xml_start_time[30];
        char xml_end_time[30];
        char xml_recorder_id[30];
        char item_start_time[30];
        char item_end_time[30];
        char rsp_item_body[4096];
        int  record_list_num = 0;
        int  record_list_ret = 0;

        memset(xml_file_path, 0, 30);
        memset(xml_start_time, 0, 30);
        memset(xml_end_time, 0, 30);
        memset(xml_recorder_id, 0, 30);
        memset(item_start_time, 0, 30);
        memset(item_end_time, 0, 30);
        memset(rsp_item_body, 0, 4096);
        printf("**********RECORD INFO START**********\r\n");
        p_str_begin = strstr(p_xml_body, "<FilePath>");/*�����ַ���"<FilePath>"*/
        p_str_end = strstr(p_xml_body, "</FilePath>");
        memcpy(xml_file_path, p_str_begin + 10, p_str_end - p_str_begin - 10);/*����<FilePath>��xml_file_path*/
        printf("<FilePath>:%s\r\n", xml_file_path);

        p_str_begin = strstr(p_xml_body, "<StartTime>");/*�����ַ���"<StartTime>"*/
        p_str_end = strstr(p_xml_body, "</StartTime>");
        memcpy(xml_start_time, p_str_begin + 11, p_str_end - p_str_begin - 11);/*����<StartTime>��xml_start_time*/
        printf("<StartTime>:%s\r\n", xml_start_time);

        p_str_begin = strstr(p_xml_body, "<EndTime>");/*�����ַ���"<EndTime>"*/
        p_str_end = strstr(p_xml_body, "</EndTime>");
        memcpy(xml_end_time, p_str_begin + 9, p_str_end - p_str_begin - 9);/*����<EndTime>��xml_end_time*/
        printf("<EndTime>:%s\r\n", xml_end_time);

        p_str_begin = strstr(p_xml_body, "<RecorderID>");/*�����ַ���"<RecorderID>"*/
        p_str_end = strstr(p_xml_body, "</RecorderID>");
        memcpy(xml_recorder_id, p_str_begin + 12, p_str_end - p_str_begin - 12);/*����<RecorderID>��xml_recorder_id*/
        printf("<RecorderID>:%s\r\n", xml_recorder_id);
        printf("***********RECORD INFO END***********\r\n");
        for(;;)
        {
            record_list_ret = csenn_eXosip_callback.csenn_eXosip_getRecordTime(xml_start_time, xml_end_time, item_start_time, item_end_time);
            if(0 > record_list_ret)
            {
                break;
            }
            else
            {
                char temp_body[1024];
                memset(temp_body, 0, 1024);
                snprintf(temp_body, 1024, "<Item>\r\n"
                                          "<DeviceID>%s</DeviceID>\r\n"/*�豸/�������*/
                                          "<Name>%s</Name>\r\n"/*�豸/��������*/
                                          "<FilePath>%s</FilePath>\r\n"/*�ļ�·����*/
                                          "<Address>Address1</Address>\r\n"/*¼���ַ*/
                                          "<StartTime>%s</StartTime>\r\n"/*¼��ʼʱ��*/
                                          "<EndTime>%s</EndTime>\r\n"/*¼�����ʱ��*/
                                          "<Secrecy>0</Secrecy>\r\n"/*��������/0Ϊ������/1Ϊ����*/
                                          "<Type>time</Type>\r\n"/*¼���������*/
                                          "<RecorderID>%s</RecorderID>\r\n"/*¼�񴥷���ID*/
                                          "</Item>\r\n",
                                          xml_device_id,
                                          device_info.device_name,
                                          xml_file_path,
                                          item_start_time,
                                          item_end_time,
                                          xml_recorder_id);
                strcat(rsp_item_body, temp_body);
                record_list_num++;
                if(0 == record_list_ret)
                {
                    break;
                }
            }
        }
        if(0 >= record_list_num)/*δ�������κ��豸����Ƶ�ļ�*/
        {
            return;
        }
        snprintf(rsp_xml_body, 4096, "<?xml version=\"1.0\"?>\r\n"
                                     "<Response>\r\n"
                                     "<CmdType>RecordInfo</CmdType>\r\n"/*��������*/
                                     "<SN>%s</SN>\r\n"/*�������к�*/
                                     "<DeviceID>%s</DeviceID>\r\n"/*�豸/�������*/
                                     "<Name>%s</Name>\r\n"/*�豸/��������*/
                                     "<SumNum>%d</SumNum>\r\n"/*��ѯ�������*/
                                     "<RecordList Num=\"%d\">\r\n"/*�ļ�Ŀ¼���б�*/
                                     "%s\r\n"
                                     "</RecordList>\r\n"
                                     "</Response>\r\n",
                                     xml_cmd_sn,
                                     xml_device_id,
                                     device_info.device_name,
                                     record_list_num,
                                     record_list_num,
                                     rsp_item_body);
    }
    else/*CmdTypeΪ��֧�ֵ�����*/
    {
        printf("**********OTHER TYPE START**********\r\n");
        printf("***********OTHER TYPE END***********\r\n");
        return;
    }
    osip_message_set_body(rsp_msg, rsp_xml_body, strlen(rsp_xml_body));
    osip_message_set_content_type(rsp_msg, "Application/MANSCDP+xml");
    eXosip_message_send_request(g_p_eXosip_context,rsp_msg);/*�ظ�"MESSAGE"����*/
    printf("eXosip_message_send_request success!\r\n");
}
/*����INVITE��SDP��Ϣ�壬ͬʱ����ȫ��INVITE����ID��ȫ�ֻỰID*/
void csenn_eXosip_paraseInviteBody(eXosip_event_t *p_event)
{
    sdp_message_t *sdp_msg = NULL;
    char *media_sever_ip = NULL;
    char *media_sever_port = NULL;

    sdp_msg = eXosip_get_remote_sdp(g_p_eXosip_context,p_event->did);
    if(sdp_msg == NULL)
    {
        printf("eXosip_get_remote_sdp NULL!\r\n");
        return;
    }
    printf("eXosip_get_remote_sdp success!\r\n");

    g_call_id = p_event->cid;/*����ȫ��INVITE����ID*/
    /*ʵʱ�㲥*/
    if(0 == strcmp(sdp_msg->s_name, "Play"))
    {
        g_did_realPlay = p_event->did;/*����ȫ�ֻỰID��ʵʱ����Ƶ�㲥*/
    }
    /*�ط�*/
    else if(0 == strcmp(sdp_msg->s_name, "Playback"))
    {
        g_did_backPlay = p_event->did;/*����ȫ�ֻỰID����ʷ����Ƶ�ط�*/
    }
    /*����*/
    else if(0 == strcmp(sdp_msg->s_name, "Download"))
    {
        g_did_fileDown = p_event->did;/*����ȫ�ֻỰID������Ƶ�ļ�����*/
    }
    /*��SIP��������������INVITE�����o�ֶλ�c�ֶ��л�ȡý���������IP��ַ��˿�*/
    media_sever_ip = sdp_message_o_addr_get(sdp_msg);/*ý�������IP��ַ*/
    media_sever_port = sdp_message_m_port_get(sdp_msg, 0);/*ý�������IP�˿�*/
    printf("%s->%s:%s\r\n", sdp_msg->s_name, media_sever_ip, media_sever_port);
    csenn_eXosip_callback.csenn_eXosip_mediaControl(sdp_msg->s_name, media_sever_ip, media_sever_port);
}
/*����INFO��RTSP��Ϣ��*/
void csenn_eXosip_paraseInfoBody(eXosip_event_t *p_event)
{
    osip_body_t *p_msg_body = NULL;
    char *p_rtsp_body = NULL;
    char *p_str_begin = NULL;
    char *p_str_end = NULL;
    char *p_strstr = NULL;
    char rtsp_scale[10];
    char rtsp_range_begin[10];
    char rtsp_range_end[10];
    char rtsp_pause_time[10];

    memset(rtsp_scale, 0, 10);
    memset(rtsp_range_begin, 0, 10);
    memset(rtsp_range_end, 0, 10);
    memset(rtsp_pause_time, 0, 10);

    osip_message_get_body(p_event->request, 0, &p_msg_body);
    if(NULL == p_msg_body)
    {
        printf("osip_message_get_body null!\r\n");
        return;
    }
    p_rtsp_body = p_msg_body->body;
    printf("osip_message_get_body success!\r\n");

    p_strstr = strstr(p_rtsp_body, "PLAY");
    if(NULL != p_strstr)/*���ҵ��ַ���"PLAY"*/
    {
        /*�����ٶ�*/
        p_str_begin = strstr(p_rtsp_body, "Scale:");/*�����ַ���"Scale:"*/
        p_str_end = strstr(p_rtsp_body, "Range:");
        memcpy(rtsp_scale, p_str_begin + 6, p_str_end - p_str_begin - 6);/*����Scale��rtsp_scale*/
        printf("PlayScale:%s\r\n", rtsp_scale);
        /*���ŷ�Χ*/
        p_str_begin = strstr(p_rtsp_body, "npt=");/*�����ַ���"npt="*/
        p_str_end = strstr(p_rtsp_body, "-");
        memcpy(rtsp_range_begin, p_str_begin + 4, p_str_end - p_str_begin - 4);/*����RangeBegin��rtsp_range_begin*/
        printf("PlayRangeBegin:%s\r\n", rtsp_range_begin);
        p_str_begin = strstr(p_rtsp_body, "-");/*�����ַ���"-"*/
        strcpy(rtsp_range_end, p_str_begin + 1);/*����RangeEnd��rtsp_range_end*/
        printf("PlayRangeEnd:%s\r\n", rtsp_range_end);
        csenn_eXosip_callback.csenn_eXosip_playControl("PLAY", rtsp_scale, NULL, rtsp_range_begin, rtsp_range_end);
        return;
    }

    p_strstr = strstr(p_rtsp_body, "PAUSE");
    if(NULL != p_strstr)/*���ҵ��ַ���"PAUSE"*/
    {
        /*��ͣʱ��*/
        p_str_begin = strstr(p_rtsp_body, "PauseTime:");/*�����ַ���"PauseTime:"*/
        strcpy(rtsp_pause_time, p_str_begin + 10);/*����PauseTime��rtsp_pause_time*/
        printf("PauseTime:%3s\r\n", rtsp_pause_time);
        csenn_eXosip_callback.csenn_eXosip_playControl("PAUSE", NULL, rtsp_pause_time, NULL, NULL);
        return;
    }

    printf("can`t find string PLAY or PAUSE!");
}

/*��Ⲣ��ӡ�¼�*/
void csenn_eXosip_printEvent(eXosip_event_t *p_event)
{
    osip_message_t *clone_event = NULL;
    size_t length = 0;
    char *message = NULL;

    printf("\r\n##############################################################\r\n");
    switch(p_event->type)
    {
        //case EXOSIP_REGISTRATION_NEW:
        //    printf("EXOSIP_REGISTRATION_NEW\r\n");
        //    break;
        case EXOSIP_REGISTRATION_SUCCESS:
            printf("EXOSIP_REGISTRATION_SUCCESS\r\n");
            break;
        case EXOSIP_REGISTRATION_FAILURE:
            printf("EXOSIP_REGISTRATION_FAILURE\r\n");
            break;
        //case EXOSIP_REGISTRATION_REFRESHED:
        //    printf("EXOSIP_REGISTRATION_REFRESHED\r\n");
        //    break;
        //case EXOSIP_REGISTRATION_TERMINATED:
        //    printf("EXOSIP_REGISTRATION_TERMINATED\r\n");
        //    break;
        case EXOSIP_CALL_INVITE:
            printf("EXOSIP_CALL_INVITE\r\n");
            break;
        case EXOSIP_CALL_REINVITE:
            printf("EXOSIP_CALL_REINVITE\r\n");
            break;
        case EXOSIP_CALL_NOANSWER:
            printf("EXOSIP_CALL_NOANSWER\r\n");
            break;
        case EXOSIP_CALL_PROCEEDING:
            printf("EXOSIP_CALL_PROCEEDING\r\n");
            break;
        case EXOSIP_CALL_RINGING:
            printf("EXOSIP_CALL_RINGING\r\n");
            break;
        case EXOSIP_CALL_ANSWERED:
            printf("EXOSIP_CALL_ANSWERED\r\n");
            break;
        case EXOSIP_CALL_REDIRECTED:
            printf("EXOSIP_CALL_REDIRECTED\r\n");
            break;
        case EXOSIP_CALL_REQUESTFAILURE:
            printf("EXOSIP_CALL_REQUESTFAILURE\r\n");
            break;
        case EXOSIP_CALL_SERVERFAILURE:
            printf("EXOSIP_CALL_SERVERFAILURE\r\n");
            break;
        case EXOSIP_CALL_GLOBALFAILURE:
            printf("EXOSIP_CALL_GLOBALFAILURE\r\n");
            break;
        case EXOSIP_CALL_ACK:
            printf("EXOSIP_CALL_ACK\r\n");
            break;
        case EXOSIP_CALL_CANCELLED:
            printf("EXOSIP_CALL_CANCELLED\r\n");
            break;
        //case EXOSIP_CALL_TIMEOUT:
        //    printf("EXOSIP_CALL_TIMEOUT\r\n");
        //    break;
        case EXOSIP_CALL_MESSAGE_NEW:
            printf("EXOSIP_CALL_MESSAGE_NEW\r\n");
            break;
        case EXOSIP_CALL_MESSAGE_PROCEEDING:
            printf("EXOSIP_CALL_MESSAGE_PROCEEDING\r\n");
            break;
        case EXOSIP_CALL_MESSAGE_ANSWERED:
            printf("EXOSIP_CALL_MESSAGE_ANSWERED\r\n");
            break;
        case EXOSIP_CALL_MESSAGE_REDIRECTED:
            printf("EXOSIP_CALL_MESSAGE_REDIRECTED\r\n");
            break;
        case EXOSIP_CALL_MESSAGE_REQUESTFAILURE:
            printf("EXOSIP_CALL_MESSAGE_REQUESTFAILURE\r\n");
            break;
        case EXOSIP_CALL_MESSAGE_SERVERFAILURE:
            printf("EXOSIP_CALL_MESSAGE_SERVERFAILURE\r\n");
            break;
        case EXOSIP_CALL_MESSAGE_GLOBALFAILURE:
            printf("EXOSIP_CALL_MESSAGE_GLOBALFAILURE\r\n");
            break;
        case EXOSIP_CALL_CLOSED:
            printf("EXOSIP_CALL_CLOSED\r\n");
            break;
        case EXOSIP_CALL_RELEASED:
            printf("EXOSIP_CALL_RELEASED\r\n");
            break;
        case EXOSIP_MESSAGE_NEW:
            printf("EXOSIP_MESSAGE_NEW\r\n");
            break;
        case EXOSIP_MESSAGE_PROCEEDING:
            printf("EXOSIP_MESSAGE_PROCEEDING\r\n");
            break;
        case EXOSIP_MESSAGE_ANSWERED:
            printf("EXOSIP_MESSAGE_ANSWERED\r\n");
            break;
        case EXOSIP_MESSAGE_REDIRECTED:
            printf("EXOSIP_MESSAGE_REDIRECTED\r\n");
            break;
        case EXOSIP_MESSAGE_REQUESTFAILURE:
            printf("EXOSIP_MESSAGE_REQUESTFAILURE\r\n");
            break;
        case EXOSIP_MESSAGE_SERVERFAILURE:
            printf("EXOSIP_MESSAGE_SERVERFAILURE\r\n");
            break;
        case EXOSIP_MESSAGE_GLOBALFAILURE:
            printf("EXOSIP_MESSAGE_GLOBALFAILURE\r\n");
            break;
        //case EXOSIP_SUBSCRIPTION_UPDATE:
        //    printf("EXOSIP_SUBSCRIPTION_UPDATE\r\n");
        //    break;
        //case EXOSIP_SUBSCRIPTION_CLOSED:
        //    printf("EXOSIP_SUBSCRIPTION_CLOSED\r\n");
        //    break;
        case EXOSIP_SUBSCRIPTION_NOANSWER:
            printf("EXOSIP_SUBSCRIPTION_NOANSWER\r\n");
            break;
        case EXOSIP_SUBSCRIPTION_PROCEEDING:
            printf("EXOSIP_SUBSCRIPTION_PROCEEDING\r\n");
            break;
        case EXOSIP_SUBSCRIPTION_ANSWERED:
            printf("EXOSIP_SUBSCRIPTION_ANSWERED\r\n");
            break;
        case EXOSIP_SUBSCRIPTION_REDIRECTED:
            printf("EXOSIP_SUBSCRIPTION_REDIRECTED\r\n");
            break;
        case EXOSIP_SUBSCRIPTION_REQUESTFAILURE:
            printf("EXOSIP_SUBSCRIPTION_REQUESTFAILURE\r\n");
            break;
        case EXOSIP_SUBSCRIPTION_SERVERFAILURE:
            printf("EXOSIP_SUBSCRIPTION_SERVERFAILURE\r\n");
            break;
        case EXOSIP_SUBSCRIPTION_GLOBALFAILURE:
            printf("EXOSIP_SUBSCRIPTION_GLOBALFAILURE\r\n");
            break;
        case EXOSIP_SUBSCRIPTION_NOTIFY:
            printf("EXOSIP_SUBSCRIPTION_NOTIFY\r\n");
            break;
        //case EXOSIP_SUBSCRIPTION_RELEASED:
        //    printf("EXOSIP_SUBSCRIPTION_RELEASED\r\n");
        //    break;
        case EXOSIP_IN_SUBSCRIPTION_NEW:
            printf("EXOSIP_IN_SUBSCRIPTION_NEW\r\n");
            break;
        //case EXOSIP_IN_SUBSCRIPTION_RELEASED:
        //    printf("EXOSIP_IN_SUBSCRIPTION_RELEASED\r\n");
        //    break;
        case EXOSIP_NOTIFICATION_NOANSWER:
            printf("EXOSIP_NOTIFICATION_NOANSWER\r\n");
            break;
        case EXOSIP_NOTIFICATION_PROCEEDING:
            printf("EXOSIP_NOTIFICATION_PROCEEDING\r\n");
            break;
        case EXOSIP_NOTIFICATION_ANSWERED:
            printf("EXOSIP_NOTIFICATION_ANSWERED\r\n");
            break;
        case EXOSIP_NOTIFICATION_REDIRECTED:
            printf("EXOSIP_NOTIFICATION_REDIRECTED\r\n");
            break;
        case EXOSIP_NOTIFICATION_REQUESTFAILURE:
            printf("EXOSIP_NOTIFICATION_REQUESTFAILURE\r\n");
            break;
        case EXOSIP_NOTIFICATION_SERVERFAILURE:
            printf("EXOSIP_NOTIFICATION_SERVERFAILURE\r\n");
            break;
        case EXOSIP_NOTIFICATION_GLOBALFAILURE:
            printf("EXOSIP_NOTIFICATION_GLOBALFAILURE\r\n");
            break;
        case EXOSIP_EVENT_COUNT:
            printf("EXOSIP_EVENT_COUNT\r\n");
            break;
        default:
            printf("..................\r\n");
            break;
    }
    osip_message_clone(p_event->request, &clone_event);
    osip_message_to_str(clone_event, &message, &length);
    printf("%s\r\n", message);
    printf("##############################################################\r\n\r\n");
}

/*��Ϣѭ������*/
void csenn_eXosip_processEvent(void)
{
    eXosip_event_t *g_event = NULL;/*��Ϣ�¼�*/
    osip_message_t *g_answer = NULL;/*�����ȷ����Ӧ��*/

    while(1)
    {
        /*�ȴ�����Ϣ�ĵ���*/
        g_event = eXosip_event_wait(g_p_eXosip_context,0, 200);/*������Ϣ�ĵ���*/
        eXosip_lock(g_p_eXosip_context);
        eXosip_default_action(g_p_eXosip_context,g_event);
        eXosip_automatic_refresh(g_p_eXosip_context);/*Refresh REGISTER and SUBSCRIBE before the expiration delay*/
        eXosip_unlock(g_p_eXosip_context);
        if(NULL == g_event)
        {
            continue;
        }
        csenn_eXosip_printEvent(g_event);
        /*��������Ȥ����Ϣ*/
        switch(g_event->type)
        {
            /*��ʱ��Ϣ��ͨ��˫���������Ƚ�������*/
            case EXOSIP_MESSAGE_NEW:/*MESSAGE:MESSAGE*/
            {
                printf("\r\n<EXOSIP_MESSAGE_NEW>\r\n");
                if(MSG_IS_MESSAGE(g_event->request))/*ʹ��MESSAGE����������*/
                {
                    /*�豸����*/
                    /*�����¼�֪ͨ�ͷַ�������֪ͨ��Ӧ*/
                    /*�����豸��Ϣ��ѯ*/
                    /*�豸����Ƶ�ļ�����*/
                    printf("<MSG_IS_MESSAGE>\r\n");
                    eXosip_lock(g_p_eXosip_context);
                    eXosip_message_build_answer(g_p_eXosip_context,g_event->tid, 200, &g_answer);/*Build default Answer for request*/
                    eXosip_message_send_answer(g_p_eXosip_context,g_event->tid, 200, g_answer);/*���չ���ظ�200OK*/
                    printf("eXosip_message_send_answer success!\r\n");
                    eXosip_unlock(g_p_eXosip_context);
                    csenn_eXosip_paraseMsgBody(g_event);/*����MESSAGE��XML��Ϣ�壬ͬʱ����ȫ�ֻỰID*/
                }
            }
            break;
            /*��ʱ��Ϣ�ظ���200OK*/
            case EXOSIP_MESSAGE_ANSWERED:/*200OK*/
            {
                /*�豸����*/
                /*�����¼�֪ͨ�ͷַ�������֪ͨ*/
                /*�����豸��Ϣ��ѯ*/
                /*�豸����Ƶ�ļ�����*/
                printf("\r\n<EXOSIP_MESSAGE_ANSWERED>\r\n");
            }
            break;
            /*�������͵���Ϣ���������Ƚ�������*/
            case EXOSIP_CALL_INVITE:/*INVITE*/
            {
                printf("\r\n<EXOSIP_CALL_INVITE>\r\n");
                if(MSG_IS_INVITE(g_event->request))/*ʹ��INVITE����������*/
                {
                    /*ʵʱ����Ƶ�㲥*/
                    /*��ʷ����Ƶ�ط�*/
                    /*����Ƶ�ļ�����*/
                    osip_message_t *asw_msg = NULL;/*�����ȷ����Ӧ��*/
                    char sdp_body[4096];

                    memset(sdp_body, 0, 4096);
                    printf("<MSG_IS_INVITE>\r\n");

                    eXosip_lock(g_p_eXosip_context);
                    if(0 != eXosip_call_build_answer(g_p_eXosip_context,g_event->tid, 200, &asw_msg))/*Build default Answer for request*/
                    {
                        eXosip_call_send_answer(g_p_eXosip_context,g_event->tid, 603, NULL);
                        eXosip_unlock(g_p_eXosip_context);
                        printf("eXosip_call_build_answer error!\r\n");
                        break;
                    }
                    eXosip_unlock(g_p_eXosip_context);
                    snprintf(sdp_body, 4096, "v=0\r\n"/*Э��汾*/
                                             "o=%s 0 0 IN IP4 %s\r\n"/*�ỰԴ*//*�û���/�ỰID/�汾/��������/��ַ����/��ַ*/
                                             "s=Embedded IPC\r\n"/*�Ự��*/
                                             "c=IN IP4 %s\r\n"/*������Ϣ*//*��������/��ַ��Ϣ/������ĵ�ַ*/
                                             "t=0 0\r\n"/*ʱ��*//*��ʼʱ��/����ʱ��*/
                                             "m=video %s RTP/AVP 96\r\n"/*ý��/�˿�/���Ͳ�Э��/��ʽ�б�*/
                                             "a=sendonly\r\n"/*�շ�ģʽ*/
                                             "a=rtpmap:96 H264/90000\r\n"/*��������/������/ʱ������*/
                                             "a=username:%s\r\n"
                                             "a=password:%s\r\n"
                                             "y=100000001\r\n"
                                             "f=\r\n",
                                             device_info.ipc_id,
                                             device_info.ipc_ip,
                                             device_info.ipc_ip,
                                             device_info.ipc_port,
                                             device_info.ipc_id,
                                             device_info.ipc_pwd);
                    eXosip_lock(g_p_eXosip_context);
                    osip_message_set_body(asw_msg, sdp_body, strlen(sdp_body));/*����SDP��Ϣ��*/
                    osip_message_set_content_type(asw_msg, "application/sdp");
                    eXosip_call_send_answer(g_p_eXosip_context,g_event->tid, 200, asw_msg);/*���չ���ظ�200OK with SDP*/
                    printf("eXosip_call_send_answer success!\r\n");
                    eXosip_unlock(g_p_eXosip_context);
                }
            }
            break;
            case EXOSIP_CALL_ACK:/*ACK*/
            {
                /*ʵʱ����Ƶ�㲥*/
                /*��ʷ����Ƶ�ط�*/
                /*����Ƶ�ļ�����*/
                printf("\r\n<EXOSIP_CALL_ACK>\r\n");/*�յ�ACK�ű�ʾ�ɹ���������*/
                csenn_eXosip_paraseInviteBody(g_event);/*����INVITE��SDP��Ϣ�壬ͬʱ����ȫ��INVITE����ID��ȫ�ֻỰID*/
            }
            break;
            case EXOSIP_CALL_CLOSED:/*BEY*/
            {
                /*ʵʱ����Ƶ�㲥*/
                /*��ʷ����Ƶ�ط�*/
                /*����Ƶ�ļ�����*/
                printf("\r\n<EXOSIP_CALL_CLOSED>\r\n");
                if(MSG_IS_BYE(g_event->request))
                {
                    printf("<MSG_IS_BYE>\r\n");
                    if((0 != g_did_realPlay) && (g_did_realPlay == g_event->did))/*ʵʱ����Ƶ�㲥*/
                    {
                        /*�رգ�ʵʱ����Ƶ�㲥*/
                        printf("realPlay closed success!\r\n");
                        g_did_realPlay = 0;
                    }
                    else if((0 != g_did_backPlay) && (g_did_backPlay == g_event->did))/*��ʷ����Ƶ�ط�*/
                    {
                        /*�رգ���ʷ����Ƶ�ط�*/
                        printf("backPlay closed success!\r\n");
                        g_did_backPlay = 0;
                    }
                    else if((0 != g_did_fileDown) && (g_did_fileDown == g_event->did))/*����Ƶ�ļ�����*/
                    {
                        /*�رգ�����Ƶ�ļ�����*/
                        printf("fileDown closed success!\r\n");
                        g_did_fileDown = 0;
                    }
                    if((0 != g_call_id) && (0 == g_did_realPlay) && (0 == g_did_backPlay) && (0 == g_did_fileDown))/*����ȫ��INVITE����ID*/
                    {
                        printf("call closed success!\r\n");
                        g_call_id = 0;
                    }
                }
            }
            break;
            case EXOSIP_CALL_MESSAGE_NEW:/*MESSAGE:INFO*/
            {
                /*��ʷ����Ƶ�ط�*/
                printf("\r\n<EXOSIP_CALL_MESSAGE_NEW>\r\n");
                if(MSG_IS_INFO(g_event->request))
                {
                    osip_body_t *msg_body = NULL;

                    printf("<MSG_IS_INFO>\r\n");
                    osip_message_get_body(g_event->request, 0, &msg_body);
                    if(NULL != msg_body)
                    {
                        eXosip_call_build_answer(g_p_eXosip_context,g_event->tid, 200, &g_answer);/*Build default Answer for request*/
                        eXosip_call_send_answer(g_p_eXosip_context,g_event->tid, 200, g_answer);/*���չ���ظ�200OK*/
                        printf("eXosip_call_send_answer success!\r\n");
                        csenn_eXosip_paraseInfoBody(g_event);/*����INFO��RTSP��Ϣ��*/
                    }
                }
            }
            break;
            case EXOSIP_CALL_MESSAGE_ANSWERED:/*200OK*/
            {
                /*��ʷ����Ƶ�ط�*/
                /*�ļ�����ʱ����MESSAGE(File to end)��Ӧ��*/
                printf("\r\n<EXOSIP_CALL_MESSAGE_ANSWERED>\r\n");
            }
            break;
            /*����������Ȥ����Ϣ*/
            default:
            {
                printf("\r\n<OTHER>\r\n");
                printf("eXosip event type:%d\n", g_event->type);
            }
            break;
        }
    }
}

void csenn_eXosip_launch(void)
{
    char eXosip_server_id[30] = "34020000002000000001";
    char eXosip_server_ip[20] = "192.168.1.178";
    char eXosip_server_port[10] = "5060";
    char eXosip_ipc_id[30] = "34020000001320000001";
    char eXosip_ipc_pwd[20] = "12345678";
    char eXosip_ipc_ip[20] = "192.168.1.144";
    char eXosip_ipc_port[10] = "6000";

    char eXosip_device_name[30] = "IPC";
    char eXosip_device_manufacturer[30] = "csenn";
    char eXosip_device_model[30] = "TEST001";
    char eXosip_device_firmware[30] = "V1.0";
    char eXosip_device_encode[10] = "ON";
    char eXosip_device_record[10] = "OFF";

    char eXosip_status_on[10] = "ON";
    char eXosip_status_ok[10] = "OK";
    char eXosip_status_online[10] = "ONLINE";
    char eXosip_status_guard[10] = "OFFDUTY";
    char eXosip_status_time[30] = "2013-05-08T13:12:20";

    device_info.server_id = eXosip_server_id;
    device_info.server_ip = eXosip_server_ip;
    device_info.server_port = eXosip_server_port;
    device_info.ipc_id = eXosip_ipc_id;
    device_info.ipc_pwd = eXosip_ipc_pwd;
    device_info.ipc_ip = eXosip_ipc_ip;
    device_info.ipc_port = eXosip_ipc_port;

    device_info.device_name = eXosip_device_name;
    device_info.device_manufacturer = eXosip_device_manufacturer;
    device_info.device_model = eXosip_device_model;
    device_info.device_firmware = eXosip_device_firmware;
    device_info.device_encode = eXosip_device_encode;
    device_info.device_record = eXosip_device_record;

    device_status.status_on = eXosip_status_on;
    device_status.status_ok = eXosip_status_ok;
    device_status.status_online = eXosip_status_online;
    device_status.status_guard = eXosip_status_guard;
    device_status.status_time = eXosip_status_time;

    csenn_eXosip_callback.csenn_eXosip_getDeviceInfo(&device_info);
    while(csenn_eXosip_init());
    while(csenn_eXosip_register(3600));
    csenn_eXosip_processEvent();
}

/******************* (C) COPYRIGHT 2013 CSENN ******************END OF FILE****/