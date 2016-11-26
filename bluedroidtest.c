/******************************************************************************
 *
 *  Copyright (C) 2009-2012 Broadcom Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

/************************************************************************************
 *
 *  Filename:      bluedroidtest.c
 *
 *  Description:   Bluedroid Test application
 *
 ***********************************************************************************/

#include <stdio.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/prctl.h>
#include <sys/capability.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>

#include <private/android_filesystem_config.h>
#include <android/log.h>
#include <cutils/properties.h>

#include <hardware/hardware.h>
#include <hardware/bluetooth.h>

/************************************************************************************
**  Constants & Macros
************************************************************************************/

#define PID_FILE "/data/.bdt_pid"

#ifndef MAX
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif

#ifndef FALSE
#define TRUE 1
#define FALSE 0
#endif

#if 1
#define DEBUG(fmt,args...)  printf(fmt,##args) 
#else
#define DEBUG(fmt,args...)  do{}while(0) 
#endif

typedef unsigned short boolean;

#define CASE_RETURN_STR(const) case const: return #const;

#define UNUSED __attribute__((unused))

/************************************************************************************
**  Local type definitions
************************************************************************************/

/************************************************************************************
**  Static variables
************************************************************************************/

static unsigned char main_done = 0;
static bt_status_t status;

/* Main API */
static bluetooth_device_t* bt_device;

const bt_interface_t* sBtInterface = NULL;

static gid_t groups[] = { AID_NET_BT, AID_INET, AID_NET_BT_ADMIN,
                          AID_SYSTEM, AID_MISC, AID_SDCARD_RW,
                          AID_NET_ADMIN, AID_VPN};

/* Set to 1 when the Bluedroid stack is enabled */
static unsigned char bt_enabled = 0;

/************************************************************************************
**  Static functions
************************************************************************************/

static void process_cmd(char *p, unsigned char is_job);
static void job_handler(void *param);
static void bdt_log(const char *fmt_str, ...);


/************************************************************************************
**  Externs
************************************************************************************/

/************************************************************************************
**  Functions
************************************************************************************/

typedef char bdstr_t[18];

#define FACTORY_BT_BDADDR_STORAGE_LEN   17
#define ISXDIGIT(a) (((a>='0') && (a<='9'))||((a>='A') && (a<='F'))||((a>='a') && (a<='f')))

//
// taken from external/bluetooth/bluedroid/btif/src/btif_util.c
// -------------------------------------------------

/**
 * PROPERTY_BT_BDADDR_PATH
 * The property key stores the storage location of Bluetooth Device Address
 */
#ifndef PROPERTY_BT_BDADDR_PATH
#define PROPERTY_BT_BDADDR_PATH         "ro.bt.bdaddr_path"
#endif

/**
 * PERSIST_BDADDR_PROPERTY
 * If there is no valid bdaddr available from PROPERTY_BT_BDADDR_PATH,
 * generating a random BDADDR and keeping it in the PERSIST_BDADDR_DROP.
 */
#ifndef PERSIST_BDADDR_PROPERTY
#define PERSIST_BDADDR_PROPERTY         "persist.service.bdroid.bdaddr"
#endif

const char* dump_property_type(bt_property_type_t type)
{
    switch(type)
    {
        CASE_RETURN_STR(BT_PROPERTY_BDNAME);
        CASE_RETURN_STR(BT_PROPERTY_BDADDR);
        CASE_RETURN_STR(BT_PROPERTY_UUIDS);
        CASE_RETURN_STR(BT_PROPERTY_CLASS_OF_DEVICE);
        CASE_RETURN_STR(BT_PROPERTY_TYPE_OF_DEVICE);
        CASE_RETURN_STR(BT_PROPERTY_REMOTE_RSSI);
        CASE_RETURN_STR(BT_PROPERTY_ADAPTER_DISCOVERY_TIMEOUT);
        CASE_RETURN_STR(BT_PROPERTY_ADAPTER_BONDED_DEVICES);
        CASE_RETURN_STR(BT_PROPERTY_ADAPTER_SCAN_MODE);
        CASE_RETURN_STR(BT_PROPERTY_REMOTE_FRIENDLY_NAME);

        default:
            return "UNKNOWN PROPERTY ID";
    }
}

const char* dump_device_type(bt_device_type_t type)
{
    switch (type)
    {
        CASE_RETURN_STR(BT_DEVICE_DEVTYPE_BREDR);
        CASE_RETURN_STR(BT_DEVICE_DEVTYPE_BLE);
        CASE_RETURN_STR(BT_DEVICE_DEVTYPE_DUAL);

        default:
            return "UNKNOWN DEVICE TYPE";
    }
}

char *bd2str(bt_bdaddr_t *bdaddr, bdstr_t *bdstr)
{
    char *addr = (char *) bdaddr->address;

    sprintf((char*)bdstr, "%02x:%02x:%02x:%02x:%02x:%02x",
                       (int)addr[0],(int)addr[1],(int)addr[2],
                       (int)addr[3],(int)addr[4],(int)addr[5]);
    return (char *)bdstr;
}

int str2bd(char *str, bt_bdaddr_t *addr)
{
    int32_t i = 0;
    for (i = 0; i < 6; i++) {
       addr->address[i] = (uint8_t) strtoul(str, (char **)&str, 16);
       str++;
    }
    return 0;
}

void uuid_to_string(bt_uuid_t *p_uuid, char *str)
{
    uint32_t uuid0, uuid4;
    uint16_t uuid1, uuid2, uuid3, uuid5;

    memcpy(&uuid0, &(p_uuid->uu[0]), 4);
    memcpy(&uuid1, &(p_uuid->uu[4]), 2);
    memcpy(&uuid2, &(p_uuid->uu[6]), 2);
    memcpy(&uuid3, &(p_uuid->uu[8]), 2);
    memcpy(&uuid4, &(p_uuid->uu[10]), 4);
    memcpy(&uuid5, &(p_uuid->uu[14]), 2);

    sprintf((char *)str, "%.8x-%.4x-%.4x-%.4x-%.8x%.4x",
            ntohl(uuid0), ntohs(uuid1),
            ntohs(uuid2), ntohs(uuid3),
            ntohl(uuid4), ntohs(uuid5));
    return;
}


void debug_bt_property_t(bt_property_t prop) {
    bt_property_type_t ptype = prop.type;
    bt_device_type_t dtype;
    bt_bdaddr_t addr;
    char text[2000];

    DEBUG("%s: ", dump_property_type(ptype));	
    memset(text, 0, sizeof(char)*2000);
    switch(prop.type){
	case BT_PROPERTY_BDNAME:
	    memcpy(&text, prop.val, prop.len);
	    DEBUG("%s", text);
	    break;
	case BT_PROPERTY_BDADDR:
	    memcpy(&addr, prop.val, prop.len);
	    DEBUG("%s", bd2str(&addr, (bdstr_t *)&text));
	    break;
	case BT_PROPERTY_TYPE_OF_DEVICE:
	    memcpy(&dtype, prop.val, prop.len);
	    DEBUG("%s", dump_device_type(dtype));
	    break;
	case BT_PROPERTY_ADAPTER_SCAN_MODE:
	case BT_PROPERTY_ADAPTER_DISCOVERY_TIMEOUT:
	    DEBUG("%i", (int)prop.val);
	default:
	    break;
    }

    DEBUG("\n");
}

#define DEBUG_STATUS_PROP()                                             \
    DEBUG("%s: Status is: %d, Properties: %d\n", __FUNCTION__, status, num_properties)

#define DEBUG_STATUS()                          \
    DEBUG("%s: Status is: %d\n", __FUNCTION__, status)

/************************************************************************************
**  Shutdown helper functions
************************************************************************************/

static void bdt_shutdown(void)
{
    bdt_log("shutdown bdroid test app\n");
    main_done = 1;
}


/*****************************************************************************
** Android's init.rc does not yet support applying linux capabilities
*****************************************************************************/

static void config_permissions(void)
{
    struct __user_cap_header_struct header;
    struct __user_cap_data_struct cap;

    bdt_log("set_aid_and_cap : pid %d, uid %d gid %d", getpid(), getuid(), getgid());

    header.pid = 0;

    prctl(PR_SET_KEEPCAPS, 1, 0, 0, 0);

    setuid(AID_BLUETOOTH);
    setgid(AID_BLUETOOTH);

    header.version = _LINUX_CAPABILITY_VERSION;

    cap.effective = cap.permitted =  cap.inheritable =
                    1 << CAP_NET_RAW |
                    1 << CAP_NET_ADMIN |
                    1 << CAP_NET_BIND_SERVICE |
                    1 << CAP_SYS_RAWIO |
                    1 << CAP_SYS_NICE |
                    1 << CAP_SETGID;

    capset(&header, &cap);
    setgroups(sizeof(groups)/sizeof(groups[0]), groups);
}



/*****************************************************************************
**   Logger API
*****************************************************************************/

void bdt_log(const char *fmt_str, ...)
{
    static char buffer[1024];
    va_list ap;

    va_start(ap, fmt_str);
    vsnprintf(buffer, 1024, fmt_str, ap);
    va_end(ap);

    fprintf(stdout, "%s\n", buffer);
}

/*******************************************************************************
 ** Misc helper functions
 *******************************************************************************/
static const char* dump_bt_status(bt_status_t status)
{
    switch(status)
    {
        CASE_RETURN_STR(BT_STATUS_SUCCESS)
        CASE_RETURN_STR(BT_STATUS_FAIL)
        CASE_RETURN_STR(BT_STATUS_NOT_READY)
        CASE_RETURN_STR(BT_STATUS_NOMEM)
        CASE_RETURN_STR(BT_STATUS_BUSY)
        CASE_RETURN_STR(BT_STATUS_UNSUPPORTED)

        default:
            return "unknown status code";
    }
}

static void hex_dump(char *msg, void *data, int size, int trunc)
{
    unsigned char *p = data;
    unsigned char c;
    int n;
    char bytestr[4] = {0};
    char addrstr[10] = {0};
    char hexstr[ 16*3 + 5] = {0};
    char charstr[16*1 + 5] = {0};

    bdt_log("%s  \n", msg);

    /* truncate */
    if(trunc && (size>32))
        size = 32;

    for(n=1;n<=size;n++) {
        if (n%16 == 1) {
            /* store address for this line */
            snprintf(addrstr, sizeof(addrstr), "%.4x",
               (unsigned int)((uintptr_t)p-(uintptr_t)data) );
        }

        c = *p;
        if (isalnum(c) == 0) {
            c = '.';
        }

        /* store hex str (for left side) */
        snprintf(bytestr, sizeof(bytestr), "%02X ", *p);
        strncat(hexstr, bytestr, sizeof(hexstr)-strlen(hexstr)-1);

        /* store char str (for right side) */
        snprintf(bytestr, sizeof(bytestr), "%c", c);
        strncat(charstr, bytestr, sizeof(charstr)-strlen(charstr)-1);

        if(n%16 == 0) {
            /* line completed */
            bdt_log("[%4.4s]   %-50.50s  %s\n", addrstr, hexstr, charstr);
            hexstr[0] = 0;
            charstr[0] = 0;
        } else if(n%8 == 0) {
            /* half line: add whitespaces */
            strncat(hexstr, "  ", sizeof(hexstr)-strlen(hexstr)-1);
            strncat(charstr, " ", sizeof(charstr)-strlen(charstr)-1);
        }
        p++; /* next byte */
    }

    if (strlen(hexstr) > 0) {
        /* print rest of buffer if not empty */
        bdt_log("[%4.4s]   %-50.50s  %s\n", addrstr, hexstr, charstr);
    }
}

/*******************************************************************************
 ** Console helper functions
 *******************************************************************************/

void skip_blanks(char **p)
{
  while (**p == ' ')
    (*p)++;
}

uint32_t get_int(char **p, int DefaultValue)
{
  uint32_t Value = 0;
  unsigned char   UseDefault;

  UseDefault = 1;
  skip_blanks(p);

  while ( ((**p)<= '9' && (**p)>= '0') )
    {
      Value = Value * 10 + (**p) - '0';
      UseDefault = 0;
      (*p)++;
    }

  if (UseDefault)
    return DefaultValue;
  else
    return Value;
}

int get_signed_int(char **p, int DefaultValue)
{
  int    Value = 0;
  unsigned char   UseDefault;
  unsigned char  NegativeNum = 0;

  UseDefault = 1;
  skip_blanks(p);

  if ( (**p) == '-')
    {
      NegativeNum = 1;
      (*p)++;
    }
  while ( ((**p)<= '9' && (**p)>= '0') )
    {
      Value = Value * 10 + (**p) - '0';
      UseDefault = 0;
      (*p)++;
    }

  if (UseDefault)
    return DefaultValue;
  else
    return ((NegativeNum == 0)? Value : -Value);
}

void get_str(char **p, char *Buffer)
{
  skip_blanks(p);

  while (**p != 0 && **p != ' ')
    {
      *Buffer = **p;
      (*p)++;
      Buffer++;
    }

  *Buffer = 0;
}

uint32_t get_hex(char **p, int DefaultValue)
{
  uint32_t Value = 0;
  unsigned char   UseDefault;

  UseDefault = 1;
  skip_blanks(p);

  while ( ((**p)<= '9' && (**p)>= '0') ||
          ((**p)<= 'f' && (**p)>= 'a') ||
          ((**p)<= 'F' && (**p)>= 'A') )
    {
      if (**p >= 'a')
        Value = Value * 16 + (**p) - 'a' + 10;
      else if (**p >= 'A')
        Value = Value * 16 + (**p) - 'A' + 10;
      else
        Value = Value * 16 + (**p) - '0';
      UseDefault = 0;
      (*p)++;
    }

  if (UseDefault)
    return DefaultValue;
  else
    return Value;
}

void get_bdaddr(const char *str, bt_bdaddr_t *bd) {
    char *d = ((char *)bd), *endp;
    int i;
    for(i = 0; i < 6; i++) {
        *d++ = strtol(str, &endp, 16);
        if (*endp != ':' && i != 5) {
            memset(bd, 0, sizeof(bt_bdaddr_t));
            return;
        }
        str = endp + 1;
    }
}

#define is_cmd(str) ((strlen(str) == strlen(cmd)) && strncmp((const char *)&cmd, str, strlen(str)) == 0)
#define if_cmd(str)  if (is_cmd(str))

typedef void (t_console_cmd_handler) (char *p);

typedef struct {
    const char *name;
    t_console_cmd_handler *handler;
    const char *help;
    unsigned char is_job;
} t_cmd;


const t_cmd console_cmd_list[];
static int console_cmd_maxlen = 0;

static void cmdjob_handler(void *param)
{
    char *job_cmd = (char*)param;

    bdt_log("cmdjob starting (%s)", job_cmd);

    process_cmd(job_cmd, 1);

    bdt_log("cmdjob terminating");

    free(job_cmd);
}

static int create_cmdjob(char *cmd)
{
    pthread_t thread_id;
    char *job_cmd;

    job_cmd = malloc(strlen(cmd)+1); /* freed in job handler */
    strcpy(job_cmd, cmd);

    if (pthread_create(&thread_id, NULL,
                       (void*)cmdjob_handler, (void*)job_cmd)!=0)
      perror("pthread_create");

    return 0;
}

/*******************************************************************************
 ** Load stack lib
 *******************************************************************************/

int HAL_load(void)
{
    int err = 0;

    hw_module_t* module;
    hw_device_t* device;

    bdt_log("Loading HAL lib + extensions");

    err = hw_get_module(BT_HARDWARE_MODULE_ID, (hw_module_t const**)&module);
    if (err == 0)
    {
        err = module->methods->open(module, BT_HARDWARE_MODULE_ID, &device);
        if (err == 0) {
            bt_device = (bluetooth_device_t *)device;
            sBtInterface = bt_device->get_bluetooth_interface();
        }
    }

    bdt_log("HAL library loaded (%s)", strerror(err));

    return err;
}

int HAL_unload(void)
{
    int err = 0;

    bdt_log("Unloading HAL lib");

    sBtInterface = NULL;

    bdt_log("HAL library unloaded (%s)", strerror(err));

    return err;
}

/*******************************************************************************
 ** HAL test functions & callbacks
 *******************************************************************************/

void setup_test_env(void)
{
    int i = 0;

    while (console_cmd_list[i].name != NULL)
    {
        console_cmd_maxlen = MAX(console_cmd_maxlen, (int)strlen(console_cmd_list[i].name));
        i++;
    }
}

void check_return_status(bt_status_t status)
{
    if (status != BT_STATUS_SUCCESS)
    {
        bdt_log("HAL REQUEST FAILED status : %d (%s)", status, dump_bt_status(status));
    }
    else
    {
        bdt_log("HAL REQUEST SUCCESS");
    }
}

static void adapter_state_changed(bt_state_t state)
{
    bdt_log("ADAPTER STATE UPDATED : %s", (state == BT_STATE_OFF)?"OFF":"ON");
    if (state == BT_STATE_ON) {
        bt_enabled = 1;
    } else {
        bt_enabled = 0;
    }
}

void adapter_properties_cb(bt_status_t status,
                           int num_properties,
                           bt_property_t *properties) {
    DEBUG_STATUS_PROP();
    int i;

    if (status != BT_STATUS_SUCCESS) {
        return;
    }

    for (i = 0; i < num_properties; i++) {
        debug_bt_property_t(properties[i]);
    }

}

void remote_device_properties_cb(bt_status_t status, bt_bdaddr_t *bd_addr,
                                              int num_properties, bt_property_t *properties) {
    int i;
    DEBUG_STATUS_PROP();
    if (status != BT_STATUS_SUCCESS) {
        return;
    }

    for (i = 0; i < num_properties; i++) {
        debug_bt_property_t(properties[i]);
    }

    return;
}


void device_found_cb(int num_properties, bt_property_t *properties) {
    int addr_index = -1;
    int i;
    DEBUG("%s: Properties: %i\n", __FUNCTION__, num_properties);

    for (i = 0; i < num_properties; i++) {
        if (properties[i].type == BT_PROPERTY_BDADDR) {
            addr_index = i;
        }
        debug_bt_property_t(properties[i]);
    }

    if ( addr_index == -1 ){
        DEBUG("no bluetooth address provided\n");
        return;
    }

    remote_device_properties_cb(BT_STATUS_SUCCESS,
                                      (bt_bdaddr_t *)properties[addr_index].val,
                                      num_properties, properties);

}

void bond_state_changed_cb(bt_status_t status, bt_bdaddr_t *bd_addr,
                                        bt_bond_state_t state) {
    DEBUG_STATUS();
    if (!bd_addr){
        DEBUG("Address is null in %s\n", __FUNCTION__);
        return;
    }
}

void acl_state_changed_cb(bt_status_t status, bt_bdaddr_t *bd_addr,
                                       bt_acl_state_t state)
{
    DEBUG_STATUS();
    if (!bd_addr) {
        DEBUG("Address is null in %s\n", __FUNCTION__);
        return;
    }
}

void discovery_state_changed_cb(bt_discovery_state_t state) {
    DEBUG("%s: DiscoveryState: %d \n", __FUNCTION__, state);
}

void pin_request_cb(bt_bdaddr_t *bd_addr, bt_bdname_t *bdname, uint32_t cod) {
    DEBUG("%s\n", __FUNCTION__);
    if (!bd_addr) {
        DEBUG("Address is null in %s\n", __FUNCTION__);
        return;
    }
    return;
}

void ssp_request_cb(bt_bdaddr_t *bd_addr, bt_bdname_t *bdname, uint32_t cod,
                                 bt_ssp_variant_t pairing_variant, uint32_t pass_key) {
    DEBUG("%s\n", __FUNCTION__);
    return;
}

void thread_event_cb(bt_cb_thread_evt event) {
    if (event  == ASSOCIATE_JVM) {
        DEBUG("%s: ASSOCIATE\n", __FUNCTION__);

    } else if (event == DISASSOCIATE_JVM) {
        DEBUG("%s: DISASSOCIATE\n", __FUNCTION__);
    }
}

static void dut_mode_recv(uint16_t UNUSED opcode, uint8_t UNUSED *buf, uint8_t UNUSED len)
{
    bdt_log("DUT MODE RECV : NOT IMPLEMENTED");
}

static void le_test_mode(bt_status_t status, uint16_t packet_count)
{
    bdt_log("LE TEST MODE END status:%s number_of_packets:%d", dump_bt_status(status), packet_count);
}

static bt_callbacks_t bt_callbacks = {
    sizeof(bt_callbacks_t),
    adapter_state_changed,
    adapter_properties_cb,
    remote_device_properties_cb,
    device_found_cb,
    discovery_state_changed_cb,
    pin_request_cb,
    ssp_request_cb,
    bond_state_changed_cb,
    acl_state_changed_cb,
    thread_event_cb,
    dut_mode_recv, /* dut_mode_recv_cb */
#if BLE_INCLUDED == TRUE
    le_test_mode, /* le_test_mode_cb */
#else
    NULL, /* le_test_mode_cb */
#endif
    NULL /* energy_info_cb */
};

static bool set_wake_alarm(uint64_t delay_millis, bool should_wake, alarm_cb cb, void *data) {
  static timer_t timer;
  static bool timer_created;

  if (!timer_created) {
    struct sigevent sigevent;
    memset(&sigevent, 0, sizeof(sigevent));
    sigevent.sigev_notify = SIGEV_THREAD;
    sigevent.sigev_notify_function = (void (*)(union sigval))cb;
    sigevent.sigev_value.sival_ptr = data;
    timer_create(CLOCK_MONOTONIC, &sigevent, &timer);
    timer_created = true;
  }

  struct itimerspec new_value;
  new_value.it_value.tv_sec = delay_millis / 1000;
  new_value.it_value.tv_nsec = (delay_millis % 1000) * 1000 * 1000;
  new_value.it_interval.tv_sec = 0;
  new_value.it_interval.tv_nsec = 0;
  timer_settime(timer, 0, &new_value, NULL);

  return true;
}

static int acquire_wake_lock(const char *lock_name) {
  return BT_STATUS_SUCCESS;
}

static int release_wake_lock(const char *lock_name) {
  return BT_STATUS_SUCCESS;
}

static bt_os_callouts_t callouts = {
    sizeof(bt_os_callouts_t),
    set_wake_alarm,
    acquire_wake_lock,
    release_wake_lock,
};

void bdt_init(void)
{
    bdt_log("INIT BT ");
    status = sBtInterface->init(&bt_callbacks);

    if (status == BT_STATUS_SUCCESS) {
        status = sBtInterface->set_os_callouts(&callouts);
    }

    check_return_status(status);
}

void bdt_enable(void)
{
    bdt_log("ENABLE BT");
    if (bt_enabled) {
        bdt_log("Bluetooth is already enabled");
        return;
    }
    status = sBtInterface->enable();

    check_return_status(status);
}

void bdt_scan(void)
{
    bdt_log("SCAN BT");
    if (!bt_enabled) {
        bdt_log("Bluetooth is not enabled");
        return;
    }
    status = sBtInterface->start_discovery();

    check_return_status(status);
}

void bdt_disable(void)
{
    bdt_log("DISABLE BT");
    if (!bt_enabled) {
        bdt_log("Bluetooth is already disabled");
        return;
    }
    status = sBtInterface->disable();

    check_return_status(status);
}
void bdt_dut_mode_configure(char *p)
{
    int32_t mode = -1;

    bdt_log("BT DUT MODE CONFIGURE");
    if (!bt_enabled) {
        bdt_log("Bluetooth must be enabled for test_mode to work.");
        return;
    }
    mode = get_signed_int(&p, mode);
    if ((mode != 0) && (mode != 1)) {
        bdt_log("Please specify mode: 1 to enter, 0 to exit");
        return;
    }
    status = sBtInterface->dut_mode_configure(mode);

    check_return_status(status);
}

#define HCI_LE_RECEIVER_TEST_OPCODE 0x201D
#define HCI_LE_TRANSMITTER_TEST_OPCODE 0x201E
#define HCI_LE_END_TEST_OPCODE 0x201F

void bdt_le_test_mode(char *p)
{
    int cmd;
    unsigned char buf[3];
    int arg1, arg2, arg3;

    bdt_log("BT LE TEST MODE");
    if (!bt_enabled) {
        bdt_log("Bluetooth must be enabled for le_test to work.");
        return;
    }

    memset(buf, 0, sizeof(buf));
    cmd = get_int(&p, 0);
    switch (cmd)
    {
        case 0x1: /* RX TEST */
           arg1 = get_int(&p, -1);
           if (arg1 < 0) bdt_log("%s Invalid arguments", __FUNCTION__);
           buf[0] = arg1;
           status = sBtInterface->le_test_mode(HCI_LE_RECEIVER_TEST_OPCODE, buf, 1);
           break;
        case 0x2: /* TX TEST */
            arg1 = get_int(&p, -1);
            arg2 = get_int(&p, -1);
            arg3 = get_int(&p, -1);
            if ((arg1 < 0) || (arg2 < 0) || (arg3 < 0))
                bdt_log("%s Invalid arguments", __FUNCTION__);
            buf[0] = arg1;
            buf[1] = arg2;
            buf[2] = arg3;
            status = sBtInterface->le_test_mode(HCI_LE_TRANSMITTER_TEST_OPCODE, buf, 3);
           break;
        case 0x3: /* END TEST */
            status = sBtInterface->le_test_mode(HCI_LE_END_TEST_OPCODE, buf, 0);
           break;
        default:
            bdt_log("Unsupported command");
            return;
            break;
    }
    if (status != BT_STATUS_SUCCESS)
    {
        bdt_log("%s Test 0x%x Failed with status:0x%x", __FUNCTION__, cmd, status);
    }
    return;
}

void bdt_cleanup(void)
{
    bdt_log("CLEANUP");
    sBtInterface->cleanup();
}

/*******************************************************************************
 ** Console commands
 *******************************************************************************/

void do_help(char UNUSED *p)
{
    int i = 0;
    int max = 0;
    char line[128];
    int pos = 0;

    while (console_cmd_list[i].name != NULL)
    {
        pos = sprintf(line, "%s", (char*)console_cmd_list[i].name);
        bdt_log("%s %s\n", (char*)line, (char*)console_cmd_list[i].help);
        i++;
    }
}

void do_quit(char UNUSED *p)
{
    bdt_shutdown();
}

/*******************************************************************
 *
 *  BT TEST  CONSOLE COMMANDS
 *
 *  Parses argument lists and passes to API test function
 *
*/

void do_init(char UNUSED *p)
{
    bdt_init();
}

void do_enable(char UNUSED *p)
{
    bdt_enable();
}

void do_scan(char UNUSED *p)
{
    bdt_scan();
}

void do_disable(char UNUSED *p)
{
    bdt_disable();
}
void do_dut_mode_configure(char *p)
{
    bdt_dut_mode_configure(p);
}

void do_le_test_mode(char *p)
{
    bdt_le_test_mode(p);
}

void do_cleanup(char UNUSED *p)
{
    bdt_cleanup();
}

//
// taken from external/bluetooth/bluedroid/btif/src/btif_core.c
// -------------------------------------------------

void do_writemac(char *write_mac)
{
    int addr_fd;
    bt_bdaddr_t addr;
    const uint8_t null_bdaddr[6] = {0,0,0,0,0,0};

    if(!ISXDIGIT(write_mac[0]) ||!ISXDIGIT(write_mac[1]) \
	||!ISXDIGIT(write_mac[3]) ||!ISXDIGIT(write_mac[4]) \
	||!ISXDIGIT(write_mac[6]) ||!ISXDIGIT(write_mac[7]) \
	||!ISXDIGIT(write_mac[9]) ||!ISXDIGIT(write_mac[10]) \
	||!ISXDIGIT(write_mac[12]) ||!ISXDIGIT(write_mac[13]) \
	||!ISXDIGIT(write_mac[15]) ||!ISXDIGIT(write_mac[16]) \
	||':'!=write_mac[2] ||':'!=write_mac[5]||':'!=write_mac[8]\
	||':'!=write_mac[11]||':'!=write_mac[14]){
	bdt_log("invalid mac address %s, while the expected format is xx:xx:xx:xx:xx:xx\n",write_mac);
	return;
    }

    str2bd(write_mac, &addr);
    if (memcmp(addr.address, null_bdaddr, 6) == 0){
      bdt_log("invalid mac address:%s, it must NOT be all 0",write_mac);
      return;
    }
	
    if ((addr_fd = open("/data/misc/bluedroid/bt_config.xml",  O_RDWR)) != -1)
    {
        char data[1024];
        int ret;
        // <N2 Tag="Address" Type="string">22:22:ac:02:ba:63</N2>
        read(addr_fd, data, sizeof data);
        if((ret = strstr(data, "Tag=\"Address\" Type=\"string\">"))!=0){
        	ret = (int)ret - (int)data + strlen("Tag=\"Address\" Type=\"string\">");
        	lseek(addr_fd, ret, SEEK_SET);
        	write(addr_fd, write_mac,17);	
        }
        close(addr_fd);
    }

    if (property_set(PERSIST_BDADDR_PROPERTY, (char*)write_mac) < 0)
        bdt_log("Failed to set random BDA in prop %s",PERSIST_BDADDR_PROPERTY);
    else
        bdt_log(":: do_writemac %s",write_mac);
}

static void readmac(bt_bdaddr_t * local_addr)
{
     int addr_fd;
     uint8_t valid_bda = FALSE;
     const uint8_t null_bdaddr[6] = {0,0,0,0,0,0};
     char val[256];

     /* Get local bdaddr storage path from property */
     if (property_get(PROPERTY_BT_BDADDR_PATH, val, NULL))
     {
        int addr_fd;

        if ((addr_fd = open(val, O_RDONLY)) != -1)
        {
            bdt_log("local bdaddr is stored in %s", val);

            memset(val, 0, sizeof(val));
            read(addr_fd, val, 17);
            str2bd(val, local_addr);
            /* If this is not a reserved/special bda, then use it */
            if (memcmp(local_addr->address, null_bdaddr, 6) != 0)
            {
                valid_bda = TRUE;
                bdt_log("Got Factory BDA %02X:%02X:%02X:%02X:%02X:%02X",
                    local_addr->address[0], local_addr->address[1], local_addr->address[2],
                    local_addr->address[3], local_addr->address[4], local_addr->address[5]);
            }

            close(addr_fd);
        }
     }

     /* Look for local bdaddr from bt_config.xml */
     if(!valid_bda)
     {
	    if ((addr_fd = open("/data/misc/bluedroid/bt_config.xml",  O_RDWR)) != -1)
	    {
	        char data[1024];
	        int ret;
	        // <N2 Tag="Address" Type="string">22:22:ac:02:ba:63</N2>
	        read(addr_fd, data, sizeof data);
	        if((ret = strstr(data, "Tag=\"Address\" Type=\"string\">"))!=0){
        		ret = (int)ret - (int)data + strlen("Tag=\"Address\" Type=\"string\">");
	        	lseek(addr_fd, ret, SEEK_SET);
			memset(val, 0, sizeof(val));
	        	read(addr_fd, val, 17);
	        }
	            str2bd(val, local_addr);
	            /* If this is not a reserved/special bda, then use it */
	            if (memcmp(local_addr->address, null_bdaddr, 6) != 0)
	            {
	                valid_bda = TRUE;
	                bdt_log("Got local bdaddr from bt_config.xml is %02X:%02X:%02X:%02X:%02X:%02X",
	                    local_addr->address[0], local_addr->address[1], local_addr->address[2],
	                    local_addr->address[3], local_addr->address[4], local_addr->address[5]);
	            }

	            close(addr_fd);
	    }
     }  

     /* No factory BDADDR found. Look for previously generated random BDA */
    if ((!valid_bda) && \
        (property_get(PERSIST_BDADDR_PROPERTY, val, NULL)))
    {
        str2bd(val, local_addr);
        valid_bda = TRUE;
        bdt_log("Got prior random BDA %02X:%02X:%02X:%02X:%02X:%02X",
            local_addr->address[0], local_addr->address[1], local_addr->address[2],
            local_addr->address[3], local_addr->address[4], local_addr->address[5]);
    }
	 	
    /* Generate new BDA if necessary */
    if (!valid_bda)
    {
        bdstr_t bdstr;
		
	/* Seed the random number generator */
        srand((unsigned int) (time(0)));

        /* No autogen BDA. Generate one now. */
        local_addr->address[0] = 0x22;
        local_addr->address[1] = 0x22;
        local_addr->address[2] = (uint8_t) ((rand() >> 8) & 0xFF);
        local_addr->address[3] = (uint8_t) ((rand() >> 8) & 0xFF);
        local_addr->address[4] = (uint8_t) ((rand() >> 8) & 0xFF);
        local_addr->address[5] = (uint8_t) ((rand() >> 8) & 0xFF);

        /* Convert to ascii, and store as a persistent property */
        bd2str(local_addr, &bdstr);

        bdt_log("No preset BDA. Generating BDA: %s for prop %s",
             (char*)bdstr, PERSIST_BDADDR_PROPERTY);

        if (property_set(PERSIST_BDADDR_PROPERTY, (char*)bdstr) < 0)
            bdt_log("Failed to set random BDA in prop %s",PERSIST_BDADDR_PROPERTY);

        //save the bd address to config file
        if ((addr_fd = open("/data/misc/bluedroid/bt_config.xml",  O_RDWR)) != -1)
        {
            char data[1024];
            int ret;
            // <N2 Tag="Address" Type="string">22:22:ac:02:ba:63</N2>
            read(addr_fd, data, sizeof data);
            if((ret = strstr(data, "Tag=\"Address\" Type=\"string\">"))!=0){
        	    ret = (int)ret - (int)data + strlen("Tag=\"Address\" Type=\"string\">");
        	    lseek(addr_fd, ret, SEEK_SET);
        	    write(addr_fd, (char*)bdstr,17);	
            }
            close(addr_fd);
            }
	}
}

void do_readmac(char *p)
{
	bt_bdaddr_t addr;
	bt_bdaddr_t  * local_addr = &addr;

	memset(&addr,sizeof addr,0);
	readmac(local_addr);
	bdt_log(":: do_readmac %02X:%02X:%02X:%02X:%02X:%02X",
	                    local_addr->address[0], local_addr->address[1], local_addr->address[2],
	                    local_addr->address[3], local_addr->address[4], local_addr->address[5]);
}
/*******************************************************************
 *
 *  CONSOLE COMMAND TABLE
 *
*/

const t_cmd console_cmd_list[] =
{
    /*
     * INTERNAL
     */

    { "help", do_help, ":: lists all available console commands", 0 },
    { "quit", do_quit, ":: quit from this tool", 0},

    /*
     * API CONSOLE COMMANDS
     */

     /* Init and Cleanup shall be called automatically */
    { "enable", do_enable, ":: enables bluetooth", 0 },
    { "scan", do_scan, ":: scan bluetooth devices", 0 },
    { "readmac", do_readmac, ":: read mac address", 0 },    
    { "writemac xx:xx:xx:xx:xx:xx", do_writemac, ":: write mac address", 0 },    
    { "disable", do_disable, ":: disables bluetooth", 0 },
    { "dut_mode_configure", do_dut_mode_configure, ":: DUT mode - 1 to enter,0 to exit", 0 },
    { "le_test_mode", do_le_test_mode, ":: LE Test Mode - RxTest - 1 <rx_freq>, \n\t \
                      TxTest - 2 <tx_freq> <test_data_len> <payload_pattern>, \n\t \
                      End Test - 3 <no_args>", 0 },
    /* add here */

    /* last entry */
    {NULL, NULL, "", 0},
};

/*
 * Main console command handler
*/

static void process_cmd(char *p, unsigned char is_job)
{
    char cmd[64];
    int i = 0;
    char *p_saved = p;

    get_str(&p, cmd);

    /* table commands */
    while (console_cmd_list[i].name != NULL)
    {
        if (is_cmd(console_cmd_list[i].name))
        {
            if (!is_job && console_cmd_list[i].is_job)
                create_cmdjob(p_saved);
            else
            {
                console_cmd_list[i].handler(p);
            }
            return;
        }
        i++;
    }
    bdt_log("%s : unknown command\n", p_saved);
    do_help(NULL);
}
 
int test_app (int UNUSED argc, char UNUSED *argv[])
{
    int opt;
    char cmd[128];
    int args_processed = 0;
    int pid = -1;
    int flag = 0;

	while((opt=getopt(argc, argv, "t:w:r:o:"))>0)
	{
		switch (opt)
		{
		case 't':
			flag = 1;
			break;
		case 'w':
			do_writemac(optarg);
			return 0;
		case 'r':
			do_readmac(NULL);
			return 0;
		case 'o':
		    if ( HAL_load() < 0 ) {
		        perror("HAL failed to initialize, exit\n");
		        exit(0);
		    }
		    bdt_init();
		    bdt_enable();
			return 0;
		default:
			break;
		}
	}

    config_permissions();
    bdt_log("\n:::::::::::::::::::::::::::::::::::::::::::::::::::");
    bdt_log(":: Bluedroid test app starting");

    if ( HAL_load() < 0 ) {
        perror("HAL failed to initialize, exit\n");
        unlink(PID_FILE);
        exit(0);
    }

    setup_test_env();

    /* Automatically perform the init */
    bdt_init();

    if(1 == flag){
        process_cmd("enable", 0);
        sleep(3);
        process_cmd("disable", 0);
        sleep(1);
        process_cmd("quit", 0);
        goto here;
    }

    while(!main_done)
    {
        char line[128];

        /* command prompt */
        printf( ">" );
        fflush(stdout);

        fgets (line, 128, stdin);

        if (line[0]!= '\0')
        {
            /* remove linefeed */
            line[strlen(line)-1] = 0;
            if(strncmp("writemac", line, 8) == 0){
            	int i = 0;
            	while(line[8+i] == ' ')i++;
            	if(line[8+i])do_writemac((char *)((int)line+8+i));
            }else
            process_cmd(line, 0);
            memset(line, '\0', 128);
        }
    }

here:
    /* FIXME: Commenting this out as for some reason, the application does not exit otherwise*/
    bdt_cleanup();

    HAL_unload();

    bdt_log(":: Bluedroid test app terminating");

    return 0;
}
