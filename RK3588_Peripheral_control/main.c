#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <string.h>
#include <sys/ioctl.h>
#include<time.h>
#include <errno.h>
#include <pthread.h>
#include<sys/time.h>


#include <arpa/inet.h>
#include <sys/socket.h>


#define ERR_MSG(err_code) do {                                     \
    err_code = errno;                                              \
    fprintf(stderr, "ERROR code: %d \n", err_code);                \
    perror("PERROR message");                                      \
} while (0)



#define GPIO_EXPORT "/sys/class/gpio/export"
#define GPIO_UNEXPORT "/sys/class/gpio/unexport"

#define led_Yellow 491 
#define led_Red 450 
#define led_Green 485 
#define buzzer 510 
#define fire 444 
#define jiujing 445 
#define TRIG_PIN 449
#define ECHO_PIN 492

pthread_t sendHcThread;      
pthread_t connectTcpThread; 
pthread_t buzzerThread; 
pthread_t sr04Thread;       //超声波线程
pthread_t monitoringThread; //监控线程


const int BUF_LEN = 100;
int cli_socket_fd = 0;
float distance = 0;
char send_buff[128]={'\0'};
int mark_HC_Open = 0;
pthread_mutex_t mutex; //定义互斥量（锁）

//导出GPIO函数
void exportPin(int pin) {
	char pinStr[4];
	sprintf(pinStr, "%d", pin);

	int fd = open(GPIO_EXPORT, O_WRONLY);
	if (fd == -1) {
		printf("Failed to open GPIO export file.\n");
		exit(1);
	}

	write(fd, pinStr, strlen(pinStr));
	close(fd);
}
//释放GPIO函数
void unexportPin(int pin) {
	char pinStr[4];
	sprintf(pinStr, "%d", pin);

	int fd = open(GPIO_UNEXPORT, O_WRONLY);
	if (fd == -1) {
		printf("Failed to open GPIO unexport file.\n");
		exit(1);
	}

	write(fd, pinStr, strlen(pinStr));
	close(fd);
}
//设置GPIO方向函数
void setPinDirection(int pin, const char *direction) {
	char pinDir[100];
	sprintf(pinDir, "/sys/class/gpio/gpio%d/direction", pin);

	int fd = open(pinDir, O_WRONLY);
	if (fd == -1) {
		printf("Failed to open pin %d\n", pin);
		exit(1);
	}

	write(fd, direction, strlen(direction));
	close(fd);
}
//设置GPIO输出值函数
void setPinValue(int pin, int value) {
	char pinValue[100];
	sprintf(pinValue, "/sys/class/gpio/gpio%d/value", pin);

	int fd = open(pinValue, O_WRONLY);
	if (fd == -1) {
		printf("Failed to open pin %d\n", pin);
		exit(1);
	}

	char strValue[2];
	sprintf(strValue, "%d", value);
	write(fd, strValue, strlen(strValue));
	close(fd);
}
//读取GPIO输入值函数
int readPinValue(int pin) {
	char pinValue[100];
	sprintf(pinValue, "/sys/class/gpio/gpio%d/value", pin);

	int fd = open(pinValue, O_RDONLY);
	if (fd == -1) {
		printf("Failed to open pin %d\n", pin);
		exit(1);
	}

	char value;
	read(fd, &value, sizeof(value));
	close(fd);

	return (value == '1') ? 1 : 0;
}

void gpio_Init()
{
	exportPin(buzzer);     
		
	exportPin(fire);           
	exportPin(jiujing);    
	exportPin(led_Yellow);          
	exportPin(led_Red);           
	exportPin(led_Green);    

	setPinDirection(jiujing, "in");
	setPinDirection(buzzer, "out");
	setPinDirection(fire, "out");
	setPinValue(fire, 0);
	setPinDirection(fire, "in");
	
	setPinDirection(led_Yellow, "out");
	setPinDirection(led_Red, "out");
	setPinDirection(led_Green, "out");
	setPinValue(led_Yellow, 0);
	setPinValue(led_Red, 0);
	setPinValue(led_Green, 0);
	
}


void startHc()
{
         setPinValue(TRIG_PIN, 0);// 设置GPIO引脚输出高电平
         usleep(5);
         setPinValue(TRIG_PIN, 1);// 设置GPIO引脚输出低电平
         usleep(10);
         setPinValue(TRIG_PIN, 0);// 设置GPIO引脚输出低电平
}


void *sr04_thread(void *datas) //温湿度线程
{
    int pinValue;
    sleep(2);
    exportPin(TRIG_PIN);   // 导出GPIO引脚        
    setPinDirection(TRIG_PIN, "out");// 设置GPIO引脚方向为输出 
    exportPin(ECHO_PIN);   // 导出GPIO引脚        
    setPinDirection(ECHO_PIN, "in"); 
    printf("初始化成功\n");
	
	mark_HC_Open = 1;
    while (1) {
        // 发送触发信号
	startHc();
        // 等待接收回波
        struct timeval startTime, endTime;
		
        while (readPinValue(ECHO_PIN) == 0);
        {
        gettimeofday(&startTime, NULL);
        }

        while (readPinValue(ECHO_PIN)== 1);
        {
        gettimeofday(&endTime, NULL);
        }
		
        // 计算距离（单位：厘米）
       long difftime = 1000000*(endTime.tv_sec-startTime.tv_sec)+(endTime.tv_usec-startTime.tv_usec);//计算时间，单位为微秒
       if((difftime < 60000) && (difftime > 1))
       {
       distance = (double)difftime /1000000 *34000/2;//计算距离  单位是cm，声音速度为34000cm每秒，一来一回所以要除以2

        printf("Distance: %.2f cm\n", distance);
        usleep(1000000);  // 每隔1秒进行一次测量
       
    	}
    }	

    return 0;
}

void *sendHc_thread(void *datas) 
{
	while(1){
		
		if(mark_HC_Open == 1){
			sprintf(send_buff,"Distance: %.2f cm\n",distance);
			send(cli_socket_fd, send_buff, BUF_LEN, 0);
			sleep(1);
			memset(send_buff,'\0',128);
		}
	}
}

void *buzzer_thread(void *datas) //警报线程
{
    exportPin(buzzer);   // 导出GPIO引脚        
	exportPin(fire);   // 导出GPIO引脚        
	exportPin(jiujing);   // 导出GPIO引脚        
	setPinDirection(jiujing, "in");
	setPinDirection(buzzer, "out");// 设置GPIO引脚方向为输出 
	setPinDirection(fire, "out");// 设置GPIO引脚方向为输出 
	setPinValue(fire, 0);
	setPinDirection(fire, "in"); 
	while(1){
		//pinValue = readPinValue(jiujing);
		
		if(readPinValue(jiujing) == 0 ){
			setPinValue(buzzer, 1);// 设置GPIO引脚输出高电平

		}
		else if(readPinValue(fire) == 0){
			setPinValue(buzzer, 1);// 设置GPIO引脚输出高电平

		}
		else{

			setPinValue(buzzer, 0);
		}
		sleep(1);
	}
}


void *connectTCP_thread(void *datas) 
{
	/* 配置 Server Sock 信息。*/
    struct sockaddr_in srv_sock_addr;
    memset(&srv_sock_addr, 0, sizeof(srv_sock_addr));
    srv_sock_addr.sin_family = AF_INET;
    srv_sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);  // 即 0.0.0.0 表示监听本机所有的 IP 地址。
    srv_sock_addr.sin_port = htons(5000);      //监听端口号

    /* 创建 Server Socket。*/
    int srv_socket_fd = 0;
    if (-1 == (srv_socket_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP))) {
        printf("Create socket file descriptor ERROR.\n");
        ERR_MSG(errno);
        exit(EXIT_FAILURE);
    }
    /* 设置 Server Socket 选项。*/
    int optval = 1;
    if (setsockopt(srv_socket_fd,
                   SOL_SOCKET,    // 表示套接字选项的协议层。
                   SO_REUSEADDR,  // 表示在绑定地址时允许重用本地地址。这样做的好处是，当服务器进程崩溃或被关闭时，可以更快地重新启动服务器，而不必等待一段时间来释放之前使用的套接字。
                   &optval,
                   sizeof(optval)) < 0)
    {
        printf("Set socket options ERROR.\n");
        ERR_MSG(errno);
        exit(EXIT_FAILURE);
    }

    /* 绑定 Socket 与 Sock Address 信息。*/
    if (-1 == bind(srv_socket_fd,
                   (struct sockaddr *)&srv_sock_addr,
                   sizeof(srv_sock_addr)))
    {
        printf("Bind socket ERROR.\n");
        ERR_MSG(errno);
        exit(EXIT_FAILURE);
    }

    /* 开始监听 Client 发出的连接请求。*/
    if (-1 == listen(srv_socket_fd, 10))
    {
        printf("Listen socket ERROR.\n");
        ERR_MSG(errno);
        exit(EXIT_FAILURE);
    }

    /* 初始化 Client Sock 信息存储变量。*/
    struct sockaddr cli_sock_addr;
    memset(&cli_sock_addr, 0, sizeof(cli_sock_addr));
    int cli_sockaddr_len = sizeof(cli_sock_addr);

    cli_socket_fd = 0;

    int recv_len = 0;
    char recv_buff[] = {0};

    /* 永远接受 Client 的连接请求。*/
   
        if (-1 == (cli_socket_fd = accept(srv_socket_fd,
                                          (struct sockaddr *)(&cli_sock_addr),  // 填充 Client Sock 信息。
                                          (socklen_t *)&cli_sockaddr_len)))
        {
            printf("Accept connection from client ERROR.\n");
            ERR_MSG(errno);
            exit(EXIT_FAILURE);
        }
		pthread_create(&sendHcThread, NULL, sendHc_thread, NULL);   
 while (1)
    {
        /* 接收指定 Client Socket 发出的数据，*/
        memset(recv_buff, 0, BUF_LEN); 
        if ((recv_len = recv(cli_socket_fd, recv_buff, BUF_LEN, 0)) <= 0)
        {
            printf("Client quit\n");
            //ERR_MSG(errno);
			pthread_cancel(sendHcThread);
			if (-1 == (cli_socket_fd = accept(srv_socket_fd,
                                          (struct sockaddr *)(&cli_sock_addr),  // 填充 Client Sock 信息。
                                          (socklen_t *)&cli_sockaddr_len)))
			{
				printf("Accept connection from client ERROR.\n");
				ERR_MSG(errno);
				exit(EXIT_FAILURE);
			}
			pthread_create(&sendHcThread, NULL, sendHc_thread, NULL); 
			continue;

        }
		
		if(strcmp(recv_buff,"openRed") == 0){
			printf("red open\n");
			setPinValue(led_Red, 1);// 设置GPIO引脚输出高电平
			
		}
		
		else if(strcmp(recv_buff,"openGreen") == 0){
			printf("green open\n");
			setPinValue(led_Green, 1);// 设置GPIO引脚输出高电平
			
		}
		
	    else if(strcmp(recv_buff,"openYellow") == 0){
			printf("yellow open\n");
			setPinValue(led_Yellow, 1);// 设置GPIO引脚输出高电平
			
		}
		
		else if(strcmp(recv_buff,"openBuzzer") == 0){
			pthread_create(&buzzerThread, NULL, buzzer_thread, NULL); 
			setPinValue(buzzer, 1);// 设置GPIO引脚输出高电平
			
		} 
		
		else if(strcmp(recv_buff,"openHC") == 0){
			printf("HC open\n");					
			pthread_create(&sr04Thread, NULL, sr04_thread, NULL); // 超声波线程启动
			
		}
		
		else if(strcmp(recv_buff,"closeGreen") == 0){
			setPinValue(led_Green, 0);
			printf("green close\n");
			
		}
		
		else if(strcmp(recv_buff,"closeRed") == 0){
			setPinValue(led_Red, 0);
			printf("red close\n");
			
		}
		
		else if(strcmp(recv_buff,"closeYellow") == 0){
			setPinValue(led_Yellow, 0);
			printf("yellow close\n");
			
		}
		
		else if(strcmp(recv_buff,"closeBuzzer") == 0){
			pthread_cancel(buzzerThread);
			printf("buzzer close\n");
			setPinValue(buzzer, 0);
			
		}
		
		else if(strcmp(recv_buff,"closeHC") == 0){
			printf("HC close\n");
			mark_HC_Open = 0;
			pthread_cancel(sr04Thread);
			
		}
		
        printf("Receive data from client: %s\n", recv_buff);

    }

   // close(srv_socket_fd);
}


void *monitoring_thread(void *datas) //视频监控线程
{
    system("/usr/local/bin/mjpg_streamer -i \"/usr/local/lib/mjpg-streamer/input_uvc.so -n -f 30 -r 1280x720\" -o \"/usr/local/lib/mjpg-streamer/output_http.so -p 8080 -w /usr/local/share/mjpg-streamer/www\"");//执行脚本，打开视频监控
    pthread_exit(NULL);             //退出线程
}

const char default_path[] = "/dev/ttyAMA1";//根据具体的设备修改
int main(int argc, char *argv[])
{
	int fd;
	int res;
	char *path;
	char txbuf[512] = "This is tty send test.\n";
	char rxbuf[1024];

	//若无输入参数则使用默认终端设备
	if (argc > 1)
		path = argv[1];
	else
		path = (char *)default_path; //获取串口设备描述符
	fd = open(path, O_RDWR);//打开串口
	if (fd < 0) {
		printf("Fail to Open %s device\n", path);
		return 0;
	}

	/* 设置串口参数 */
	struct termios opt; 
	tcflush(fd, TCIFLUSH);//清空串口接收和发送缓冲区
	tcflush(fd, TCOFLUSH);
	tcgetattr(fd, &opt); // 获取串口参数 opt
	cfsetospeed(&opt, B115200); //设置串口输出波特率
	cfsetispeed(&opt, B115200);//设置串口输入波特率
	opt.c_cflag &= ~CSIZE;//设置数据位数
	opt.c_cflag |= CS8;
	opt.c_cflag &= ~PARENB;//校验位
	opt.c_iflag &= ~INPCK;
	opt.c_cflag &= ~CSTOPB;//设置停止位
	tcsetattr(fd, TCSANOW, &opt);//更新配置
	printf("Device %s is set to 115200bps,8N1\n",path);


	if(fcntl(fd,F_SETFL,0) < 0)//阻塞， 即使前面在open串口设备时设置的是非阻塞的，这里设为阻塞后，以此为准
	{
		printf("fcntl failed\n");
	}
	else{
		printf("fcntl=%d\n", fcntl(fd,F_SETFL,0));
	}
	fcntl(fd, F_SETFL, 0);  //串口阻塞

	/* 收发程序 */
	write(fd, txbuf, strlen(txbuf));


	gpio_Init();

	pthread_create(&monitoringThread, NULL, monitoring_thread, NULL); // 监控线程启动
	pthread_create(&connectTcpThread, NULL, connectTCP_thread, NULL);
	
	
	
	while(1)
	{

		tcflush(fd, TCIFLUSH);//清空串口接收和发送缓冲区
		res = read(fd, rxbuf, sizeof(rxbuf)); //接收字符串
		if (res >0 ) 
		{
			rxbuf[res] = '\0';  //给接收到的字符串加结束符
			// printf("Receive res = %d bytes data: %s\n",res,rxbuf);
			printf("%s\n",rxbuf);      
			write(fd, "ok!\n", 4);  //回应发送字符串ok!
			//tcflush(fd, TCOFLUSH);
			switch(*rxbuf){
				case 'A':
					printf("red open\n");
					setPinValue(led_Red, 1);// 设置GPIO引脚输出高电平
					break;	

				case 'B':
					printf("green open\n");
					setPinValue(led_Green, 1);// 设置GPIO引脚输出高电平
					break;

				case 'C':
					printf("yellow open\n");
					setPinValue(led_Yellow, 1);// 设置GPIO引脚输出高电平
					break;
			
				case 'D':
					printf("buzzer open\n");
					pthread_create(&buzzerThread, NULL, buzzer_thread, NULL); 
					break;

				case 'E':
					printf("HC open\n");					
					pthread_create(&sr04Thread, NULL, sr04_thread, NULL); // 超声波线程启动
					break;

				case 'F':
					setPinValue(led_Green, 0);
					printf("green close\n");
					break;

				case 'G':
					setPinValue(led_Red, 0);
					printf("red close\n");
					break;

				case 'H':
					setPinValue(led_Yellow, 0);
					printf("yellow close\n");
					break;

				case 'I':
					printf("buzzer close\n");
					pthread_cancel(buzzerThread);
					setPinValue(buzzer, 0)
					break;

				case 'J':
					printf("HC close\n");
					mark_HC_Open = 0;
					pthread_cancel(sr04Thread);
					break;
			}
			
			res = 0;
		}	
		usleep(20000);  
		memset(rxbuf,'\0',1024);
	} 

	printf("read error,res = %d",res);

	close(fd);
	return 0;

}



