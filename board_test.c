#include <stdlib.h>
#include "ui.h"
#include "test.h"
#include "event.h"
#include "process.h"
#include "event_queue.h"
#include "i2c-dev.h"
#include <string.h>
#include "console.h"
//#include "bluetooth.h"
//#include "bluedroidtest.c"
#include "board_test.h"
#include <linux/videodev2.h>
//#include <linux/videodev2_samsung.h>

#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

//#include "cutils/sockets.h"

//#include <hardware/audio.h>
//#include <system/audio.h>
//#include <tinyalsa/asoundlib.h>

#ifndef pr_info
#define pr_info(fmt, args...)  printf("[board_test]: "fmt, ##args)
#endif


char g_uart_recv_buf[100] = "";
char g_uart_recv_syscall[100] = "";
int g_uart_fd = -1;
int g_virt_uart_fd = -1;
char g_board_ext_info[50];
int g_board_test_mode = 0;
int g_uart_test_mode = 0;
int g_testmode = 1;
int row = 0, col = 0, test_num = 0;

// uart cmd & parameters
char g_cmd[32];
int g_param1 = 0;
int g_param2 = 0;
int g_param3 = 0;
int g_param4 = 0;
char g_param_sn[64];
char g_param_fg[64];
char g_param_group_sn[64];
char g_param_modem2_sn[64];
char g_param_imei[19];
char g_param_btant[128];
char g_product_model[16];
char device_name[PROPERTY_VALUE_MAX];
char g_swversion[64];
char g_inner_swversion[64];
int g_fuse_sdcard = 0;
int g_encrypted = 0;
char g_param_hWID[64];
#define BMP18X_CHIP_ID_REG		0xD0
#define BMP18X_CHIP_ID			0x55

#define MPU6500_ID               0x74      /* unique WHOAMI */

#define MPU6500_ID_REG           0x75

#define MAX_BUF_SIZE    2048
#define SOCKET_VCD "/dev/socket/vcd_factory_stream"

#define SENSORHUB_TCMD "/sys/class/cywee_sensorhub/sensor_hub/calibrator_cmd"
#define SENSORHUB_TRES "/sys/class/cywee_sensorhub/sensor_hub/calibrator_data"
#define SENSORHUB_CMD_LEN       64

static struct win board_test_win =
{
	.title = "Board Test",
	.cur = 0,
	.n = 0,
};
static struct win format_win =
{
	.title = "partition",
	.cur = 0,
	.n = 0,
};

int gps_test(void)
{

        int fd;
        char nmea[10];
        int count;
        int ret = FAIL;

	system("echo '$pglirm,start_brm,/system/etc/gpsconfig.xml,Factory_High_SNR' > /data/gps/glgpsctrl");
        sleep(3);
        fd = open("/data/gps/gpspipe", O_RDONLY);

        if (fd < 0){
                ret = FAIL;
        }
        else{
                count = read(fd, nmea, 10);
                if(count > 0)
                        ret = OK;
                else
                        ret = FAIL;
        }
	close(fd);
        sleep(3);
        system("echo '$pglirm,stop_brm' > /data/gps/glgpsctrl");
        return ret;
}

int gps_test_open(void)
{
        int fd;
        char nmea[10];
        int count;
        int ret = FAIL;

	system("echo '$pglirm,start_brm,/system/etc/gpsconfig.xml,Factory_High_SNR' > /data/gps/glgpsctrl");
        sleep(3);

        fd = open("/data/gps/gpspipe", O_RDONLY);

        if (fd < 0){
                ret = FAIL;
        }
        else{
                count = read(fd, nmea, 10);
                if(count > 0)
                        ret = OK;
                else
                        ret = FAIL;
	}
	close(fd);
	return ret;
}

int gps_test_close(void)
{
	sleep(3);
	system("echo '$pglirm,stop_brm' > /data/gps/glgpsctrl");
	return OK;
}

int adb_test_on(void)
{
	int ret_id = 0;
	ret_id = system("echo 0 > /sys/class/android_usb/android0/enable");
	if(ret_id < 0)
		return FAIL;
	ret_id = system("echo 18d1 > /sys/class/android_usb/android0/idVendor");
	if(ret_id < 0)
		return FAIL;
	ret_id = system("echo 4e22 > /sys/class/android_usb/android0/idProduct");
	if(ret_id < 0)
		return FAIL;
	ret_id = system("echo dm,acm,adb > /sys/class/android_usb/android0/functions");
	if(ret_id < 0)
		return FAIL;
	ret_id = system("echo 1 > /sys/class/android_usb/android0/enable");
	//        printf("mylog adb off ret:%d\n",ret_id);
	if(ret_id < 0)
		return FAIL;
	return OK;
}

int adb_test_off(void)
{
	int ret_id = 0;
	ret_id = system("echo 0 > /sys/class/android_usb/android0/enable");
	if(ret_id < 0)
		return FAIL;
	ret_id = system("echo 18d1 > /sys/class/android_usb/android0/idVendor");
	if(ret_id < 0)
		return FAIL;
	ret_id = system("echo 2222> /sys/class/android_usb/android0/idProduct");
	if(ret_id < 0)
		return FAIL;
	ret_id = system("echo acm > /sys/class/android_usb/android0/functions");
	if(ret_id < 0)
		return FAIL;
	ret_id = system("echo 1 > /sys/class/android_usb/android0/enable");
	//        printf("mylog adb off ret:%d\n",ret_id);
	if(ret_id < 0)
		return FAIL;
	return OK;
}

/* system test function */
int system_partition_test(void)
{
	int fd;
	int ret;	
	char data[1024];

	/* power on and init wifi */

	fd = open("/proc/mounts", O_RDONLY);
	memset(data,0, sizeof data);
	ret = read(fd, data, sizeof(data));
	close(fd);

	ret = (int)strstr(data, "system");
	if (!ret) {
	   return FAIL;
	}

	ret = OK;
	return ret;
}

/* sdcard test function */
#if 0
int sd_test(void)
{
	int fd;
	int ret = FAIL;
	char rbuf[10];

	system("mount -t vfat /dev/block/mmcblk0p1 /sdcard");
	usleep(500*1000);

	fd = open("/sdcard/test_sdcard.txt", O_RDWR|O_CREAT, 00664);
	if(fd < 0)
		ret = FAIL;
	else {
		write(fd, "OK", 2);
		lseek(fd, 0, SEEK_SET);
		read(fd, rbuf, 10);
		if(strncmp(rbuf, "OK", 2) == 0)
			ret = OK;
	}

	system("rm -f /sdcard/test_sdcard.txt");

	return ret;
	return 0;
}
#endif
int handle_null(void)
{
	int ret;
	
	pr_info("Enter %s\n", __func__);

	// TODO
	// ... ...

	// wait input event
	// ret: OK -> KEY_END, Redo -> KEY_REDO, Fail -> KEY_BACK 

	// OK: 0, Fail: -1
	return FAIL;
}

int handle_sim_test(void)
{
	int ret = -1;
#if 0
	pr_info("Enter %s\n", __func__);

	int modem_fd=-1;
	char rsp[RSPLEN] = {0};

	int i;

	if (open_tty_port(&modem_fd, tty_modem, ATP_TTY_TYPE_MODEM) < 0) {
       	return ret;
        }

	for (i=0; i<3; ++i)
	{
		
		ret = send_at_cmd(modem_fd, "AT+CPIN?\r", rsp, sizeof(rsp));  //AT+CPIN?
		if(ret <= 0)
			break;
		
		sleep(1);
	}

#endif
	return ret;
}

int usb_typec_test()
{
	int fd;
	int ret = FAIL;
	char buf[8];
	int count = 0;
	fd = open("/sys/bus/i2c/devices/11-0060/devid", O_RDONLY);

	if (fd < 0){
		ret = FAIL;
	}
	else{
		count = read(fd, buf, 8);
		if(count > 0 && buf[0] == '0')
			ret = OK;
		else
			ret = FAIL;
	}
	return ret;
}

unsigned char get_sensor_id(const char *path, unsigned char addr, unsigned char reg)
{
	int fd;
	struct i2c_smbus_ioctl_data ctrl_data;
	union i2c_smbus_data sdata;

	fd = open(path, O_RDWR);
	if (fd < 0)
	{
		pr_info("Fail to open %s\n", path);
		return -1;
	}

	if (ioctl(fd, I2C_SLAVE_FORCE, addr) < 0)
	{
		pr_info("Fail to set i2c addr 0x%02x\n", addr);
		return -1;
	}

	ctrl_data.read_write = I2C_SMBUS_READ;
	ctrl_data.command = reg;
	ctrl_data.size = I2C_SMBUS_BYTE_DATA;
	ctrl_data.data = &sdata;

	if (ioctl(fd, I2C_SMBUS, &ctrl_data) < 0)
	{
		pr_info("Fail to read chip id(%s: addr = 0x%02x)\n", path, addr);
		return -1;
	}

	close(fd);
	return ctrl_data.data->byte;
}

int run_sensor_selftest(int sensor_id)
{
    int fd;
    int fd_result;
    char cmd_buf[SENSORHUB_CMD_LEN] = {0};
    char byte1[4] = {0};
    char byte2[4] = {0};
    char result[SENSORHUB_CMD_LEN] = {0};
    int ret = -1;

    fd = open(SENSORHUB_TCMD, O_RDWR);
    if (fd < 0) {
        pr_info("Fail to open %s\n", SENSORHUB_TCMD);
        return -1;
    }

    snprintf(cmd_buf, SENSORHUB_CMD_LEN, "0 %d 2\n", sensor_id);  
    pr_info("To be exec sensor tcmd: %s\n", cmd_buf);

    if (write(fd, cmd_buf, strlen(cmd_buf)+1) < 0) {
        pr_info("Fail to write sensor tcmd\n");
        close(fd);
        return -1;
    }
    usleep(500*1000);

    snprintf(cmd_buf, SENSORHUB_CMD_LEN, "1 %d 2\n", sensor_id);
    pr_info("To be exec sensor tcmd: %s\n", cmd_buf);

    if (write(fd, cmd_buf, strlen(cmd_buf)+1) < 0) {
        pr_info("Fail to write sensor tcmd\n");
        close(fd);
        return -1;
    }
    usleep(50*1000);

    fd_result = open(SENSORHUB_TRES, O_RDONLY);
    if (fd_result < 0) {
        pr_info("Fail to open %s\n", SENSORHUB_TRES);
        return -1;
    }

    if (read(fd_result, cmd_buf, SENSORHUB_CMD_LEN-1) <= 0) {
        pr_info("Fail to read sensor's selftest result\n");
        close(fd_result);
        close(fd);
        return -1;
    }

    cmd_buf[SENSORHUB_CMD_LEN-1] = 0;
    sscanf(cmd_buf, "%s %s %s\n", byte1, byte2, result);  
    pr_info("byte1=%s, byte2=%s, other=%s\n", byte1, byte2, result);

    if (strcmp(byte2, "2") == 0)
        ret = 0;

    close(fd_result);
    close(fd);
    return ret;
}

int psensor_test(void)
{
    return run_sensor_selftest(4);
}

int msensor_test(void)
{
    return run_sensor_selftest(1);
}

int gsensor_test(void)
{
    return run_sensor_selftest(0);
}

int pressure_sensor_test(void)
{
    return run_sensor_selftest(6);
}

int lsensor_test(void)
{
    return run_sensor_selftest(3);
}

int Gyrosensor_test(void)
{
    return run_sensor_selftest(2);
}

#define BAT_PATH 	"/sys/class/power_supply/bq274xx-3/voltage_now"
int battery_test(void)
{
	int fd, ret;
	int vol;
	char voltage[50];
	
	memset(voltage, 0, sizeof(voltage));
	fd = open(BAT_PATH, O_RDONLY);
	if(fd<0)
	{
		pr_info("Fail to open %s\n", BAT_PATH);
		return -1;
	}
	ret = read(fd, voltage, 50);
	vol = atoi(voltage);
	if(vol > 10000)
	{
		vol = vol/1000;			//Battery driver has modified the unit from mV to uV

		strcpy(g_board_ext_info,voltage);
		printf("battery vol : %s\n", g_board_ext_info);
	}
	pr_info("battery voltage = %d\n", vol);
	
	close(fd);

	return 0;
}


#define UFS_SIZE_PATH 	"/sys/block/sda/size"
#define UFS_ID_PATH 	"/sys/block/sda/device/model"
int ufs_size_test(void)
{
	int fd, ret;
	long int data;
	char buff[50];
	
	memset(buff, 0, sizeof(buff));
	fd = open(UFS_SIZE_PATH, O_RDONLY);
	if(fd<0)
	{
		pr_info("Fail to open %s\n", UFS_SIZE_PATH);
		return -1;
	}
	ret = read(fd, buff, 50);
	data = atoi(buff);
	if(data != 0)
	{
		data = (data << 9) >> 30;			//1G

		strcpy(g_board_ext_info,buff);
		printf("ufs_size : %s\n", g_board_ext_info);
	}
	pr_info("ufs_size = %ld G\n", data);
	
	close(fd);

	return 0;
}


int ufs_id_test(void)
{
	int fd, ret;
	int data;
	char buff[50];
	
	memset(buff, 0, sizeof(buff));
	fd = open(UFS_ID_PATH, O_RDONLY);
	if(fd<0)
	{
		pr_info("Fail to open %s\n", UFS_ID_PATH);
		return -1;
	}
	ret = read(fd, buff, 50);
	if(buff[0] != 0)
	{

		strcpy(g_board_ext_info,buff);
		printf("ufs_id : %s\n", g_board_ext_info);
	}
	
	close(fd);

	return 0;
}


int ufs_checkid_test(void)
{
	int fd, ret;
	int data;
	char buff[50];
	char chip[50]="KLUCG8G1BD-E0B1";
	int i = 0;
	
	memset(buff, 0, sizeof(buff));
	fd = open(UFS_ID_PATH, O_RDONLY);
	if(fd<0)
	{
		pr_info("Fail to open %s\n", UFS_ID_PATH);
		return -1;
	}
	ret = read(fd, buff, 50);
	printf("ufs_id : %s\n", buff);
	printf("chip_id : %s\n", chip);
	if(strncmp(buff,chip,sizeof("KLUCG8G1BD"))!=0)
	{
		pr_info("ufs id is not same!\n");
		return -1;
	}
	close(fd);

	return 0;
}

#define CHARGER_PATH   "/sys/bus/i2c/devices/0-006b/revision"
int charger_ic_test(void)
{
       int fd, ret;
       int exist = 0;
       char exist_str[20];
      
       memset(exist_str, 0, sizeof(exist_str)); 
       fd = open(CHARGER_PATH, O_RDONLY);
       if(fd < 0)
       {
               pr_info("Fail to open %s\n", CHARGER_PATH);
               return -1;
       }
       ret = read(fd, exist_str, 20);
       exist = atoi(exist_str);
       if(exist != 1)
       {
               pr_info("Detect Charger-IC Failed! \n");
               return -1;
       }
       pr_info("charger-IC exist = %d\n", exist);
       close(fd);

       return 0;
}

int32_t write_to_interface(int32_t fd, char* buffer, int32_t buf_len)
{
    int32_t write_len = 0;
    int32_t len = 0;

    do {
        if ((len = write(fd, buffer, buf_len)) < 0) {
			pr_info("%s : Fail to write\n", __func__);
            if (errno == EINTR || errno == EAGAIN)
                continue;
            return -1;
        }
        buf_len -= len;
        buffer += len;
        write_len += len;
    } while (write_len < buf_len);
    return write_len;
}

#if 0
int AT_Command_SNRead(void){
	/**************************
	only test code now
	AT_STRING must replace the right at command.
	************************/
	#define AT_STRING       "AT\r\n"
	int ret = -1;
	int len = -1;
	int tryAgain = 0;
	int socket_fd = socket_local_client(SOCKET_VCD, ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
	if(socket_fd < 0){
		return socket_fd;
	}

	pr_info("*#%s#*SOCKET string(%s)\n", __FUNCTION__, SOCKET_VCD);

	do{
        tryAgain = 0;
		len = write_to_interface(socket_fd, AT_STRING, sizeof(AT_STRING));

		if(len > 0){
			char buf[MAX_BUF_SIZE+1] = {0, };
			int nread = 0;
			nread = read(socket_fd, buf, MAX_BUF_SIZE);
			if(nread > 0){
				if(strstr (buf, "OK")){
					pr_info("*#%s#*format finished  OK\n", __FUNCTION__);
					ret = 0;
				}else if(strstr (buf, "ONLINE")){
					pr_info("*#%s#*try again send the AT Command\n", __FUNCTION__);
					tryAgain = 1;
				}else{
					pr_info("*#%s#*format finished FAIL\n", __FUNCTION__);
				}
			}
		}
	}while(tryAgain > 0);
	
    close(socket_fd);

    return ret;
}
#endif

int AT_Command_SNWrite(void){
	return 0;
}

int AT_Command_GSNRead(void){
	return 0;
}

int AT_Command_GSNWrite(void){
	return 0;
}

int AT_Command_SIM1Test(void){
	return 0;
}

int AT_Command_SIM2Test(void){
	return 0;
}


#define GAS_GAUGE_PATH   "/sys/bus/i2c/devices/6-0055/device_type"
#define fuel_gauge_val 1062
int gas_gauge_test(void)
{
	int fd, ret;
	int data;
	char buff[50];
	
	memset(buff, 0, sizeof(buff));
	fd = open(GAS_GAUGE_PATH, O_RDONLY);
	if(fd<0)
	{
		pr_info("Fail to open %s\n", GAS_GAUGE_PATH);
		return -1;
	}
	ret = read(fd, buff, 50);
	data = atoi(buff);
	if(data != 1062)
		return -1;
	close(fd);

	return 0;
}

#if 0
static struct pcm_config config = {
    .channels = 2,
    .rate = 48000,
    .period_size = 128,
    .period_count = 2,
    .format = PCM_FORMAT_S16_LE,
};

static struct pcm *pcm_sco_out = NULL;
static struct pcm *pcm_sco_in = NULL;
static struct pcm *pcm_spa_out = NULL;
struct pcm *pcm_spa_in = NULL;

int audio_stop_playback_spk(void)
{
    pcm_stop(pcm_spa_in);
    pcm_close(pcm_spa_in);
    pcm_stop(pcm_spa_out);
    pcm_close(pcm_spa_out);
    pcm_stop(pcm_sco_in);
    pcm_close(pcm_sco_in);
    pcm_stop(pcm_sco_out);
    pcm_close(pcm_sco_out);
    return 0;
}

int audio_playback_spk(void)
{
    int ret = -1;

    // configurate audio path.
    ret = system("tinymix \'AIF3TX1 Input 1\' AIF1RX1");
    if (ret < 0)
	    return -1;
    ret = system("tinymix \'AIF3TX1 Input 2\' AIF1RX2");
    if (ret < 0)
	    return -1;

    ret = system("tinymix \'AIF3TX2 Input 1\' AIF1RX1");
    if (ret < 0)
	    return -1;

    ret = system("tinymix \'AIF3TX2 Input 2\' AIF1RX2");
    if (ret < 0)
	    return -1;

    // start spa related sound device.
    pcm_sco_out = pcm_open(0, 3, PCM_OUT | PCM_MONOTONIC, &config);
    if (pcm_sco_out && !pcm_is_ready(pcm_sco_out)) {
        printf("pcm_open(SCO_OUT) failed: %s",
                pcm_get_error(pcm_sco_out));
        goto err_sco_out;
    }
    pcm_start(pcm_sco_out);
    
    pcm_sco_in = pcm_open(0, 3, PCM_IN | PCM_MONOTONIC, &config);
    if (pcm_sco_in && !pcm_is_ready(pcm_sco_in)) {
        printf("pcm_open(SCO_IN) failed: %s",
                pcm_get_error(pcm_sco_in));
        goto err_sco_in;
    }
    pcm_start(pcm_sco_in);
    
    pcm_spa_out = pcm_open(0, 4, PCM_OUT | PCM_MONOTONIC, &config);
    if (pcm_spa_out && !pcm_is_ready(pcm_spa_out)) {
        printf("pcm_open(SPA_OUT) failed: %s",
                pcm_get_error(pcm_spa_out));
        goto err_spa_out;
    }
    pcm_start(pcm_spa_out);
    
    pcm_spa_in = pcm_open(0, 4, PCM_IN | PCM_MONOTONIC, &config);
    if (pcm_spa_in && !pcm_is_ready(pcm_spa_in)) {
        printf("pcm_open(SPA_IN) failed: %s",
                pcm_get_error(pcm_spa_in));
        goto err_spa_in;
    }
    pcm_start(pcm_spa_in);

    // use tinyplay to playback
    ret = system("tinyplay /system/media/audio/alarms/spk.wav&");
    if (ret < 0)
        return -1;
    else
        return 0;

err_spa_in:
    pcm_stop(pcm_spa_in);
    pcm_close(pcm_spa_in);
err_spa_out:
    pcm_stop(pcm_spa_out);
    pcm_close(pcm_spa_out);
err_sco_in:
    pcm_stop(pcm_sco_in);
    pcm_close(pcm_sco_in);
err_sco_out:
    pcm_stop(pcm_sco_out);
    pcm_close(pcm_sco_out);

    return -1;
}
#endif

int handle_rtc_test(void)
{
       char data[1024];
       int fd, n;
       char *ret = NULL;
       
       fd = open("/proc/driver/rtc", O_RDONLY);
       if(fd < 0)
       {
               pr_info("Open rtc proc file failed! fd=0x%x\n", fd);
               return -1;
       }
       n = read(fd, data, 1023);
       close(fd);
       if(n < 0)
       {
               pr_info("read rtc proc file failed! read_return %d", n);
               return -1;
       }
       data[n] = 0;
       pr_info("read rtc-value = %s \n", data);
       ret = strstr(data, "rtc_time");
       if(!ret)
       {
               pr_info("rtc: search string failed! \n");
               return -1;
       }

       return 0;
}

int frontcamera_test(void)
{
	int ret = -1;
	int fd;
	int  index=1;
	const char *fimc0_device   = "/dev/video41";
	unsigned short readvar = 0;
        struct v4l2_input input;
	fd = open (fimc0_device, O_RDWR);				
	if (fd < 0)
	{
		pr_info("fimc0 open fail!!!\n");
		return FAIL;
	} 
	input.index = index;
        if (ioctl(fd, VIDIOC_S_INPUT, &input) < 0) {
                printf("fimc set input fail!!!");
                close(fd);//we should close fd whatever
                return FAIL;
        }

	close(fd);
	
	return OK;
}

int rearcamera_test(void)
{
	int ret = -1;
	int fd;
	int  index=1;
	const char *fimc0_device   = "/dev/video40";
	unsigned short readvar = 0;
        struct v4l2_input input;
	fd = open (fimc0_device, O_RDWR);				
	if (fd < 0)
	{
		pr_info("fimc0 open fail!!!\n");
		return FAIL;
	} 
	input.index = index;
        if (ioctl(fd, VIDIOC_S_INPUT, &input) < 0) {
                printf("fimc set input fail!!!");
                close(fd);//we should close fd whatever
                return FAIL;
        }

	close(fd);
	
	return OK;
}

#if 0
/* bluetooth test function  */
static int bt_test(void)
{
	int fd;
	int ret;
	char data[1024];

	/* bdt tool source code: 
	* external\bluetooth\bluedroid\test\bluedroidtest\bluedroidtest.c
	* "bdt -t t" command enables and disables bluetooth.
	*/
	system("bdt -t t > /data/bt.log");

	fd = open("/data/bt.log", O_RDONLY);
	ret = read(fd, data, sizeof(data));
	close(fd);

	ret = (int)strstr(data, "ADAPTER STATE UPDATED : ON");
	if (!ret) {
		ret = FAIL;
		goto out;
	}
	ret = OK;
out:
	system("rm -f /data/bt.log");
	return ret;
}

static int bt_open(void)
{
    system("bdt -o o");
    return OK;
}

static int bt_close(void)
{
    bdt_disable();
    HAL_unload();
    return OK;
}

static int bt_test_id(void)
{
	int fd;
	int ret;
	char data[1024];

	/* bdt tool source code: 
	* external\bluetooth\bluedroid\test\bluedroidtest\bluedroidtest.c
	* "bdt -t t" command enables and disables bluetooth.
	*/
	system("logcat -c");
	system("bdt -t t");
	system("logcat -d -s bt_hwcfg > /data/bt.log");

	fd = open("/data/bt.log", O_RDONLY);
	ret = read(fd, data, sizeof(data));
	close(fd);

	ret = (int)strstr(data, "BCM4345C0");
	if (!ret) {
		ret = FAIL;
		goto out;
	}
	ret = OK;
out:
	system("rm -f /data/bt.log");
	return ret;
}

static int bt_read_addr(void)
{
	do_readmac(NULL);
	pr_info("\n");
	return OK;
}

static int bt_write_addr(void)
{
	do_writemac("22:22:ac:02:ba:63");
	return OK;
}
#endif

static int wifi_test(void)
{
	int fd;
	int ret;
	char data[1024];

	/* power on and init wifi */
	system("netcfg wlan0 up");
	system("netcfg > /data/wifi.log");

	fd = open("/data/wifi.log", O_RDONLY);
	memset(data,0, sizeof data);
	ret = read(fd, data, sizeof(data));
	close(fd);

	ret = (int)strstr(data, "wlan0    UP");
	if (!ret) {
	   ret = FAIL;
	   goto out;
	}

	ret = OK;
out:
	/* clean up and power off wifi */	
	system("rm -f /data/wifi.log");
	system("netcfg wlan0 down");
    return ret;
}

static int wifi_open(void)
{
    int fd;
    int ret;
    char data[1024];

    /* power on and init wifi */
    system("netcfg wlan0 up");
    system("netcfg > /data/wifi.log");

    fd = open("/data/wifi.log", O_RDONLY);
    memset(data,0, sizeof data);
    ret = read(fd, data, sizeof(data));
    close(fd);

    ret = (int)strstr(data, "wlan0    UP");
    if (!ret) {
       ret = FAIL;
       goto out;
    }

    ret = OK;
out:
    /* clean up */
    system("rm -f /data/wifi.log");
    return ret;
}

static int wifi_close(void)
{
    system("netcfg wlan0 down");
    return OK;
}

static int wifi_test_id(void)
{
    return wifi_test();
}

static int wifi_read_addr(void)
{
	FILE * f;
	int a;

	pr_info("wifi mac address = ");
	f = fopen("/data/wifimac","r");
	while((a = fgetc(f))!= EOF)
		pr_info("%c",a);
	fclose(f);
	pr_info("\n");
	return OK;
}

static int wifi_write_addr(void)
{
    system("echo 38:BC:1A:02:BA:63 > /data/wifimac");
    return OK;
}

#define AUDIO_PATH   "/sys/module/arizona_core/parameters/codec_id"
#define ARIZONA_CHIPID 0x6360
static int audio_test(void)
{
#if 0
	int times;
	int fd;
	int ret;
	unsigned char data[1024];

	/* set the spk mixer. some mixers just use dfl value */
	ret = system("tinymix 158 1");
	if (ret < 0)
		return FAIL;

	ret = system("tinymix 163 1");
	if (ret < 0)
		return FAIL;

	ret = system("tinymix 148 1");
	if (ret < 0)
		return FAIL;

	ret = system("tinymix 143 1");
	if (ret < 0)
		return FAIL;

	ret = system("tinymix 123 1");
	if (ret < 0)
		return FAIL;

	ret = system("tinymix 67 1");
	if (ret < 0)
		return FAIL;

	ret = system("tinymix 64 3");
	if (ret < 0)
		return FAIL;

	ret = system("tinymix 68 4");
	if (ret < 0)
		return FAIL;

	/* play the test music file for about 3 second */
	ret = system("tinyplay /system/media/audio/alarms/spk.wav &");
	if (ret < 0)
		return FAIL;

	return OK;
#endif

	int fd, ret;
	int data;
	char buff[50];
	
	memset(buff, 0, sizeof(buff));
	fd = open(AUDIO_PATH, O_RDONLY);
	if(fd<0)
	{
		pr_info("Fail to open %s\n", AUDIO_PATH);
		return -1;
	}
	ret = read(fd, buff, 50);
	data = atoi(buff);
	printf("audio chipid: %d\n",data);
	close(fd);
	if(data != ARIZONA_CHIPID)
		return -1;

	return 0;
}
int format_cache(void)
{
	int fd;
	int ret;
	char data[1024];


	/* power on and init wifi */

	fd = open("/proc/mounts", O_RDONLY);
	memset(data,0, sizeof data);
	ret = read(fd, data, sizeof(data));
	close(fd);
	ret = (int)strstr(data, "ufs");
	if (ret) {
		ret = system("make_ext4fs /dev/block/platform/15570000.ufs/by-name/cache");
	}
	else {
		ret = system("make_ext4fs /dev/block/platform/15740000.dwmmc0/by-name/cache");
	}
	usleep(500*1000);
	if (ret < 0)
		return FAIL;
		
	return OK;
}

int format_userdata(void)
{
	int fd;
	int ret;
	char data[1024];


	/* power on and init wifi */

	fd = open("/proc/mounts", O_RDONLY);
	memset(data,0, sizeof data);
	ret = read(fd, data, sizeof(data));
	close(fd);
	ret = (int)strstr(data, "ufs");
	if (ret) {
		ret = system("make_ext4fs /dev/block/platform/15570000.ufs/by-name/userdata");
	}
	else {
		ret = system("make_ext4fs /dev/block/platform/15740000.dwmmc0/by-name/userdata");
	}
	usleep(500*1000);
	if (ret < 0)
		return FAIL;
		
	return OK;
}


int factory_test(void)
{
	int fd;
	int ret;
	char data[1024];


	/* power on and init wifi */

	fd = open("/proc/mounts", O_RDONLY);
	memset(data,0, sizeof data);
	ret = read(fd, data, sizeof(data));
	close(fd);
	ret = (int)strstr(data, "ufs");
	if (ret) {
		ret = system("make_ext4fs /dev/block/platform/15570000.ufs/by-name/userdata");
		usleep(200*1000);
		if(ret < 0)
			return FAIL;
		ret = system("make_ext4fs /dev/block/platform/15570000.ufs/by-name/cache");
	}
	else {
		ret = system("make_ext4fs /dev/block/platform/15740000.dwmmc0/by-name/userdata");
		usleep(200*1000);
		if(ret < 0)
			return FAIL;
		ret = system("make_ext4fs /dev/block/platform/15740000.dwmmc0/by-name/cache");
	}
	usleep(500*1000);
	if (ret < 0)
		return FAIL;
		
	return OK;
}

#define SMART_PA_PATH "/sys/kernel/debug/tfa9891-34/regs/03-REVISIONNUMBER"
#define SPART_PA_CHIPID0 0x80
#define SPART_PA_CHIPID1 0x81
#define SPART_PA_CHIPID2 0x92
int smart_pa_test(void)
{
	int fd, ret;
	int data;
	char buff[50];
	
	memset(buff, 0, sizeof(buff));
	fd = open(SMART_PA_PATH, O_RDONLY);
	if(fd<0)
	{
		pr_info("Fail to open %s\n", SMART_PA_PATH);
		return -1;
	}
	ret = read(fd, buff, 50);
	sscanf(buff,"%x",&data);
	printf("smart_pa chipid: %d\n",data);
	close(fd);
	if((data == SPART_PA_CHIPID0)||(data == SPART_PA_CHIPID1)||(data == SPART_PA_CHIPID2))
		return 0;

	return -1;
}

int phone_off_test(void)
{
	int ret = -1;
	ret = system("svc power shutdown");
	if(ret < 0)
		return -1;
	return 0;
}
int secureboot_test (void)
{
	int val = 0,fd;
	int ret = -1;
	char data[50];
	char secure[50]= "secureboot";
	char nonsecure[50]= "nonsecureboot";

	memset(data, 0, sizeof(data));
	ret = system("pd gr 0x1000001c > /data/info");
	if(ret < 0)
		return -1;

	fd = open("/data/info", O_RDONLY);
	memset(data,0, sizeof data);
	ret = read(fd, data, sizeof(data));
	sscanf(data,"%x",&val);
	printf("secureboot:%#x\n",val);
	close(fd);
	system("rm /data/info");
	if(val == 0)
	{
		strcpy(g_board_ext_info,nonsecure);

	}
	else
	{
		strcpy(g_board_ext_info,secure);
		
	}
	return 0;

	
}

int base_power_set(void)
{
	int ret = -1;
	ret = 0; // bt_close();
	if (ret < 0)
	{
		printf("close bt failed\n");
		return -1;
	}
	ret = wifi_close();
	if (ret < 0)
	{
		printf("close wifi failed\n");
		return -1;
	}
	return 0;

}

#define CHARGER_ENABLE   "/sys/bus/i2c/devices/0-006b/enable"
int close_charger(void)
{
	int fd;
	int ret;
	fd = open(CHARGER_ENABLE, O_RDWR);
	if(fd < 0)
	{
		ret = FAIL;
	}
	else
	{
		write(fd, "1", 1);
		ret = OK;
	}
	close(fd);
	return ret;
}

/**
 * shaoguodong:
 * if the test code is not ready yet,
 * make sure keep the 'test' feild as NULL
 * the test framework will know that this sub test
 * is NA yet
 */
#if 0
static struct test_list works[] =
{
	{"SYSTEM MOUNT:", system_partition_test},
//	{"EXTERN SD :",	extern_sd_test},
	{"SIM:", NULL},
	{"SN:", NULL},
	{"Battery:", battery_test},
	{"Audio:", audio_test},
	{"Bluetooth:", bt_test},
	{"WIFI:", wifi_test},
	{"GPS:", gps_test},
	{"USB TypeC:", usb_typec_test},
	{"GSensor:", gsensor_test},
	{"MSensor:", msensor_test},
	{"PSensor:", psensor_test},
	{"LSensor:", lsensor_test},
	{"RTC:", handle_rtc_test},
	{"FCamera:", frontcamera_test},
	{"RCamera:", rearcamera_test},
	{"Gyro:", Gyrosensor_test},
	{"Charger:", charger_ic_test},
	{"ATCommand:", AT_Command_SNRead},
};
#endif

struct test_list works[] =
{
	{"SYSTEM MOUNT:", system_partition_test, "TAT01", NULL},
	{"Battery:", battery_test, "TAT04", NULL},
//	{"SIM:", NULL,"TAT02", NULL},
//	{"SN:", NULL, "TAT03", NULL},
	{"Audio:", audio_test, "TAT05", NULL},
//	{"Bluetooth:", bt_test, "TAT07", NULL},
	{"WIFI:", wifi_test, "TAT06", NULL},
//	{"Bluetooth ID:", bt_test_id, "TAT97", NULL},
	{"WIFI ID:", wifi_test_id, "TAT96", NULL},
	{"GSensor:", gsensor_test, "TAT09", NULL},
	{"MSensor:", msensor_test, "TAT10", NULL},
	{"Gyro:", Gyrosensor_test, "TAT08", NULL},
	{"Prox Sensor:", psensor_test, "TAT10", NULL},
	{"ALS Sensor:", lsensor_test, "RPSOR1", NULL},
	{"Pressure Sensor:", pressure_sensor_test, "TAT14", NULL},
	{"RearRGB sensor:", NULL, "TAT15", NULL},
	{"RTC:", handle_rtc_test, "TAT12", NULL},
	{"FCamera:", frontcamera_test, "TAT13", NULL},
	{"RCamera:", rearcamera_test, "TAT14", NULL},
	{"Charger:", charger_ic_test, "TAT16", NULL},
	{"Fuel Gauge:", gas_gauge_test, "TFUG", NULL},
	{"Audio Smart PA:", smart_pa_test, "TSMARTPA", NULL},
	{"Secboot:", secureboot_test, "SECBOOT", NULL},
	{"EMMC Size:", ufs_size_test, "EMMCSIZE", NULL},
	{"EMMC ID:", ufs_id_test, "EMMCID", NULL},
	{"Check EMMC ID:", ufs_checkid_test, "CHKEMMCID", NULL},
	{"Factory Reset:", factory_test, "TFACRESET", NULL},
	{"Write HWID:", NULL, "WHWID", NULL},
	{"Read HWID:", NULL, "RHWID", NULL},
	{"NFC:", NULL, "TNFC", NULL},
	//=-----current test
	{"BASE POWER:", base_power_set, "TAT40", NULL},
//	{"Open BT:", bt_open, "TAT41", NULL},
//	{"CLOSE BT:", bt_close, "TAT42", NULL},
	{"OPEN WIFI:", wifi_open, "TAT45", NULL},
	{"CLOSE WIFI:", wifi_close, "TAT46", NULL},
//	{"OPEN SPK:", audio_playback_spk, "TAT47", NULL},
//	{"CLOSE SPK:", audio_stop_playback_spk, "TAT48", NULL},
        {"OPEN GPS:", gps_test_open, "TAT49", NULL},
        {"CLOSE GPS:", gps_test_close, "TAT50", NULL},

      //modem
//	{"SN Read:", AT_Command_SNRead, "TAT03", NULL},
	{"SN Write:", AT_Command_SNWrite, "WSN", NULL},
	{"Read group SN:", AT_Command_GSNRead, "RGSN", NULL},
	{"Write Group SN:", AT_Command_GSNWrite, "WGSN", NULL},
	{"SIM1 TEST:", AT_Command_SIM1Test, "TAT02", NULL},
	{"SIM2 TEST:", AT_Command_SIM2Test, "TAT20", NULL},
	//--------------wifi/bt addr
	{"Read Wifi Addr:", wifi_read_addr, "RWIFIADDR", NULL},
	{"Write Wifi Addr:", wifi_write_addr, "WWIFIADDR", NULL},
//	{"Read BT Addr:", bt_read_addr, "RBTADDR", NULL},
//	{"Write BT Addr:", bt_write_addr, "WBTADDR", NULL},
	//test flow
	{"Set Flag:", NULL, "SFG", NULL},
	{"Get Flag:", NULL, "GFG", NULL},
	//----other
	{"Phone Off:", phone_off_test, "SHDN", NULL},
	{"Close Charge:", close_charger, "DCHA", NULL},
	{"ADB On:", adb_test_on, "ADBON", NULL},
	{"ADB Off:", adb_test_off, "ADBOFF", NULL},
	{"Version(in):", NULL, "TSWV", NULL},
	{"Version(out):", NULL, "INTERNSWV", NULL},
};

static struct test_list works_format[] =
{
	{"CACHE:", format_cache },
	{"USERDATA", format_userdata},
};
static void proc_event()
{
	struct event event;
	while (1) {
		dequeue_event_locked(&event);
		if (event.type == KEY_POWER ||
				event.type == HOST_EV_ENTER)
			return;
	}
}


int parse_pc_cmd(void)
{

    char *p1 = NULL;
    char *p2 = NULL;
    char *p3 = NULL;
    char *p4 = NULL;
    char *p5 = NULL;
    char *p6 = NULL;
    char* p7 = NULL;
    char* p8 = NULL;
    char* p9 = NULL;
    char sn_temp;

    char *cmd, *param1, *param2, *param3;
    int i, j;
    int cmdSize;

    // check flag
    p1 = g_uart_recv_buf;
    printf("%s:%s\n",__func__, g_uart_recv_buf);
    p2 = strstr(g_uart_recv_buf, "AABBCCDD");
    if(NULL == p2)
    {
        return -1;
    }
    p1 += 8;
    p2 = strstr(p1, "AABBCCEF");
    p3 = strstr(p1, "WSN");
    p4 = strstr(p1, "SFG");
    p5 = strstr(p1, "TBTANT");
    p6 = strstr(p1, "WIMEI");
    p7 = strstr(p1, "WSN2");
    p8 = strstr(p1, "WGSN");
    p9 = strstr(p1, "GFG");
    if(NULL == p2)
    {
        return -1;
    }

    // check cmd
    cmd = p1;
    pr_info(" cmd =%s, p3=%s\n", cmd, p3);
    p2 = strchr(cmd, '(');
    if(NULL == p2)
    {
        pr_info("Fail to find flag (\n");
        return -1;
    }
    strncpy(g_cmd, cmd, p2-cmd);
    if(p6) {
        p3 = p6 + 6;//+ WIMEI(
        memset(g_param_imei, 0, sizeof(g_param_imei));
        pr_info(" imei=%s\n", p3);
        p2 = strchr(p3, ')');
        if(NULL == p2)
        {
            pr_info("Fail to find flag )\n");
            return -1;
        }
        cmdSize = p2-p3;
        pr_info("cmd size is %d \n", cmdSize);

        if(cmdSize <= 18)
        {
            strncpy(g_param_imei,p3,cmdSize);
        }
        g_param_imei[cmdSize]='\0';
        pr_info(" IMEI=%s\n",g_param_imei);
        *p2 = '\0';
        return 0;
    }
    if(p7) {
        p7 = p7 + 5;//+ WSN2(
        memset(g_param_modem2_sn, 0, sizeof(g_param_modem2_sn));
        //sn_temp = *p7;
        pr_info(" p7=%s and strlen(p7) is %d\n", p7,strlen(p7));

        p2 = strchr(p7, ')');
        if(NULL == p2)
        {
            pr_info("Fail to find flag )\n");
            return -1;
        }
        cmdSize = p2-p7;
        pr_info("cmd size is %d \n", cmdSize);

        if(cmdSize <= 32)
        {
            strncpy(g_param_modem2_sn,p7,cmdSize);
        }
        g_param_modem2_sn[cmdSize]='\0';
        pr_info(" SN2=%s\n",g_param_modem2_sn);
        *p2 = '\0';
        return 0;
    }
    if(p8) {
        p8 = p8 + 5;//+ WGSN(
        memset(g_param_group_sn, 0, sizeof(g_param_group_sn));
        pr_info(" p8=%s and strlen(p8) is %d\n", p8,strlen(p8));

        p2 = strchr(p8, ')');
        if(NULL == p2)
        {
            pr_info("Fail to find flag )\n");
            return -1;
        }
        cmdSize = p2-p8;
        pr_info("cmd size is %d \n", cmdSize);

        if(cmdSize <= 32)
        {
            strncpy(g_param_group_sn,p8,cmdSize);
        }
        g_param_group_sn[cmdSize]='\0';
        pr_info(" GroupSN=%s\n",g_param_group_sn);
        *p2 = '\0';
        return 0;
    }
    if (p5) {
        p5 += 9;// + TBTANTRX( | + TBTANTTX(
        param1 = strchr(p5, ')');
        *param1 = '\0';
        memset(g_param_btant, 0, sizeof(g_param_btant));
        strcpy(g_param_btant, p5);
        pr_info("BT ANT test param = [%s]\n", g_param_btant);
        return 0;
    }
    if(p3)
    {
        p3 = p3 + 4;//+ WSN(
        memset(g_param_sn, 0, sizeof(g_param_sn));
        sn_temp = *p3;
        pr_info(" sn=%x\n", sn_temp);

        p2 = strchr(p3, ')');
        if(NULL == p2)
        {
            pr_info("Fail to find flag )\n");
            return -1;
        }
        cmdSize = p2-p3;
        pr_info("cmd size is %d \n", cmdSize);

        if(cmdSize <= 32)
        {
            strncpy(g_param_sn,p3,cmdSize);
        }
        g_param_sn[cmdSize]='\0';
        pr_info("SN=%s\n",g_param_sn);
        *p2 = '\0';
        return 0;
    }
    if(p4) {
        p4 = p4 + 4;//+ SFG(
        memset(g_param_fg, 0, sizeof(g_param_fg));
        pr_info(" p4=%s and strlen(p4) is %d\n", p4,strlen(p4));
        p2 = strchr(p4, ')');
        if(NULL == p2)
        {
            pr_info("Fail to find flag )\n");
            return -1;
        }
        cmdSize = p2-p4;
        pr_info("cmd size is %d \n", cmdSize);

        if(cmdSize <= 6)
        {
            strncpy(g_param_fg,p4,cmdSize);
        }
        g_param_fg[cmdSize]='\0';
        pr_info(" fg is =%s\n",g_param_fg);
        *p2 = '\0';
        return 0;
    }
    if(p9) {
        p9 = p9 + 4;//+ GFG(
        memset(g_param_fg, 0, sizeof(g_param_fg));
        pr_info(" p9=%s and strlen(p9) is %d\n", p9,strlen(p9));
        p2 = strchr(p9, ')');
        if(NULL == p2)
        {
            pr_info("Fail to find flag )\n");
            return -1;
        }
        cmdSize = p2-p9;
        pr_info("cmd size is %d \n", cmdSize);

        if(cmdSize <= 6)
        {
            strncpy(g_param_fg,p9,cmdSize);
        }
        g_param_fg[cmdSize]='\0';
        pr_info("Get fg is =%s\n",g_param_fg);
        *p2 = '\0';
        return 0;
    }

   // parse_pc_cmd_wireless(g_uart_recv_buf);

    // check param1
    param1 = p2 + 1;
    p2 = strchr(param1, ',');
    if(NULL == p2)
    {
        p2 = strchr(param1, ')');
        memset(g_uart_recv_syscall,0,sizeof(g_uart_recv_syscall));
        strcpy(g_uart_recv_syscall,param1);
        *p2 = '\0';
        g_param1 = atoi(param1);
        pr_info("param1=%s, g_param1=%x, g_uart_recv_syscall= %s\n",param1, g_param1, g_uart_recv_syscall);
        return 0;
    }

    *p2 = '\0';
    g_param1 = atoi(param1);

    // check param2
    param2 = p2 + 1;
    p2 = strchr(param2, ',');
    if(NULL == p2)
    {
        p2 = strchr(param2, ')');
        *p2 = '\0';
        g_param2 = atoi(param2);
        return 0;
    }

    *p2 = '\0';
    g_param2 = atoi(param2);

    // check param3
    param3 = p2 + 1;
    p2 = strchr(param3, ')');
    if(NULL == p2)
    {
        return -1;
    }
    *p2 = '\0';
    g_param3 = atoi(param3);

    return 0;
}
#if 1
void board_test_win_work(void)
{
	int i;
	int row = 2;
	char to_host[1000];
	char* p;
	int n;
	char* head = "engmode: ";//"*#rst#*";

	p = to_host;
	strcpy(p, head);
	n = strlen(head);
	p+=n;

	draw_win(&board_test_win);
	for (i = 0; i < sizeof(works)/sizeof(struct test_list); i++) {
		ui_puts(works[i].name, row, 1, font_color);
		strcpy(p, works[i].name);
		n = strlen(works[i].name);
		p[n-1] = ' ';
		p += n;

		if (works[i].test != NULL) {
			if (works[i].test() == OK) {
				ui_puts_right("OK", row, ok_color);
				strcpy(p, "OK");
			}
			else {
				ui_puts_right("FAIL", row, fail_color);
				strcpy(p, "FAIL");
			}
		} else {
			ui_puts_right("N/A", row, na_color);
			strcpy(p, "N/A");
		}
		row++;

		print("%s\n", p-n);
	}
	print("*#state#*test finished\n");
	proc_event();
	return;
}
#endif

void Erase_cache_data_partition_win(void)
{
	int i;
	int row = 2;
	char to_host[1000];
	char* p;
	int n;
	char* head = "Format: ";//"*#rst#*";

	p = to_host;
	strcpy(p, head);
	n = strlen(head);
	p+=n;

	draw_win(&format_win);
	for (i = 0; i < sizeof(works_format)/sizeof(struct test_list); i++) {
		ui_puts(works_format[i].name, row, 1, font_color);
		strcpy(p, works_format[i].name);
		n = strlen(works_format[i].name);
		p[n-1] = ' ';
		p += n;

		if (works_format[i].test != NULL) {
			if (works_format[i].test() == OK) {
				ui_puts_right("OK", row, ok_color);
				strcpy(p, "OK");
			}
			else {
				ui_puts_right("FAIL", row, fail_color);
				strcpy(p, "FAIL");
			}
		} else {
			ui_puts_right("N/A", row, na_color);
			strcpy(p, "N/A");
		}
		row++;

		print("%s\n", p-n);
	}
	print("*#state#*format finished\n");
	proc_event();
	return;
}
void wait_uart_order(void)
{
	unsigned int i;
	int wlen, ret;
	char wbuf[50];
	int key;
	int len;
	int uart_port = UART_PORT_FIQ;
	char uart_wbuf[1024];
	char uart_len;
	int try_times = 0;
	int get_order = 0;

	ret = uart_recv();
	//pr_info("uart_recv: %d\n", ret);
	if(ret>=0){                                     // handle physical uart cmd
		uart_port = UART_PORT_FIQ;
		get_order = 1;
	}else{                                                  // handle virtual uart cmd
		ret = virt_uart_recv();
		if(ret>=0){
			uart_port = UART_PORT_GS0;
			get_order = 1;
		}
	}
       if(get_order == 1)
       {
	       {
		       if(row==0 && col ==0)
			       fill_screen(back_color);

	       }		

	       ret = parse_pc_cmd();
	       if(ret == 0)
	       {
		       pr_info("cmd = %s, param1 = %d, param2 = %d, param3 = %d\n", \
				       g_cmd, g_param1, g_param2, g_param3);

		       g_uart_test_mode = 1;
		       for(i=0; i<sizeof(works)/sizeof(struct test_list); i++)
		       {
			       printf("%s:%s\n",g_cmd,works[i].cmd);
			       if(strcmp(g_cmd, works[i].cmd) == 0)
			       {
				       //if(row==0)
				       //	fill_screen(back_color);
				       ui_puts(works[i].name, row, col, font_color);
				       ret = works[i].test();
				       if(ret == OK)
				       {
					       ui_puts("OK", row, col + 16, GREEN_COLOR);

				       }
				       else
				       {
					       ui_puts("FAL", row, col + 16, RED_COLOR);

				       }
				       row++;


				       if(g_board_ext_info[0] != 0){
					       wlen = sprintf(wbuf, "RSP(%s,%s)", (ret==0)?"1":"0", g_board_ext_info);
					       memset(g_board_ext_info, 0 , 50);
				       } else {
					       if (ret != 0)
					       {
						       wlen = sprintf(wbuf, "RSP(%s,%s Failed)", (ret==0)?"1":"0", works[i].name);
					       }
					       else
					       {
						       wlen = sprintf(wbuf, "RSP(%s,%s)", (ret==0)?"1":"0", works[i].name);
					       }
				       }
				       test_num++;
				       if(uart_port == UART_PORT_FIQ)
					       uart_send(wbuf, wlen);
				       else
					       virt_uart_send(wbuf, wlen);
				       break;
			       }
		       }
		       if (i == sizeof(works)/sizeof(struct test_list))
		       {

			       wlen = sprintf(wbuf, "RSP(%s,%s)", "0", "Invalid cmd");
			       if(uart_port == UART_PORT_FIQ)
				       uart_send(wbuf, wlen);
			       else
				       virt_uart_send(wbuf, wlen);
		       }
		       memset(g_cmd, 0, 32);
		       g_param1 = 0;
		       g_param2 = 0;
		       g_param3 = 0;
		       g_uart_test_mode = 0;
		       if(row == max_row && col==1 )
		       {
			       row = 1;
			       col = (max_col-1)/2;
		       }
	       }//continue;

       }
	//   virt_uart_uninit();
	//   close_input_device();

	// return 0;

}

void board_test_by_console(void)
{
	unsigned int i;
	int wlen, ret;
	char wbuf[50];
	int key;
	int len;
	int row = 0, col = 0;
	int uart_port = UART_PORT_FIQ;
	char uart_wbuf[1024];
	char uart_len;
	int try_times = 0;

	fill_screen(back_color);
	if(g_uart_fd || g_virt_uart_fd)
	{
		ui_puts("console ready,please send the order:", row, col, BLUE_COLOR);
		row++;
		col++;
	}
	while(1)
	{
		sleep(1);
		ret = uart_recv();
		//pr_info("uart_recv: %d\n", ret);
		if(ret>=0){                                     // handle physical uart cmd
			uart_port = UART_PORT_FIQ;
		}else{                                                  // handle virtual uart cmd
				ret = virt_uart_recv();
			if(ret>=0){
				uart_port = UART_PORT_GS0;
			}
		} 
		ret = parse_pc_cmd();
		if(ret == 0)
			//	continue;	
			//		{
		{
			pr_info("cmd = %s, param1 = %d, param2 = %d, param3 = %d\n", \
					g_cmd, g_param1, g_param2, g_param3);

			g_uart_test_mode = 1;
			for(i=0; i<sizeof(works)/sizeof(struct test_list); i++)
			{
				printf("%s:%s\n",g_cmd,works[i].cmd);
				if(strcmp(g_cmd, works[i].cmd) == 0)
				{
					//if(row==0)
					//	fill_screen(back_color);
					ui_puts(works[i].name, row, col, font_color);
					ret = works[i].test();
					if(ret == OK)
					{
						ui_puts("OK", row, col + 16, GREEN_COLOR);
								
					}
					else
					{
						ui_puts("FAL", row, col + 16, RED_COLOR);
					
					}
					row++;


					if(g_board_ext_info[0] != 0){
						wlen = sprintf(wbuf, "RSP(%s,%s)", (ret==0)?"1":"0", g_board_ext_info);
						memset(g_board_ext_info, 0 , 50);
					} else {
						if (ret != 0)
						{
							wlen = sprintf(wbuf, "RSP(%s,%s Failed)", (ret==0)?"1":"0", works[i].name);
						}
						else
						{
							wlen = sprintf(wbuf, "RSP(%s,%s)", (ret==0)?"1":"0", works[i].name);
						}
					}
					if(uart_port == UART_PORT_FIQ)
						uart_send(wbuf, wlen);
					else
						virt_uart_send(wbuf, wlen);
					break;
				}
			}
			if (i == sizeof(works)/sizeof(struct test_list))
			{

				wlen = sprintf(wbuf, "RSP(%s,%s)", "0", "Invalid cmd");
				if(uart_port == UART_PORT_FIQ)
					uart_send(wbuf, wlen);
				else
					virt_uart_send(wbuf, wlen);
			}
			memset(g_cmd, 0, 32);
			g_param1 = 0;
			g_param2 = 0;
			g_param3 = 0;
			g_uart_test_mode = 0;
			if(row == max_row && col==1 )
			{
				row = 1;
				col = (max_col-1)/2;
			}
		}//continue;
		//		else{
		//			pr_info("Unrecognize keycode: %d\n", key);
		//			continue;
		//		}

		//		draw_all_item(g_cur_menu, len);
		//		set_frontcolor(COLOR_GREEN);
		//lcd_printf("Enter", MENU_POSY, 3);
		//lcd_printf("Back", MENU_POSY, MENU_POSX);

		//}
	}
	  uart_uninit();
	//   virt_uart_uninit();
	//   close_input_device();

	// return 0;

}
