#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "os_interfaces.h"
#include "ucp.h"
#include "handle_message.h"

void* ucp_server_rcv_thread(void *arg)
{
    int ret;
    ucp_session_type_t type;
    static uint8_t ucp_rcv_buf[1500];
    printf("ucp_server_rcv_thread in\n");

    while (1) {
        ret = ucp_recv(ucp_rcv_buf, sizeof(ucp_rcv_buf), &type);
        if (ret > 0) {
            //printf("ucp client recv type:%d, len:%d\n", type, ret);
            ucp_rcv_buf[ret] = '\0';
            handle_message((char *)ucp_rcv_buf);
        } else {
            printf("ucp_recv return error:%d\n", ret);
        }
    }

    printf("ucp_server_rcv_thread out\n");

    return NULL;
}


int my_ucp_init()
{
    int ret;
    os_thread_t rcv_thread;

    ret = ucp_init(UCP_ROLE_SERVER);
    if (0 != ret) {
        printf("ucp_init server fail\n");
        return -1;
    }

    ret = os_create_thread(&rcv_thread, ucp_server_rcv_thread, NULL);
    if (0 != ret) {
        printf("creat ucp server rcv thread fail\n");
        return -1;
    }

    return 0;
}

