/*
// ngx_mail_mruby_module.c - ngx_mruby mruby module header
//
// See Copyright Notice in ngx_http_mruby_module.c
*/

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_mail.h>

#include "mruby.h"
#include "mruby/array.h"
#include "mruby/compile.h"
#include "mruby/data.h"
#include "mruby/proc.h"
#include "mruby/string.h"
#include "mruby/variable.h"

#include "ngx_mail_mruby_init.h"
#include "ngx_mail_mruby_module.h"

typedef enum code_type_t { NGX_MRB_CODE_TYPE_FILE, NGX_MRB_CODE_TYPE_STRING } code_type_t;

typedef struct ngx_mrb_code_t {
  union code {
    char *file;
    char *string;
  } code;
  code_type_t code_type;
  struct RProc *proc;
  mrbc_context *ctx;
} ngx_mrb_code_t;

typedef struct {
  mrb_state *mrb;
  ngx_mrb_code_t *code;
} ngx_mail_mruby_srv_conf_t;

#define NGX_MRUBY_CODE_MRBC_CONTEXT_FREE(mrb, code)                                                                    \
  if (code != NGX_CONF_UNSET_PTR && mrb && (code)->ctx) {                                                              \
    mrbc_context_free(mrb, (code)->ctx);                                                                               \
    (code)->ctx = NULL;                                                                                                \
  }

/* mail session mruby directive functions */
static char *ngx_mail_mruby_build_code(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

/* mruby core functiosn for compile*/
static ngx_mrb_code_t *ngx_mail_mruby_mrb_code_from_string(ngx_pool_t *pool, ngx_str_t *code_s);
static ngx_int_t ngx_mail_mruby_shared_state_compile(ngx_conf_t *cf, mrb_state *mrb, ngx_mrb_code_t *code);

/* setup main and srv configuration */
static void *ngx_mail_mruby_create_srv_conf(ngx_conf_t *cf);
static char *ngx_mail_mruby_merge_srv_conf(ngx_conf_t *cf, void *parent, void *child);

static ngx_command_t ngx_mail_mruby_commands[] = {

    {ngx_string("mruby_mail_code"), NGX_MAIL_MAIN_CONF | NGX_MAIL_SRV_CONF | NGX_CONF_TAKE1, ngx_mail_mruby_build_code,
     NGX_MAIL_SRV_CONF_OFFSET, 0, NULL},

    ngx_null_command};

static ngx_mail_module_t ngx_mail_mruby_module_ctx = {
    NULL, /* protocol */

    NULL, /* create main configuration */
    NULL, /* init main configuration */

    ngx_mail_mruby_create_srv_conf, /* create server configuration */
    ngx_mail_mruby_merge_srv_conf   /* merge server configuration */
};

ngx_module_t ngx_mail_mruby_module = {NGX_MODULE_V1,
                                      &ngx_mail_mruby_module_ctx, /* module context */
                                      ngx_mail_mruby_commands,    /* module directives */
                                      NGX_MAIL_MODULE,            /* module type */
                                      NULL,                       /* init master */
                                      NULL,                       /* init module */
                                      NULL,                       /* init process */
                                      NULL,                       /* init thread */
                                      NULL,                       /* exit thread */
                                      NULL,                       /* exit process */
                                      NULL,                       /* exit master */
                                      NGX_MODULE_V1_PADDING};

/* create directive template */
static void *ngx_mail_mruby_create_srv_conf(ngx_conf_t *cf)
{
  ngx_mail_mruby_srv_conf_t *mscf = ngx_pcalloc(cf->pool, sizeof(ngx_mail_mruby_srv_conf_t));
  if (mscf == NULL)
    return NULL;

  mscf->code = NGX_CONF_UNSET_PTR;
  mscf->mrb = mrb_open();
  ngx_mail_mrb_class_init(mscf->mrb);

  return mscf;
}

/* merge directive configuration */
static char *ngx_mail_mruby_merge_srv_conf(ngx_conf_t *cf, void *parent, void *child)
{
  ngx_mail_mruby_srv_conf_t *prev = parent;
  ngx_mail_mruby_srv_conf_t *conf = child;

  if (conf->mrb == NULL)
    conf->mrb = prev->mrb;

  if (conf->code == NGX_CONF_UNSET_PTR)
    conf->code = prev->code;

  return NGX_CONF_OK;
}

/* ngx_mruby mail core functions */
static ngx_mrb_code_t *ngx_mail_mruby_mrb_code_from_string(ngx_pool_t *pool, ngx_str_t *code_s)
{
  ngx_mrb_code_t *code;

  code = ngx_pcalloc(pool, sizeof(*code));
  if (code == NULL)
    return NGX_CONF_UNSET_PTR;

  code->code.string = ngx_palloc(pool, code_s->len + 1);
  if (code->code.string == NULL)
    return NGX_CONF_UNSET_PTR;

  ngx_cpystrn((u_char *)code->code.string, code_s->data, code_s->len + 1);
  code->code_type = NGX_MRB_CODE_TYPE_STRING;

  return code;
}

static ngx_int_t ngx_mail_mruby_shared_state_compile(ngx_conf_t *cf, mrb_state *mrb, ngx_mrb_code_t *code)
{
  FILE *mrb_file;
  struct mrb_parser_state *p;

  if (code->code_type == NGX_MRB_CODE_TYPE_FILE) {
    if ((mrb_file = fopen((char *)code->code.file, "r")) == NULL)
      return NGX_ERROR;

    code->ctx = mrbc_context_new(mrb);
    mrbc_filename(mrb, code->ctx, (char *)code->code.file);
    p = mrb_parse_file(mrb, mrb_file, code->ctx);
    fclose(mrb_file);
  } else {
    code->ctx = mrbc_context_new(mrb);
    mrbc_filename(mrb, code->ctx, "INLINE CODE");
    p = mrb_parse_string(mrb, (char *)code->code.string, code->ctx);
  }

  if (p == NULL || (0 < p->nerr))
    return NGX_ERROR;

  code->proc = mrb_generate_code(mrb, p);
  mrb_pool_close(p->pool);
  if (code->proc == NULL)
    return NGX_ERROR;

  if (code->code_type == NGX_MRB_CODE_TYPE_FILE) {
    ngx_conf_log_error(NGX_LOG_NOTICE, cf, 0, "%s NOTICE %s:%d: compile info: code->code.file=(%s)", MODULE_NAME,
                       __func__, __LINE__, code->code.file);
  } else {
    ngx_conf_log_error(NGX_LOG_NOTICE, cf, 0, "%s NOTICE %s:%d: compile info: "
                                              "code->code.string=(%s)",
                       MODULE_NAME, __func__, __LINE__, code->code.string);
  }

  return NGX_OK;
}

/* set directive values from inline code*/
static char *ngx_mail_mruby_build_code(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
  ngx_mail_mruby_srv_conf_t *mscf = conf;
  ngx_str_t *value;
  ngx_mrb_code_t *code;
  ngx_int_t rc;

  value = cf->args->elts;
  code = ngx_mail_mruby_mrb_code_from_string(cf->pool, &value[1]);

  if (code == NGX_CONF_UNSET_PTR)
    return NGX_CONF_ERROR;

  rc = ngx_mail_mruby_shared_state_compile(cf, mscf->mrb, code);

  mscf->code = code;

  if (rc != NGX_OK) {
    ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "mrb_string(%s) load failed", value[1].data);
    return NGX_CONF_ERROR;
  }

  return NGX_CONF_OK;
}
