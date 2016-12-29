/*
// ngx_mail_mruby_module.h - ngx_mruby mruby module header
//
// See Copyright Notice in ngx_http_mruby_module.c
*/

#ifndef NGX_MAIL_MRUBY_MODULE_H
#define NGX_MAIL_MRUBY_MODULE_H

#include <nginx.h>
#include <ngx_core.h>
#include <ngx_mail.h>

#define MODULE_NAME "ngx_mruby-mail-module"

extern ngx_module_t ngx_mail_mruby_module;

typedef struct {
  ngx_mail_session_t *s;
  ngx_int_t mail_status;
} ngx_mail_mruby_internal_ctx_t;

#endif // NGX_MAIL_MRUBY_MODULE_H
