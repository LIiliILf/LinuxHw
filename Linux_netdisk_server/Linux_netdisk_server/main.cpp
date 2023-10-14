#include <cstdio>
#include <sys/types.h>          /* socket */
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>            /* thread */
#include <dirent.h>             /* opendir */
#include <sys/stat.h>           /* open */
#include <fcntl.h>

#define MSG_TYPE_LOGIN 0
#define MSG_TYPE_FILENAME 1
#define MSG_TYPE_DOWNLOAD 2
#define MSG_TYPE_UPLOAD 3
#define MSG_TYPE_UPLOAD_DATA 4
//通讯协议的定制，两边要保持一致
typedef struct msg {
    int type; //协议类型 0登录协议包；1文件名传输包；2文件下载包
    int flag; //标志位
    char buffer[1024]; //存放除文件名之外的内容
    char fname[50]; //如果type是1，文件名传输包，那么fname里面就存放着文件名
    int bytes;  //这个字段用来记录传输文件时每个数据包实际的文件字节数
}MSG; //这个结构体会根据业务需求的不断变化，添加新的字段

void search_server_dir(int accept_socket){//把套接字作为参数传进来
    struct dirent* dir = NULL; //定义一个存放目录信息的结构体
    MSG info_msg = { 0 };
    int res = 0;
    DIR* dp = opendir("/home/linux");    //opendir是打开linux目录的api函数
    info_msg.type = MSG_TYPE_FILENAME;
    printf("文件目录发送完成\n");
    if (NULL == dp){
        perror("open dir error:");
        return;
    }
    while (1){
        dir = readdir(dp);
        if (NULL == dir){ //如果返回空值，表示目录全部读取完成。
            break;
        }
        if (dir->d_name[0] != '.'){ //把.隐藏文件屏蔽
            memset(info_msg.fname, 0, sizeof(info_msg.fname));  //处理前先清空一下
            strcpy(info_msg.fname, dir->d_name); //d_name拷贝到fname 
            res = write(accept_socket, &info_msg, sizeof(MSG));   //把每个文件名拷贝到info_msg结构体中，通过套接字发送出去
            if (res < 0){
                perror("send client error:");
                return;
            }
        }
    }
}
//download通过打开服务器中的某个文件，并使用socket网络发送给客户端，先把文件定为1.txt
void server_file_download(int accept_socket){
    MSG file_msg = { 0 };
    int res = 0;
    int fd;//文件描述符，linux系统下很重要的概念，linux认为所有设备都是文件，所有都通过文件描述符打开。文件的打开、对设备的读写
    fd = open("/home/linux/testdownload.txt", O_RDONLY);//man open
    if (fd < 0) {
        perror("file open error");
        return;
    }
    file_msg.type = MSG_TYPE_DOWNLOAD;
    strcpy(file_msg.fname, "testdownload.txt");
    //在读取文件并把文件传到客户端的时候，MSG结构体中的buffer就是存放文件的内容，但是一般来说文件都超过1024字节 所以要发送多个包。
    while ((res = read(fd, file_msg.buffer, sizeof(file_msg.buffer))) > 0) {// 当read用于读取文件的时候,每次读到文件末尾之后将返回小于0
        file_msg.bytes = res;        //res 是实际读取的文件字节数
        write(accept_socket, &file_msg, sizeof(MSG));
        if (res <= 0){
            perror("server send file error");
        }
        memset(file_msg.buffer, 0, sizeof(file_msg.buffer));
    }
}

/*原来用buffer 改成用结构体*/
void* thread_fun(void* arg){
    int acpt_socket = *((int*)arg);     //accpet传递过来后，需要把它取出来,强制转换成指针
    int res;
    char buffer[50] = { 0 };
    int fd = -1;    // 文件描述符
    char up_file_name[20] = { 0 };
    MSG recv_msg = { 0 };   //从buffer缓冲区转换成结构体大小
    while (1){
        res = read(acpt_socket, &recv_msg, sizeof(MSG));
        if (res == 0) {
            printf("客户端已经断开\n");
            break;
        }
        if (recv_msg.type == MSG_TYPE_FILENAME){//说明客户端发过来的信息要的是目录
            search_server_dir(acpt_socket);
            memset(&recv_msg, 0, sizeof(MSG));
        }
        else if (recv_msg.type == MSG_TYPE_DOWNLOAD){
            server_file_download(acpt_socket);
            memset(&recv_msg, 0, sizeof(MSG));
        }
        else if (recv_msg.type == MSG_TYPE_UPLOAD) { //准备接收客户端发来的文件数据
            //要从数据包的文件名里面获取文件名信息，然后创建文件，放到默认创建的文件夹，在home目录下。
            strcpy(up_file_name, recv_msg.fname);
            //然后在home目录下创建文件
            fd = open("/home/linux/testupload.txt", O_CREAT | O_WRONLY | O_TRUNC, 0666);
            if (fd < 0){
                perror("create uploadfile error");
            }
            //server_file_upload(acpt_socket);
            //memset(&recv_msg, 0, sizeof(MSG));
        }
        else if (recv_msg.type == MSG_TYPE_UPLOAD_DATA) {
            res = write(fd, recv_msg.buffer, recv_msg.bytes);   //从buffer中写出来,写的字节数是recv_msg.bytes
            if (recv_msg.bytes < sizeof(recv_msg.buffer)) { //最后一部分数据
                printf("client upload file completed\n");
                close(fd);
                fd = -1;
            }
            memset(&recv_msg, 0, sizeof(MSG));
        }

        memset(&recv_msg, 0, sizeof(MSG));
    }
}
int main(){
    char buffer[50] = { 0 };
    int res = 0;
    printf("开始创建tcp服务器\n");
    int server_socket; 
    int accept_socket;
    pthread_t thread_id;    //线程编号
    //1.使用socket创建一个TCP流式套接字
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0){
        perror("socket create error");
        return 0;
    }
    //2.告诉服务器ip地址和端口号。
    struct sockaddr_in server_addr;     //IPv4套接口地址数据结构
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; //自动绑定网卡ip
    server_addr.sin_port = htons(6666);//通过网络地址转换协议htons,把主机字节顺序转换成网络字节顺序
    int optvalue = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &optvalue, sizeof(optvalue));   //端口复用
    //3.用bind()将ip地址和端口号绑定到server_socket描述符
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
        perror("server bind error:");
        return 0;
    }
    //4.调用listen监听程序 
    if (listen(server_socket, 10) < 0){
        perror("server listen error:");
        return 0;
    }
    //5. 等待客户端连接 调用accept()接收客户端的连接请求
    /*当我们程序调用这个函数时，如果没有客户端连接到服务器上那么这个函数将阻塞。
     直到有客户端连接到服务器上，解除阻塞并返回一个新的套接字描述符，那么后期和客户端的通讯都交给这个新的套接字描述符来负责*/
    printf("tcp服务器准备完成，等待客户端连接\n");

    /*多线程 把accept放到while里面然后创建线程
    创建成功后系统会执行thread_fun()里面的代码，会和主线程代码一起执行*/
    while (1){
        accept_socket = accept(server_socket, NULL, NULL);
        printf("有客户端连接到服务器\n");
        pthread_create(&thread_id, NULL, thread_fun, &accept_socket);
    }

	printf("%s 向你问好!\n", "Linux_netdisk_server");
	return 0;
}