#include <cstdio>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>

#define MSG_TYPE_LOGIN 0
#define MSG_TYPE_FILENAME 1
//通讯协议的定制，两边要保持一致，拷贝
typedef struct msg
{
    int type; //协议类型 0登录协议包； 1文件名传输包；
    int flag; //标志位
    char buffer[128]; //存放除文件名之外的内容
    char fname[50]; //如果type是1，文件名传输包，那么fname里面就存放着文件名
}MSG; //这个结构体会根据业务需求的不断变化，添加新的字段

//网盘
void net_disk_ui()
{
    //在服务器连接成功之后调用一下
    system("clear");
    printf("==========TCP网盘客户端================\n");
    printf("============功能菜单===================\n");
    printf("\t\t\t1.查询文件\n");
    printf("\t\t\t2.下载文件\n");
    printf("\t\t\t3.上传文件\n");
    printf("\t\t\t0.退出系统\n");
    printf("-----------------------------------------\n");
    printf("请选择对应的操作");
    
}


int main()
{
    int client_socket;
    struct sockaddr_in server_addr; //用来填写要连接的服务器的ip地址和端口号
    char buffer[50] = {0};
    int res;

    MSG recv_msg = { 0 };

    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0)
    {
        perror("client socket failed:");
        return 0;
    }
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("192.168.226.16"); //一种是直接指定，另一种如果s和c在同一台电脑中可以用127.0.0.1
    server_addr.sin_port = htons(7777);

    //创建好套接字之后，客户端要连接到服务器，这时候使用connect连接服务器 man connect
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)    //& server_addr 取地址，然后强制转换
    {
        perror("connect error");
        return 0;
    }


    printf("client连接server成功\n");
    net_disk_ui();


    //客户端往服务器发送数据，用户再客户端终端连续输入字符串，回车表示发送数据。用while循环
    //while (fgets(buffer, sizeof(buffer), stdin ) != NULL )
    //{
    //    res =  write(client_socket,buffer,sizeof(buffer));//把buffer里面所有的数据发送给服务器,用write往服务器发送数据。 man write
    //    printf("send bytes = %d\n", res);
    //    memset(buffer, 0, sizeof(buffer));
    //    res = read(client_socket, buffer, sizeof(buffer));
    //    printf("receive from server info:%s\n",buffer);
    //    memset(buffer, 0, sizeof(buffer));
    //}
    
    //改造
   
    while (1)
    {
        res = read(client_socket, &recv_msg, sizeof(MSG));
        if (recv_msg.type == MSG_TYPE_FILENAME)
        {
            printf("server path filename = %s\n", recv_msg.fname);
            memset(&recv_msg,0,sizeof(MSG));
        }
    }
    
    printf("%s 向你问好!\n", "Linux_disk_net_client");
    return 0;
}