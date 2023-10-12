#include <cstdio>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>
#include <dirent.h> //opendir
#include <sys/stat.h>//open
#include <fcntl.h>


#define MSG_TYPE_LOGIN 0
#define MSG_TYPE_FILENAME 1
#define MSG_TYPE_DOWNLOAD 2
#define MSG_TYPE_UPLOAD 3
#define MSG_TYPE_UPLOAD_DATA 4

//通讯协议的定制，两边要保持一致，拷贝过去
typedef struct msg
{
    int type; //协议类型 0登录协议包； 1文件名传输包；2文件下载包
    int flag; //标志位
    char buffer[1024]; //存放除文件名之外的内容
    char fname[50]; //如果type是1，文件名传输包，那么fname里面就存放着文件名
    int bytes;  //这个字段用来记录传输文件时每个数据包实际的文件字节数
}MSG; //这个结构体会根据业务需求的不断变化，添加新的字段

//根据网盘客户端的业务需求，客户端想要查看服务器这边目录下的文件名信息。
//服务器实现一个功能：把某个目录下的文件名信息全获取出来然后发给客户端。
//默认情况下服务器的目录设置为用户的 /home/ 目录
//search实现在linux下如何对文件和目录进行读取并获取文件名
void search_server_dir(int accept_socket)//把套接字作为参数传进来
{
    //opendir是打开linux目录的api函数
    struct dirent* dir = NULL; //定义一个存放目录信息的结构体
    MSG info_msg = {0};
    int res =0;
    DIR* dp = opendir("/home/linux");
    info_msg.type = MSG_TYPE_FILENAME;
    if (NULL == dp)
    {
        perror("open dir error:");
        return;
    }
    //用while获取
    while (1)
    {
        dir = readdir(dp);
        if (NULL == dir) //如果返回空值，表示目录全部读取完成。
        {
            break;
        }

        if (dir->d_name[0] != '.') //把.隐藏文件屏蔽
        {
            //printf("name=%s\n", dir->d_name); 
            memset(info_msg.fname, 0, sizeof(info_msg.fname));  //处理前先清空一下
            strcpy(info_msg.fname, dir->d_name); //d_name拷贝到fname 
            res = write(accept_socket, &info_msg, sizeof(MSG));   //把每个文件名拷贝到info_msg结构体中，通过套接字发送出去
            if (res < 0)
            {
                perror("send client error:");
                return;
            }
        }
    }
}

//0916_0953 这个函数通过打开服务器中的某个文件，并使用socket网络发送给客户端，先把文件定为 HiBurn.exe,根据实际情况我自己新建了test.txt
void server_file_download(int accept_socket)
{
    MSG file_msg = { 0 };
    int res = 0;
    int fd;//文件描述符，linux系统下很重要的概念，linux认为所有设备都是文件，所有都通过文件描述符打开。文件的打开、对设备的读写
    fd = open("/home/linux/1.txt",O_RDONLY);//man open
    if (fd < 0) {
        perror("file open error");
        return;
    }
    file_msg.type = MSG_TYPE_DOWNLOAD;
    strcpy(file_msg.fname,"1.txt");
    //在读取文件并把文件传到客户端的时候，MSG结构体中的buffer就是存放文件的内容，但是一般来说文件都超过1024字节
    //所以要发送多个包。而且这个MSG结构中type类型
    while ((res = read(fd, file_msg.buffer, sizeof(file_msg.buffer))) > 0)// 当read用于读取文件的时候,每次读到文件末尾之后将返回小于0
    {
        //res 是实际读取的文件字节数
        file_msg.bytes = res;
        write(accept_socket, &file_msg, sizeof(MSG));
        if (res <= 0)
        {
            perror("server send file error");

        }
        memset(file_msg.buffer, 0, sizeof(file_msg.buffer));
    }
}

//多客户端连接问题
void* thread_fun(void* arg)
{
    //accpet传递过来后，需要把它取出来
    int acpt_socket = *((int*)arg); //强制转换成指针
    int res;
    char buffer[50] = { 0 };
    char up_file_name[20] = { 0 };
    int fd = -1;       //定义一个文件描述符
    MSG recv_msg = { 0 };   //从buffer缓冲区转换成结构体大小
    // 0916_0848 等到按查询文件的时候才 search 注释掉,如果在这里调用那么不输入1就直接打印出目录了
    //search_server_dir(acpt_socket);
    printf("目录信息发送给客户端完成\n");
    while (1)
    {
        //read函数就是接收客户端发来的数据，返回值表示实际从客户端那边收到的字节数。
        //buffer就是收到客户端数据后把数据存放的地址，sizeof(buffer)就是希望读取的字节数。
        res = read(acpt_socket, &recv_msg, sizeof(MSG));
        //0916_08:44服务器read问题  说明客户端已经断开服务器的连接了，read默认情况下他是阻塞模式
        if (res == 0) {
            printf("客户端已经断开\n");
            break;
        }
        if (recv_msg.type == MSG_TYPE_FILENAME)//说明客户端发过来的信息要的是目录
        {
            search_server_dir(acpt_socket);
            memset(&recv_msg, 0, sizeof(MSG));
        }
        else if (recv_msg.type == MSG_TYPE_DOWNLOAD)
        {
            server_file_download(acpt_socket);
            memset(&recv_msg, 0, sizeof(MSG));
        }
        else if (recv_msg.type == MSG_TYPE_UPLOAD)//准备接收客户端发来的文件数据
        {
            //要从数据包的文件名里面获取文件名信息，然后创建文件，放到默认创建的文件夹，在home目录下。
            strcpy(up_file_name, recv_msg.fname);
                //然后在home目录下创建文件
            fd = open("/home/linux/css.txt", O_CREAT | O_WRONLY, 0666);
            if (fd < 0)
            {
                perror("create file error");
            }
            //server_file_upload(acpt_socket);
            memset(&recv_msg, 0, sizeof(MSG));
        }
        else if (recv_msg.type == MSG_TYPE_UPLOAD_DATA)
        {
            //从buffer里面写出来
            res = write(fd, recv_msg.buffer, recv_msg.bytes);
            if(recv_msg.bytes < sizeof(recv_msg.buffer))
            {//说明这个部分数据是文件的最后一部分数据
                printf("client up file ok\n");
                close(fd);
            }
            memset(&recv_msg, 0, sizeof(MSG));
        }
        //printf("client read %s\n", buffer); //0916 0903
        //write(acpt_socket, buffer, res);
        //memset(buffer, 0, sizeof(buffer));
        memset(&recv_msg, 0, sizeof(MSG));
        //服务器收到客户端数据后，原封不动把数据再回发给客户端。
    }
}

int main()
{
    char buffer[50] = { 0 };
    int res = 0;
    //1.买手机 2买电话卡 3 电话卡插入手机 4 带上手机接收
    printf("开始创建tcp服务器\n");
    int server_socket; //socket网络描述符，也叫做套接字描述符，用于向网络发送数据。
    int accept_socket;
    pthread_t thread_id; //线程编号
    //1.创建套接字描述符
    server_socket = socket(AF_INET, SOCK_STREAM, 0);//以后要想向互联网发送数据都使用server socket这个套接字描述符
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
    server_addr.sin_port = htons(6666);//通过网络地址转换协议htons,把主机字节顺序转换成网络字节顺序

    //如果我们服务器程序退出后，又立刻打开服务器程序，系统会提示 Address already in used.
    //因为ip地址和端口号是资源资源，必须设置为端口号可以重复使用
    int optvalue = 1;
    setsockopt(server_socket,SOL_SOCKET,SO_REUSEADDR,&optvalue,sizeof(optvalue));

    //3.把我们设定好的ip地址和端口号绑定到我们的server_socket描述符上。 查 bind
    if (bind(server_socket, (struct sockaddr*) & server_addr, sizeof(server_addr)) < 0)
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
    //5.以上4个步骤ok了就可以等待客户端连接过来了 man accept
    // 当我们程序调用这个函数时，如果没有客户端连接到服务器上那么这个函数将阻塞。
    // 直到有客户端连接到服务器上，解除阻塞并返回一个新的套接字描述符，那么后期和客户端的通讯都交给这个新的套接字描述符来负责
    printf("tcp服务器准备完成，等待客户端连接\n");

    //主线程中，一旦有客户端连接就创建一个线程，就可以解决多客户端连接的问题。
    while (1)
    {
        accept_socket = accept(server_socket, NULL, NULL);
        printf("有客户端连接到服务器\n");
        //把accept放到while里面，然后创建线程
        //创建成功之后，系统就会执行thread_fun里面的代码。而这里面的代码是多线程代码（会和主线程代码一起执行）
        pthread_create(&thread_id, NULL, thread_fun, &accept_socket);   //thread_fun函数写在上面
    }
    printf("%s 向你问好!\n", "Linux_c_sample");
    return 0;
}