#include <cstdio>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

//使用opendir加的2个头文件
#include <sys/types.h>
#include <dirent.h>

#define MSG_TYPE_LOGIN
#define MSG_TYPE_FILENAME

typedef struct msg
{
    int type; //协议类型 0登录协议包； 1文件传输包；
    int flag;
    char buffer[128]; //存放除文件夹之外的内容
    char fname[50]; //如果type是1，文件名传输包，那么fname里面就存放着文件名
}MSG; //这个结构体会根据业务需求的不断变化，添加新的字段


void* thread_fun(void* arg)
{
    int acpt_socket = *((int*)arg);
    int res;
    char buffer[50] = {0};
    search_server_dir(acpt_socket);
    printf("目录信息发送给客户端完成\n");

}

//根据网盘客户端的业务需求，客户端想要查看服务器这边目录下的文件名信息。
//服务器实现一个功能：把某个目录下的文件名信息全获取出来然后发给客户端。
//默认情况下服务器的目录设置为用户的 /home/ 目录
//在linux下如何对文件和目录进行读取并获取文件名
void search_server_dir(int accept_socket)
{
    //opendir是打开linux目录的api函数
    struct dirent* dir = NULL; //定义一个存放目录信息的结构体
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
        if (NULL == dir) //如果返回空值，表示目录全部读取完成。
        {
            break;
        }
        if (dir->d_name[0] != '.') //把隐藏文件屏蔽
        {
            printf("name=%s\n", dir->d_name);
            memset(info_msg.fname, 0, sizeof(info_msg.fname));
            strcpy(info_msg.fname, dir.->d_name);
            res = write(accept_socket,&info_msg,sizeof(MSG));   //把每个文件名拷贝到info_msg结构体中，通过套接字发送出去
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
    //1.买手机 2买电话卡 3 电话卡插入手机 4 带上手机接收
    int server_socket; //这个是socket网络描述符，也叫做套接字描述符，通过这个发送
    int accept_socket;
    char buffer[50] = { 0 };
    int res = 0;
    pthread_t thread_id;//线程编号

        //1.创建套接字描述符
        printf("开始创建tcp服务器\n");
    server_socket = socket(AF_INET, SOCKET_STREAM, 0);//以后要想向互联网发送数据都使用server socket这个套接字描述符
    //只要是流式都一样的步骤

    if (server_socket < 0)
    {
        perror("socket create failed:");
        return 0;
    }
    //2.要告诉这个服务器，我的ip地址和端口号。看文档，有一个保存ip地址和端口的变量 加几个头文件
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; //INADDR_ANY宏定义，告诉系统自动绑定网卡ip
    server_addr.sin_port = htons（6666）;//通过网络地址转换协议htons,把主机字节顺序转换成网络字节顺序

    //如果服务器程序退出后，又离开打开服务器程序，系统会提示Address already in use
    //这是因为ip地址和端口号是系统资源，必须设置 端口号可以重复使用
    int optvalue = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &optvalue, sizeof(optvalue);


    //3.把我们设定好的ip地址和端口号绑定到我们的server_socket描述符上。 查 bind
    if (bind(server_socket, (struct sockaddr_in*)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("server bind error:");
        return 0;
    }
    //4.调用listen开始监听程序  man listen   
    if (listen(server_socket, 10) < 0)
    {
        perror("server listen error:");
        return 0;
    }
    //5.以上4个步骤ok了就可以等待客户端连接过来了 man 2 accept
    // 当我们程序调用这个函数时，如果没有客户端连接到服务器上那么这个函数将阻塞。
    // 直到有客户端连接到服务器上，解除阻塞并返回一个新的套接字描述符，那么后期和客户端的通讯都交给这个新的套接字描述符来负责
    printf("tcp服务器准备完成，等待客户端连接\n");

 
    while (1)
    {
        accept_socket = accept(server_socket, NULL, NULL);
        printf("有客户端连接到服务器\n");
        //创建一个新的线程,
        pthread_create(&thread_id,NULL,thread_fun,& accept_socket);

        //read函数就是接收客户端发来的数据，返回值表示实际从客户端那边收到的字节数。
        //buffer就是收到客户端数据后把数据存放的地址，sizeof(buffer)就是希望读取的字节数。
        res = read(accept_socket, buffer, sizeof(buffer));
        printf("client read %s\n", buffer);
        write(accept_socket, buffer, res);
        memset(buffer, 0, sizeof(buffer));
        //服务器收到客户端数据后，原封不动把数据再会给客户端。

    }
    printf("%s 向你问好!\n", "Linux_c_sample");
    return 0;
}