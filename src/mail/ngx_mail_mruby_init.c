/*
// ngx_mail_mruby_init.c - ngx_mruby mruby init functions
//
// See Copyright Notice in ngx_http_mruby_module.c
*/

#include "ngx_mail_mruby_init.h"
#include "ngx_mail_mruby_module.h"

#include "ngx_mail_mruby_core.h"

#include <mruby.h>
#include <mruby/compile.h>

#define GC_ARENA_RESTORE mrb_gc_arena_restore(mrb, 0);

void ngx_mail_mrb_core_class_init(mrb_state *mrb, struct RClass *calss);

ngx_int_t ngx_mail_mrb_class_init(mrb_state *mrb)
{
  struct RClass *top;
  struct RClass *class;

  /* define Nginx::Mail class */
  top = mrb_define_class(mrb, "Nginx", mrb->object_class);
  class = mrb_define_class_under(mrb, top, "Mail", mrb->object_class);

  ngx_mail_mrb_core_class_init(mrb, class);
  GC_ARENA_RESTORE;

  return NGX_OK;
}
