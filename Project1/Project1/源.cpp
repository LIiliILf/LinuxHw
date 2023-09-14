#include <cstdio>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

//ʹ��opendir�ӵ�2��ͷ�ļ�
#include <sys/types.h>
#include <dirent.h>

#define MSG_TYPE_LOGIN
#define MSG_TYPE_FILENAME

typedef struct msg
{
    int type; //Э������ 0��¼Э����� 1�ļ��������
    int flag;
    char buffer[128]; //��ų��ļ���֮�������
    char fname[50]; //���type��1���ļ������������ôfname����ʹ�����ļ���
}MSG; //����ṹ������ҵ������Ĳ��ϱ仯������µ��ֶ�


void* thread_fun(void* arg)
{
    int acpt_socket = *((int*)arg);
    int res;
    char buffer[50] = {0};
    search_server_dir(acpt_socket);
    printf("Ŀ¼��Ϣ���͸��ͻ������\n");

}

//�������̿ͻ��˵�ҵ�����󣬿ͻ�����Ҫ�鿴���������Ŀ¼�µ��ļ�����Ϣ��
//������ʵ��һ�����ܣ���ĳ��Ŀ¼�µ��ļ�����Ϣȫ��ȡ����Ȼ�󷢸��ͻ��ˡ�
//Ĭ������·�������Ŀ¼����Ϊ�û��� /home/ Ŀ¼
//��linux����ζ��ļ���Ŀ¼���ж�ȡ����ȡ�ļ���
void search_server_dir(int accept_socket)
{
    //opendir�Ǵ�linuxĿ¼��api����
    struct dirent* dir = NULL; //����һ�����Ŀ¼��Ϣ�Ľṹ��
    MSG info_msg = { 0 };
    int res = 0;

    DIR* dp = opendir("/home/ub");
    info_msg.type = 
    if (NULL = dp)
    {
        perror("open dir error:");
        return 0;
    }
    while (1)
    {
        dir = readdir(dp);
        if (NULL == dir) //������ؿ�ֵ����ʾĿ¼ȫ����ȡ��ɡ�
        {
            break;
        }
        if (dir->d_name[0] != '.') //�������ļ�����
        {
            printf("name=%s\n", dir->d_name);
            memset(info_msg.fname, 0, sizeof(info_msg.fname));
            strcpy(info_msg.fname, dir.->d_name);
            res = write(accept_socket,&info_msg,sizeof(MSG));   //��ÿ���ļ���������info_msg�ṹ���У�ͨ���׽��ַ��ͳ�ȥ
            if (res < 0)
            {
                perror("send client error:");
                return;
            }
        }

    }
}


int main()
{
    //1.���ֻ� 2��绰�� 3 �绰�������ֻ� 4 �����ֻ�����
    int server_socket; //�����socket������������Ҳ�����׽�����������ͨ���������
    int accept_socket;
    char buffer[50] = { 0 };
    int res = 0;
    pthread_t thread_id;//�̱߳��

        //1.�����׽���������
        printf("��ʼ����tcp������\n");
    server_socket = socket(AF_INET, SOCKET_STREAM, 0);//�Ժ�Ҫ���������������ݶ�ʹ��server socket����׽���������
    //ֻҪ����ʽ��һ���Ĳ���

    if (server_socket < 0)
    {
        perror("socket create failed:");
        return 0;
    }
    //2.Ҫ����������������ҵ�ip��ַ�Ͷ˿ںš����ĵ�����һ������ip��ַ�Ͷ˿ڵı��� �Ӽ���ͷ�ļ�
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; //INADDR_ANY�궨�壬����ϵͳ�Զ�������ip
    server_addr.sin_port = htons��6666��;//ͨ�������ַת��Э��htons,�������ֽ�˳��ת���������ֽ�˳��

    //��������������˳������뿪�򿪷���������ϵͳ����ʾAddress already in use
    //������Ϊip��ַ�Ͷ˿ں���ϵͳ��Դ���������� �˿ںſ����ظ�ʹ��
    int optvalue = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &optvalue, sizeof(optvalue);


    //3.�������趨�õ�ip��ַ�Ͷ˿ںŰ󶨵����ǵ�server_socket�������ϡ� �� bind
    if (bind(server_socket, (struct sockaddr_in*)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("server bind error:");
        return 0;
    }
    //4.����listen��ʼ��������  man listen   
    if (listen(server_socket, 10) < 0)
    {
        perror("server listen error:");
        return 0;
    }
    //5.����4������ok�˾Ϳ��Եȴ��ͻ������ӹ����� man 2 accept
    // �����ǳ�������������ʱ�����û�пͻ������ӵ�����������ô���������������
    // ֱ���пͻ������ӵ��������ϣ��������������һ���µ��׽�������������ô���ںͿͻ��˵�ͨѶ����������µ��׽���������������
    printf("tcp������׼����ɣ��ȴ��ͻ�������\n");

 
    while (1)
    {
        accept_socket = accept(server_socket, NULL, NULL);
        printf("�пͻ������ӵ�������\n");
        //����һ���µ��߳�,
        pthread_create(&thread_id,NULL,thread_fun,& accept_socket);

        //read�������ǽ��տͻ��˷��������ݣ�����ֵ��ʾʵ�ʴӿͻ����Ǳ��յ����ֽ�����
        //buffer�����յ��ͻ������ݺ�����ݴ�ŵĵ�ַ��sizeof(buffer)����ϣ����ȡ���ֽ�����
        res = read(accept_socket, buffer, sizeof(buffer));
        printf("client read %s\n", buffer);
        write(accept_socket, buffer, res);
        memset(buffer, 0, sizeof(buffer));
        //�������յ��ͻ������ݺ�ԭ�ⲻ���������ٻ���ͻ��ˡ�

    }
    printf("%s �����ʺ�!\n", "Linux_c_sample");
    return 0;
}