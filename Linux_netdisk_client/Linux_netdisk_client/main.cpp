#include <cstdio>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/stat.h>       /* mkdir */
#include <fcntl.h>
#include <errno.h>

#define MSG_TYPE_LOGIN 0
#define MSG_TYPE_FILENAME 1
#define MSG_TYPE_DOWNLOAD 2
#define MSG_TYPE_UPLOAD 3
#define MSG_TYPE_UPLOAD_DATA 4
//通讯协议的定制，两边要保持一致

typedef struct msg{
    int type; //协议类型 0登录协议包；1文件名传输包；
    int flag; //标志位
    char buffer[1024]; //存放除文件名之外的内容
    char fname[50]; //如果type是1，文件名传输包，那么fname里面就存放着文件名
    int bytes;  //这个字段用来记录传输文件时每个数据包实际的文件字节数
}MSG; //这个结构体会根据业务需求的不断变化，添加新的字段
int fd = -1; // 这个用来打开文件进行读写的文件描述符。默认为-1表示还没打开
char up_file_name[20] = { 0 };
void net_disk_ui() {
    //在服务器连接成功之后调用一下
    system("clear");
    printf("==========TCP网盘客户端================\n");
    printf("============功能菜单===================\n");
    printf("\t\t\t1.查询文件\n");
    printf("\t\t\t2.下载文件\n");
    printf("\t\t\t3.上传文件\n");
    printf("\t\t\t0.刷新页面\n");
    printf("-----------------------------------------\n");
    printf("请选择对应的操作");
}
/*考虑到上传文件比较慢，把需要发送文件内容的代码放进线程中*/
void* upload_file_thread(void* args) {    //客户端实现上传文件到服务器思路
    //1.打开文件
    MSG up_file_msg = { 0 };
    char buffer[1024] = { 0 };//用来保存读取文件的缓冲区数据
    int client_socket = *((int*)args);
    int fd = -1;
    int res = 0;
    fd = open("./download/testupload.txt", O_RDONLY);
    if (fd < 0){
        perror("open uploadfile error");
        return NULL;
    }
    up_file_msg.type = MSG_TYPE_UPLOAD_DATA;
    //2.读取文件内容
    while ((res = read(fd, buffer, sizeof(buffer))) >0) {       
        memcpy(up_file_msg.buffer, buffer, res);    //把文件数据拷贝到msg结构体中的buffer中
        up_file_msg.bytes = res;
        res = write(client_socket, &up_file_msg, sizeof(MSG));  
        memset(buffer, 0, sizeof(buffer));
        memset(up_file_msg.buffer, 0, sizeof(up_file_msg.buffer));
    }
    // 2. 读取文件内容
    //while ((res = read(fd, up_file_msg.buffer, sizeof(up_file_msg.buffer))) > 0) {
    //    up_file_msg.bytes = res;
    //    res = write(client_socket, &up_file_msg, sizeof(MSG));
    //    memset(up_file_msg.buffer, 0, sizeof(up_file_msg.buffer));
    //}
}

//client也要创建线程。接收服务器发送过来的数据
void* thread_func(void* arg){
    int clinet_socket = *((int*)arg);
    MSG recv_msg = { 0 };
    fd = -1;
    int res;
    while (1){
        res = read(clinet_socket, &recv_msg, sizeof(MSG));        //用来接收服务器端发过来的数据
        if (recv_msg.type == MSG_TYPE_FILENAME){
            printf("filename:%s \n", recv_msg.fname);
            memset(&recv_msg, 0, sizeof(MSG));
        }
        else if (recv_msg.type == MSG_TYPE_DOWNLOAD)  {  //说明服务器端发过来的一定是文件，做好接收准备
            //1.要下载到哪个目录下，用mkdir()创建一个目录download
            if (mkdir("download", S_IRWXU) < 0){
                if (errno != EEXIST) {
                    perror("mkdir error");
                }
            }
            //2.创建目录后，开始创建文件
            if (fd == -1){ //表示文件还没打开过
                fd = open("./download/testdownload.txt", O_CREAT | O_WRONLY | O_TRUNC, 0666);//打开成功后    // O_CREATE表示这个文件不存在，要重新创建
                if (fd < 0){
                    perror("file open error");
                }
            }
            //通过上面的创建目录，以及文件描述符的判断通过后就可以从MSG结构体里面的buffer部分取数据
            res = write(fd, recv_msg.buffer, recv_msg.bytes);            //recv_msg.buffer存放的是文件的部分内容，recv_msg.bytes是这个部分文件的字节数
            if (res < 0){
                perror("file write error");
            }
            //那么如何判断文件内容都全部发完了呢 可以通过recv_msg_bytes 如果小于recv_buffer的1024说明发完了
            if (recv_msg.bytes < sizeof(recv_msg.buffer)){
                printf("file download finish\n");
                close(fd);
                fd = -1;
            }
        }
    }
}
int main(){
    int client_socket;
    struct sockaddr_in server_addr; //用来填写要连接的服务器的ip地址和端口号
    char buffer[50] = { 0 };
    int res;
    char c;     //c用来保存用户在键盘上输入的操作数字
    pthread_t thread_id;
    pthread_t thread_send_id;
    char up_file_name[20] = { 0 };
    MSG send_msg = { 0 };
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0){
        perror("client socket failed:");
        return 0;
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("192.168.85.16"); //直接指定或者如果s和c在同一台电脑中可以用127.0.0.1
    server_addr.sin_port = htons(6666);

    //客户端使用connect()连接服务器 man connect
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {   //& server_addr 取地址，然后强制转换
        perror("connect error");
        return 0;
    }
    printf("client连接server成功\n");
    net_disk_ui();
    pthread_create(&thread_id, NULL, thread_func, &client_socket);

    /*原来的发送数据进行改造*/
    while (1){
        c = getchar(); //获取键盘输入
        switch (c){
        case '1':
            send_msg.type = MSG_TYPE_FILENAME;
            res = write(client_socket, &send_msg, sizeof(MSG));
            if (res < 0){
                perror("send msg error:");
            }
            memset(&send_msg, 0, sizeof(MSG));
            net_disk_ui();
            break;
        case '2':
            send_msg.type = MSG_TYPE_DOWNLOAD;
            res = write(client_socket, &send_msg, sizeof(MSG));
            if (res < 0){
                perror("send msg error:");
            }
            memset(&send_msg, 0, sizeof(MSG));
            net_disk_ui();
            break;
        case '3':
            send_msg.type = MSG_TYPE_UPLOAD;
            printf("input upload filename:");
            puts(up_file_name);
            //scanf("%s", up_file_name);
            strcpy(send_msg.fname, up_file_name);   //上传文件前要先发送一个数据包给服务器，告诉服务器准备上传文件了
            res = write(client_socket, &send_msg, sizeof(MSG));
            if (res < 0) {
                perror("send upload package error");
                continue;
            }
            memset(&send_msg, 0, sizeof(MSG));
            pthread_create(&thread_send_id, NULL, upload_file_thread, &client_socket);
            break;
        case '0':
            return 0;
        }
    }

    printf("%s 向你问好!\n", "Linux_netdisk_client");
    return 0;
}