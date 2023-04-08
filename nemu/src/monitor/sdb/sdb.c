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

#include <isa.h>
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "sdb.h"


// static : only in this file can use its function


// ? 
static int is_batch_mode = false;


void init_regex();
void init_wp_pool();

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {

  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}


static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display information about all supported commands", cmd_help },  //ok
  { "c", "Continue the execution of the program", cmd_c },  
  { "q", "Exit NEMU", cmd_q },   //ok
  {"si","run some step",cmd_si},  //ok
  {"info","some info status",cmd_info},
  {"x","search memory",cmd_x},
  {"qw","1",cmd_qw},        //简易表达式求值
  {"p","expression",cmd_p},   //ok 
  {"w","watch point",cmd_w},

  /* TODO: Add more commands */

};

#define NR_CMD ARRLEN(cmd_table)          // 命令条数
// define ARRLEN(arr) (int)(sizeof(arr) / sizeof(arr[0]))


static int cmd_w(char *args){
  char *exp=args;
  new_wp(exp);
  
  return 0;
}


static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}



static int cmd_q(char *args) {
  return -1;
}



static int cmd_p(char *args){

  char *exp=args;

  bool success=false;
  word_t val=expr(exp,&success);

  if(!success)
    printf("Invalid Expression\n");
  else 
    printf("%u (0x%x)\n",val,val);

  return 0;
}

static int cmd_help(char *args) {   
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {      //全部
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {                //单指
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}


static int cmd_si(char *args){
  int num=1;
  if(args!=NULL)
    sscanf(args,"%d",&num);
  cpu_exec(num);            //单步执行num次

  return 0;
}

static int cmd_x(char *args){  //需要调整
  
  char*n =strtok(NULL," ");             
  //On the first call to strtok(), the string to be parsed should be specified in str.
  //之后不需要再指定
  char* exp=strtok(NULL," ");
  
  int num;
  uint32_t addr;   //无符号 16进制

  sscanf(n,"%d",&num);
  
  bool success=false;
  //首先规定只能是16进制数
  sscanf(exp,"%x",&addr);   

  int i=0;
  while(num--){
    printf(" %02x",paddr_read( addr+(i++) ,1 )  );
  }
  putchar('\n');
  
  return 0;

}


// TODO::   info w 打印监视点信息
static int cmd_info(char *args){
  char *subcmd=strtok(NULL," ");
  
  switch (subcmd[0])
  {
  case 'r':
    isa_reg_display();
    break;

  case 'w':
    printf("\n\n\n%x",gethead());
    watch_display();
    break;

  default:
    printf("Unsupported subcommand: %s\n", subcmd);
  }

  return 0;
}


static int cmd_qw(char *args){
  char *exp = args;
  bool success=false;
  expr(exp,&success);
}


void sdb_set_batch_mode() {  // 不明白
  is_batch_mode = true;
}



void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }               // 解析调用子命令
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}



void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}
