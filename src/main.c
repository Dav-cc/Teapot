#include "http/server.h"
#include "core/log.h"
#include "config.h"

int main(void)
{
    log_message(LOG_LEVEL_INFO,
                "starting server...");
    server_config* sc = config_load("server.conf");
    if(sc == NULL){
        log_message(LOG_LEVEL_ERROR, " Fail to start server, server config couldn't load");
        return 1;
    }
    g_log_level = sc->log_level;

    if (init_tcp_server(sc) == -1)
    {
        log_message(LOG_LEVEL_ERROR,
                    "failed to start server");
        return 1;
    }

    return 0;
}
