/*
// ngx_mail_mruby_init.h - ngx_mruby mruby init header
//
// See Copyright Notice in ngx_http_mruby_module.c
*/

#ifndef NGX_MAIL_MRUBY_INIT_H
#define NGX_MAIL_MRUBY_INIT_H

#include "ngx_mail_mruby_core.h"
#include <mruby.h>
#include <ngx_mail.h>

ngx_int_t ngx_mail_mrb_class_init(mrb_state *mrb);

#endif // NGX_MAIL_MRUBY_INIT_H
