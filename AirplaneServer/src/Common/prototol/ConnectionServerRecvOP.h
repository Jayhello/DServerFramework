#ifndef _LOGICSERVER_2_CONNECTIONSERVER_H
#define _LOGICSERVER_2_CONNECTIONSERVER_H

/*  �߼����������͸����ӷ�������OP    */
enum CONNECTIONSERVER_RECV_OP
{
    CONNECTION_SERVER_RECV_LOGICSERVER_LOGIN,           /*  �յ�logic server ��½����   */
    CONNECTION_SERVER_RECV_PING,
    CONNECTION_SERVER_RECV_PACKET2CLIENT_BYSOCKINFO,    /*  �߼�������������Ϣ�������(������ҵ�cs id��socket id)   */
    CONNECTION_SERVER_RECV_PACKET2CLIENT_BYRUNTIMEID,   /*  �߼�������������Ϣ�����(������ҵ�RuntimeID)    */
    CONNECTION_SERVER_RECV_IS_SETCLIENT_SLAVEID,        /*  ��Ϸ�������������ӷ�����������ҵ�Slave ID*/
    CONNECTION_SERVER_RECV_KICKCLIENT_BYRUNTIMEID,      /*  �ڲ������������߳�ĳ���    */
};

#endif