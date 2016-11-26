/*******************************************************************************
 ** Load stack lib
 *******************************************************************************/
#include <hardware/hardware.h>
#include <hardware/bluetooth.h>
#define pr_info(fmt, args...)  printf("[board_test]: "fmt, ##args)
/* Main API */
static bluetooth_device_t* bt_device;

const bt_interface_t* sBtInterface = NULL;
/* Set to 1 when the Bluedroid stack is enabled */
static unsigned char bt_enabled = 0;
static bt_status_t status;
//extern int hw_get_module(const char *id, const struct hw_module_t **module);

static void adapter_state_changed(bt_state_t state)
{
    pr_info("ADAPTER STATE UPDATED : %s \n", (state == BT_STATE_OFF)?"OFF":"ON");
    if (state == BT_STATE_ON) {
        bt_enabled = 1;
    } else {
        bt_enabled = 0;
    }
}

static bt_callbacks_t bt_callbacks = {
    sizeof(bt_callbacks_t),
    adapter_state_changed,
    NULL, /*adapter_properties_cb */
    NULL, /* remote_device_properties_cb */
    NULL, /* device_found_cb */
    NULL, /* discovery_state_changed_cb */
    NULL, /* pin_request_cb  */
    NULL, /* ssp_request_cb  */
    NULL, /*bond_state_changed_cb */
    NULL, /* acl_state_changed_cb */
    NULL, /* thread_evt_cb */
    NULL, /*dut_mode_recv_cb */
};

int HAL_load(void)
{
    int err = 0;

    hw_module_t* module;
    hw_device_t* device;

    pr_info("Loading HAL lib + extensions\n");

    err = hw_get_module(BT_HARDWARE_MODULE_ID, (hw_module_t const**)&module);
    if (err == 0)
    {
        err = module->methods->open(module, BT_HARDWARE_MODULE_ID, &device);
        if (err == 0) {
            bt_device = (bluetooth_device_t *)device;
            sBtInterface = bt_device->get_bluetooth_interface();
        }
    }

    pr_info("HAL library loaded (%s)\n", strerror(err));

    return err;
}

int HAL_unload(void)
{
    int err = 0;

    pr_info("Unloading HAL lib\n");

    sBtInterface = NULL;

    pr_info("HAL library unloaded (%s)\n", strerror(err));

    return err;
}

void check_bt_return_status(bt_status_t status)
{
    if (status != BT_STATUS_SUCCESS)
    {
        pr_info("HAL REQUEST FAILED status : %d\n", status);
    }
    else
    {
        pr_info("HAL REQUEST SUCCESS\n");
    }
}

void bt_init(void)
{
    pr_info("INIT BT\n");
    status = sBtInterface->init(&bt_callbacks);
    check_bt_return_status(status);
}

int bt_enable(void)
{
    pr_info("enable BT\n");
    bt_init();
    if (bt_enabled) {
        pr_info("Bluetooth is already enabled\n ");
        return -1;
    }
    status = sBtInterface->enable();
    sleep(1);
    check_bt_return_status(status);
    return 0;
}

void bt_disable(void)
{
    pr_info("DISABLE BT\n");
    if (!bt_enabled) {
        pr_info("Bluetooth is already disabled\n");
        return;
    }
    status = sBtInterface->disable();
    check_bt_return_status(status);
    bt_enabled = 0;
}
