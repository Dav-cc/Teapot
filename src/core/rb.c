#include "rb.h"
#include "log.h"
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>

buffer_t* db_create(size_t size){
    buffer_t* db = calloc(1, sizeof(buffer_t));
    if(!db){
        log_message(LOG_LEVEL_ERROR, "error in alocation for rb: %s", strerror(errno));
        return NULL;
    }

    db->data = calloc(size, sizeof(char));
    if(!db->data){
        free(db);
        log_message(LOG_LEVEL_ERROR, "error in alocation db->data: %s", strerror(errno));
        return NULL;
    }
        db->cap = size;
        db->len = 0;
        db->offset = 0;
        return db;
}

ssize_t db_socket_read(buffer_t *db, int fd){
    if(db->cap - db->len < 4096) {
        size_t new_cap = db->cap ? db->cap * 2 : 4096;
        char* tmp = realloc(db->data, new_cap); 
        if(!tmp){
            log_message(LOG_LEVEL_ERROR, " can not realloc : %s ", strerror(errno));
            return -1;
        }
        db->data = tmp;
        db->cap = new_cap;
    }
    ssize_t readed = recv(fd, db->data + db->len, db->cap - db->len, 0);
    if(readed > 0){
        db->len += readed;
    }
    return readed;
}
ssize_t db_socket_write(buffer_t *db, int fd){
    if(db->offset >= db->len){
            return 0;
    }
    ssize_t n = send(fd, db->data + db->offset, db->len - db->offset, 0);
    if (n > 0){
      db->offset += n;
    }
    return n;
}

void db_destroy(buffer_t *db){
    if (!db)
        return;
    free(db->data);
    free(db);
}
