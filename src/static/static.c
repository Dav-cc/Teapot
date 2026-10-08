#include "static.h"
#include "../parser/http_parser.h"
#include "../http/router.h"
#include "../core/log.h"
#include "../core/dbuff.h"
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>

mime_type_table mime_table[] ={
        {".html", "text/html"},
        {".png", "image/png"},
        {".gif", "image/gif"},
        {".pdf", "application/pdf"},
        {".json", "application/json"},
        {".mp4", "video/mp4"},
};

static int mime_table_entry_count = sizeof(mime_table)/sizeof(mime_table[1]);

char *path_verifyer(http_request * req) {
  char *filepath;
  size_t index = 0;
  size_t path_counter = 0;
  size_t top = 0;
  size_t offset = 0;

  const char *root = "./public";
  size_t root_len = strlen(root);

  http_slice *path_stack = calloc(16, sizeof(*path_stack));
  if (!path_stack) {
    log_message(LOG_LEVEL_ERROR, "FAILED to init memory");
    return NULL;
  }

  filepath = calloc(1, 128);
  if (!path_stack) {
    log_message(LOG_LEVEL_ERROR, "FAILED to init memory");
    return NULL;
  }

  while (index < req->path.len) {

    if (req->path.ptr[index] == '/') {
      index++;
      continue;
    }

    if (path_counter >= 16) {
      free(path_stack);
      return NULL;
    }

    size_t start = index;

    while (index < req->path.len && req->path.ptr[index] != '/') {
      index++;
    }

    path_stack[path_counter].ptr = req->path.ptr + start;
    path_stack[path_counter].len = index - start;

    path_counter++;
  }

  for (size_t i = 0; i < path_counter; i++) {

    http_slice current = path_stack[i];

    if (current.len == 1 && current.ptr[0] == '.') {
      continue;
    }

    if (current.len == 2 && current.ptr[0] == '.' && current.ptr[1] == '.') {

      if (top == 0) {
        free(path_stack);
        return NULL;
      }

      top--;
      continue;
    }

    path_stack[top] = current;
    top++;
  }

  memcpy(filepath + offset, root, root_len);
  offset += root_len;

  for (size_t i = 0; i < top; i++) {

    filepath[offset++] = '/';
    memcpy(filepath + offset, path_stack[i].ptr, path_stack[i].len);
    offset += path_stack[i].len;
  }

  filepath[offset] = '\0';
  return filepath;
}

file_state* get_file_state(int filefd){
    struct stat st;
    file_state *fst = calloc(1, sizeof(file_state));
    int res = fstat(filefd, &st);
    if(res == -1){
        log_message(LOG_LEVEL_ERROR, "Error in function stat");
        return NULL;
    }

    fst->file_fd = filefd;
    fst->file_size = st.st_size;
    return fst;
}

file_state* find_mime_type(file_state* fs,const char* path){
    char* dot_type = strstr(path, "."); // we should make sure that every "path" reaches to here is normalized(whitout "." or "..")
    for(int i = 0; i< mime_table_entry_count; i++){
        if(strcmp(mime_table[i].mime, dot_type)){
            fs->mime_type = mime_table[i].mime;
            return fs;
        }
    }
    fs->mime_type = "application/octet-stream";
    return fs;
}

file_state* set_body_response(file_state* fs){
    fs->write_buf = dbuff_create(fs->file_size);
    io_err res = dbuff_read(fs->write_buf, fs->file_fd);
    switch(res){
        case IO_DONE:
        case IO_CLOSED: 
            break;

        case IO_ERROR:
        case IO_AGAIN:
        
            dbuff_destroy(fs->write_buf);
            return fs;
    }
    return fs;
}
