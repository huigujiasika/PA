/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#ifndef __SDB_H__
#define __SDB_H__

#include <common.h>

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  /* TODO: Add more members if necessary */
  uint32_t result;
  char* exp;

} WP;

word_t expr(char *e, bool *success);

static int cmd_help(char *args);    //ok
static int cmd_x(char *args);       //ok   但是目前只能扫描特定的
static int cmd_info(char *args);    //ok
static int cmd_si(char *args);      //ok
static int cmd_qw(char *args);
static int cmd_p(char *args);       
static int cmd_c(char *args);       //ok
static int cmd_q(char *args);       //ok
static int cmd_w(char *args);

void init_wp_pool();
void new_wp(char* exp);
void free_wp(WP *wp);
bool watch_changed(WP** wp);
void watch_display();
uint32_t gethead();


#endif
