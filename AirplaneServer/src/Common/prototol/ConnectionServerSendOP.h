#ifndef _CONNECTIONSERVER_2_LOGICSERVER_H
#define _CONNECTIONSERVER_2_LOGICSERVER_H

/*  ���ӷ��������͸��߼�����������Ϣ��   */
enum CONNECTION_SERVER_SEND_OP
{
    CONNECTION_SERVER_SEND_PONG,
    CONNECTION_SERVER_SEND_LOGICSERVER_CLIENT_DISCONNECT,   /*  ���ӷ�����֪ͨ�߼�������ĳ�����������(��Ҫ�ȴ���������) */
    //����user msg �д��� (����ping��CONNECTION_SERVER_SEND_LOGICSERVER_CLIENT_RECONNECT,    /*  ���ӷ�����֪ͨ�߼�������ĳ��Ҷ�����������   */
    CONNECTION_SERVER_SEND_LOGICSERVER_RECVCSID,            /*  ���ӷ�����������id���͸�solo������    */
    CONNECTION_SERVER_SEND_LOGICSERVER_INIT_CLIENTMIRROR,   /*  ֪ͨsolo��������ʼ��ĳ���session  */
    CONNECTION_SERVER_SEND_LOGICSERVER_DESTROY_CLIENT,      /*  ֪ͨ�߼�������ǿ��ɾ�����   */
    CONNECTION_SERVER_SEND_LOGICSERVER_FROMCLIENT,          /*  ���ӷ�����ת����ҵ���Ϣ�����߼������� */
};

#endif