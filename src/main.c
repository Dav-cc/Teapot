#include "http/server.h"
#include "core/log.h"

int main(void)
{
    log_message(LOG_LEVEL_INFO,
                "starting server...");

    if (init_tcp_server(8080) == -1)
    {
        log_message(LOG_LEVEL_ERROR,
                    "failed to start server");
        return 1;
    }

    return 0;
}
