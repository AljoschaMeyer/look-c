#include <stdio.h>
#include <stdlib.h>

#include "context.h"
#include "parser.h"
#include "rax.h"

AsgFile *oo_get_file(OoContext *cx, OoError *err, const char *path, size_t path_len) {
  char *src = NULL;

  void *f = raxFind(cx->files, path, path_len);
  if (f == raxNotFound) {
    FILE *f = fopen(path, "r");
    if (f == NULL) {
      goto file_err;
    }
    if (fseek(f, 0, SEEK_END)) {
      goto file_err;
    }
    long fsize = ftell(f);
    if (fsize < 0) {
      goto file_err;
    }
    rewind(f);

    src = malloc(fsize + 1);
    fread(src, fsize, 1, f);
    if (ferror(f)) {
      goto file_err;
    }
    src[fsize] = 0;

    if (fclose(f)) {
      goto file_err;
    }
    // end file handling

    AsgFile *asg = malloc(sizeof(AsgFile));
    parse_file(src, &err->parser, asg);
    if (err->parser.tag != ERR_NONE) {
      free(src);
      free(asg);
      err->tag = OO_ERR_SYNTAX;
      return NULL;
    }

    raxInsert(cx->files, path, path_len, asg, NULL);



    // TODO conditional compilation

    // TODO

    err->tag = OO_ERR_NONE;
    return asg;
  } else {
    err->tag = OO_ERR_NONE;
    return (AsgFile *) f;
  }

  file_err:
  free(src);
  err->tag = OO_ERR_FILE;
  return NULL;
}

void oo_context_free(OoContext *cx) {
  raxFree(cx->features);
  raxFree(cx->files);
}

// fwrite(path, sizeof(char), path_len, stdout);
// printf("\n");
