/******************** (C) COPYRIGHT 2013 CSENN *********************************
* File Name          : csenn_eXosip.c
* Author             : www.csenn.com
* Date               : 2013/05/08
**************************************************************START OF FILE****/
#include <eXosip2/eXosip.h>
#include <osip2/osip_mt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <WinSock2.h>
#include <sys/types.h>

struct _device_info                 // device infomation struct
{
    char *server_id;                // SIP server ID, default: 34020000002000000001
    char *server_ip;                // SIP server IP, default: 192.168.1.178
    char *server_port;              // SIP server port, default: 5060

    char *ipc_id;                   /*ý����������ID*//*Ĭ��ֵ��34020000001180000002*/
    char *ipc_pwd;                  /*ý��������������*//*Ĭ��ֵ��12345678*/
    char *ipc_ip;                   /*ý����������IP��ַ*//*Ĭ��ֵ��192.168.1.144*/
    char *ipc_port;                 /*ý����������IP�˿�*//*Ĭ��ֵ��6000*/

    char *device_name;              /*�豸/����/ϵͳ����*//*Ĭ��ֵ��IPC*/
    char *device_manufacturer;      /*�豸����*//*Ĭ��ֵ��csenn*/
    char *device_model;             /*�豸�ͺ�*//*Ĭ��ֵ��YF1109*/
    char *device_firmware;          /*�豸�̼��汾*//*Ĭ��ֵ��V1.0*/
    char *device_encode;            /*�Ƿ����*//*ȡֵ��Χ��ON/OFF*//*Ĭ��ֵ��ON*/
    char *device_record;            /*�Ƿ�¼��*//*ȡֵ��Χ��ON/OFF*//*Ĭ��ֵ��OFF*/
}device_info;
struct _device_status               /*�豸״̬�ṹ��*/
{
    char *status_on;                /*�豸��״̬*//*ȡֵ��Χ��ON/OFF*//*Ĭ��ֵ��ON*/
    char *status_ok;                /*�Ƿ���������*//*ȡֵ��Χ��OK/ERROR*//*Ĭ��ֵ��OFF*/
    char *status_online;            /*�Ƿ�����*//*ȡֵ��Χ��ONLINE/OFFLINE*//*Ĭ��ֵ��ONLINE*/
    char *status_guard;             /*����״̬*//*ȡֵ��Χ��ONDUTY/OFFDUTY/ALARM*//*Ĭ��ֵ��OFFDUTY*/
    char *status_time;              /*�豸���ں�ʱ��*//*��ʽ��xxxx-xx-xxTxx:xx:xx*//*Ĭ��ֵ��2012-12-18T16:23:32*/
}device_status;
enum _device_control
{
    EXOSIP_CTRL_RMT_LEFT = 1,       /*����*/
    EXOSIP_CTRL_RMT_RIGHT,          /*����*/
    EXOSIP_CTRL_RMT_UP,             /*����*/
    EXOSIP_CTRL_RMT_DOWN,           /*����*/
    EXOSIP_CTRL_RMT_LARGE,          /*�Ŵ�*/
    EXOSIP_CTRL_RMT_SMALL,          /*��С*/
    EXOSIP_CTRL_RMT_STOP,           /*ֹͣң��*/
    EXOSIP_CTRL_REC_START,          /*��ʼ�ֶ�¼��*/
    EXOSIP_CTRL_REC_STOP,           /*ֹͣ�ֶ�¼��*/
    EXOSIP_CTRL_GUD_START,          /*����*/
    EXOSIP_CTRL_GUD_STOP,           /*����*/
    EXOSIP_CTRL_ALM_RESET,          /*������λ*/
    EXOSIP_CTRL_TEL_BOOT,           /*�豸Զ������*/
};

/*�ص�����*/
struct _csenn_eXosip_callback
{
    /*��ȡ�豸��Ϣ*/
    /*device_info���豸��Ϣ�ṹ��ָ��*/
    /*����ֵ���ɹ�ʱ����0��ʧ��ʱ���ظ�ֵ*/
    int(*csenn_eXosip_getDeviceInfo)(struct _device_info *device_info);

    /*��ȡ�豸״̬*/
    /*device_info���豸״̬�ṹ��ָ��*/
    /*����ֵ���ɹ�ʱ����0��ʧ��ʱ���ظ�ֵ*/
    int(*csenn_eXosip_getDeviceStatus)(struct _device_status *device_status);

    /*��ȡ¼���ļ�����ʼʱ�������ʱ��*/
    /*ʱ���ʽ��xxxx-xx-xxTxx:xx:xx*/
    /*period_start��¼��ʱ�����ʼֵ*/
    /*period_end��¼��ʱ��ν���ֵ*/
    /*start_time����ǰ����¼���ļ�����ʼʱ��*/
    /*end_time����ǰ����¼���ļ��Ľ���ʱ��*/
    /*����ֵ���ɹ�ʱ���ط���ʱ���������ʣ��¼���ļ�������ʧ��ʱ���ظ�ֵ*/
    int(*csenn_eXosip_getRecordTime)(char *period_start, char *period_end, char *start_time, char *end_time);

    /*�豸���ƣ��������ҡ����ϡ����¡��Ŵ���С��ֹͣң��/��ʼ�ֶ�¼��ֹͣ�ֶ�¼��/����������/������λ/�豸Զ������*/
    /*ctrl_cmd���豸�������_device_control���͵�ö�ٱ���*/
    /*����ֵ���ɹ�ʱ����0��ʧ��ʱ���ظ�ֵ*/
    int(*csenn_eXosip_deviceControl)(enum _device_control ctrl_cmd);

    /*ý����ƣ�ʵʱ�㲥/�ط�/����*/
    /*control_type��ý��������ͣ�ʵʱ�㲥/Play���ط�/Playback������/Download*/
    /*media_ip��ý�������IP��ַ*/
    /*media_port��ý�������IP�˿�*/
    /*����ֵ���ɹ�ʱ����0��ʧ��ʱ���ظ�ֵ*/
    int(*csenn_eXosip_mediaControl)(char *control_type, char *media_ip, char *media_port);

    /*���ſ��ƣ�����/���/����/��ͣ*/
    /*control_type�����ſ��ƣ�����/���/����/PLAY����ͣ/PAUSE*/
    /*play_speed�������ٶȣ�1Ϊ���ţ�����1Ϊ��ţ�С��1Ϊ����*/
    /*pause_time����ͣʱ�䣬��λΪ��*/
    /*range_start�����ŷ�Χ����ʼֵ*/
    /*range_end�����ŷ�Χ�Ľ���ֵ*/
    /*����ֵ���ɹ�ʱ����0��ʧ��ʱ���ظ�ֵ*/
    int(*csenn_eXosip_playControl)(char *control_type, char *play_speed, char *pause_time, char *range_start, char *range_end);
}csenn_eXosip_callback;

/*��������*/
/*������ע��eXosip*/
void csenn_eXosip_launch(void);

/*ע��eXosip*/
/*expires������ע��ĳ�ʱʱ�䣬��λΪ��*/
/*����ֵ���ɹ�ʱ����0��ʧ��ʱ���ظ�ֵ*/
int csenn_eXosip_register(int expires);

/*ע��eXosip*/
/*����ֵ���ɹ�ʱ����0��ʧ��ʱ���ظ�ֵ*/
int csenn_eXosip_unregister(void);

/*����MESSAGE������*/
/*ʱ���ʽ��xxxx-xx-xxTxx:xx:xx*/
/*alarm_time������ʱ��*/
void csenn_eXosip_sendEventAlarm(char *alarm_time);

/*����MESSAGE���ļ�����*/
void csenn_eXosip_sendFileEnd(void);
/******************** (C) COPYRIGHT 2013 CSENN *****************END OF FILE****/
