#include <cstdio>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/stat.h>//mkdir
#include <fcntl.h>
#include <errno.h>

#define MSG_TYPE_LOGIN 0
#define MSG_TYPE_FILENAME 1
#define MSG_TYPE_DOWNLOAD 2
#define MSG_TYPE_UPLOAD 3
#define MSG_TYPE_UPLOAD_DATA 4
//通讯协议的定制，两边要保持一致，拷贝
typedef struct msg
{
    int type; //协议类型 0登录协议包； 1文件名传输包；
    int flag; //标志位
    char buffer[1024]; //存放除文件名之外的内容
    char fname[50]; //如果type是1，文件名传输包，那么fname里面就存放着文件名
    int bytes;
}MSG; //这个结构体会根据业务需求的不断变化，添加新的字段

int fd = 0; // 这个是用来打开文件进行读写的文件描述符。默认为0表示还没打开
char up_file_name[20] = { 0 };
void net_disk_ui()
{
    printf("==========TCP网盘客户端================\n");
    printf("============功能菜单===================\n");
    printf("\t\t\t1.查询文件\n");
    printf("\t\t\t2.下载文件\n");
    printf("\t\t\t3.上传文件\n");
    printf("\t\t\t0.退出系统\n");
    printf("-----------------------------------------\n");
    printf("请选择对应的操作:");
}

void* upload_file_thread(void* args)
{
    //客户端实现上传文件到服务器思路
    //1.打开文件
    MSG up_file_msg = { 0 };
    char buffer[1024] = { 0 };//用来保存读取文件的缓冲区数据
    int client_socket = *((int*)args);
    int fd = -1;
    int res = 0;
    fd = open("./download/css.txt", O_RDONLY);
    if (fd < 0)
    {
        perror("open up file error");
        return NULL;
    }
    up_file_msg.type = MSG_TYPE_UPLOAD_DATA;
    //2.读取文件内容
    while((res = read(fd,buffer,sizeof(buffer))) > 0)
    {
        //要把文件数据内容拷贝到msg结构体中的buffer中
        memcpy(up_file_msg.buffer, buffer, res);
        up_file_msg.bytes = res;
        res = write(client_socket, &up_file_msg, res);
        memset(buffer, 0, sizeof(buffer));
        memset(up_file_msg.buffer, 0, sizeof(up_file_msg.buffer));
        
    }
}

//client也要有个创建线程。接收服务器发送过来的数据 0916_0853
void* thread_func(void* arg)
{
    int clinet_socket = *((int*)arg);
    MSG recv_msg = { 0 };
    int res;
    while (1)
    {
        //用来接收服务器端发过来的数据
        res = read(clinet_socket, &recv_msg, sizeof(MSG));

        if (recv_msg.type == MSG_TYPE_FILENAME)
        {
            printf("server path filename%s \n", recv_msg.fname);
            memset(&recv_msg, 0, sizeof(MSG));
        }
        else if (recv_msg.type == MSG_TYPE_DOWNLOAD)    //说明服务器端发过来的一定是文件，做好接收准备
        {
            //1.要确定在哪个目录下，可以创建一个目录mkdir函数 download目录里
            if (mkdir("download", S_IRWXU) < 0)
            {
                if (errno == EEXIST) {
                    // printf("dir  exist continue\n");
                }
                else
                {
                    perror("mkdir error");
                    // return;
                }
            }
            //目录创建没问题之后，开始创建文件
            if (fd == -1) //表示文件还没打开过
            {
                // O_CREATE表示这个文件不存在，要重新创建
                fd = open("./download/1.txt", O_CREAT | O_WRONLY,0666);//打开成功后
                if (fd < 0)
                {
                    perror("file open error");
                }
            }
            //通过上面的创建目录，以及文件描述符的判断通过后就可以从MSG结构体里面的buffer部分取数据
            //recv_msg.buffer存放的是文件的部分内容，recv_msg.bytes是这个部分文件的字节数
            res = write(fd, recv_msg.buffer, recv_msg.bytes); 
            if (res < 0)
            {
                perror("file write error");
            }
            //那么如何判断文件内容都全部发完了呢 可以通过recv_msg_bytes 如果小于recv_buffer的1024说明发完了
            if (recv_msg.bytes < sizeof(recv_msg.buffer))
            {
                printf("file download finish\n");
                close(fd);
                fd = -1;
            }


        }
    }
}
int main()
{
    int client_socket;
    pthread_t thread_id;
    pthread_t thread_send_id;
    struct sockaddr_in server_addr; //用来填写要连接的服务器的ip地址和端口号
    char buffer[50] = {0};
    int res;

    char c; //用来保存用户在键盘上输入的字符
    //0916_0859把recv改成send，为什么不是单独创建一个send
    //MSG recv_msg = { 0 };
    MSG send_msg = { 0 };
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0)
    {
        perror("client socket failed:");
        return 0;
    }
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("192.168.226.16"); //一种是直接指定，另一种如果s和c在同一台电脑中可以用127.0.0.1
    server_addr.sin_port = htons(6666);

    //创建好套接字之后，客户端要连接到服务器，这时候使用connect连接服务器 man connect
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)    //& server_addr 取地址，然后强制转换
    {
        perror("connect error");
        return 0;
    }
    printf("client连接server成功\n");
    //连接成功之后创建线程，对应创建线程id
    pthread_create(&thread_id, NULL, thread_func, &client_socket);

    //0916_9点02
    while (1)
    {
        net_disk_ui();
        c = getchar(); //获取键盘输入
        switch (c)
        {
        case '1':
            //要让服务器给我们发送目录信息
            //这个while循环本身也是死循环，所以客户端也要创建线程，让接收服务器数据的代码放进线程里。
            send_msg.type = MSG_TYPE_FILENAME;
            res = write(client_socket, &send_msg, sizeof(MSG));
            if (res < 0)
            {
                perror("send msg error:");
            }
            memset(&send_msg, 0, sizeof(MSG));
            break;
        case '2':
            send_msg.type = MSG_TYPE_DOWNLOAD;
            res = write(client_socket, &send_msg, sizeof(MSG));
            if (res < 0)
            {
                perror("send msg error:");
            }
            memset(&send_msg, 0, sizeof(MSG));
            break;
            //0837
        case '3':
            send_msg.type == MSG_TYPE_UPLOAD;
            printf("input up laod filename:");
            puts(up_file_name);
            //在上传文件之前，先发送一个数据包给服务器，告诉服务器这边准备上传文件了
            strcpy(send_msg.fname,up_file_name);
            res = write(client_socket, &send_msg, sizeof(MSG));
            if (res < 0)
            {
                perror("send upload package error:");
                continue;
            }
            memset(&send_msg, 0, sizeof(MSG));
            //考虑到上传文件需要比较长时间，这个时候写在这里，那么可能导致其他功能卡住排队等待，
            //因此需要把发送文件内容的代码放进线程里面
            pthread_create(&thread_send_id, NULL, upload_file_thread, &client_socket);

            break;
        case '0':
            return 0;
        }
    }
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
    
    //改造 0916为什么又没有这一段了。上课没有说，但是翻到这里看到没有
   /*
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
    */
    return 0;
}