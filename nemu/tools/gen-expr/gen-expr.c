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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>

// this should be enough
static char buf[65536] = {};
static char code_buf[65536 + 128] = {}; // a little larger than `buf`
static char *code_format =
"#include <stdio.h>\n"
"int main() { "
"  unsigned result = %s; "
"  printf(\"%%u\", result); "
"  return 0; "
"}";

int tail;
int choose(int x){
  return rand()%x;
}

static inline void gen_num(){       //内联函数
  buf[tail++]=choose(10)+'0';
  if(buf[tail-1]=='0') 
    return ;
  switch (choose(5)) {           //不懂
    case 0: buf[tail++] = choose(10) + '0';
    case 1: buf[tail++] = choose(10) + '0';
    case 2: buf[tail++] = choose(10) + '0';
    case 3: buf[tail++] = choose(10) + '0';break;
    
  }


}

static inline void gen(char ch){
  buf[tail++]=ch;
}

static inline void gen_rand_op(){
  char op;
  switch (choose(4)) {
    case 0: op = '+'; break;
    case 1: op = '-'; break;
    case 2: op = '*'; break;
    case 3: op = '/'; break;
  }
  buf[tail++] = op;
}


static inline void gen_rand_expr() {
  if(tail>=60000) return;
  switch (choose(3))
  {
  case 0:
    gen_num();
    break;
  case 1:
    gen('(');
    gen_rand_expr();
    gen(')');
    break;
  case 2:
    gen_rand_expr();
    gen_rand_op();
    gen_rand_expr();
    break;
  }

  buf[tail] = '\0';
}

int main(int argc, char *argv[]) {
  int seed = time(0);
  srand(seed);
  int loop = 1;
  if (argc > 1) {
    sscanf(argv[1], "%d", &loop);
  }
  int i;
  for (i = 0; i < loop; i ++) {
    tail=0;
    gen_rand_expr();

    sprintf(code_buf, code_format, buf);
    //printf(buf);

    FILE *fp = fopen("/tmp/.code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    fclose(fp);

    int ret = system("gcc /tmp/.code.c -Wall -Werror -o /tmp/.expr 2>/dev/null");  
                               //通过将错误重定向即丢弃
    if (ret != 0) continue;

    fp = popen("/tmp/.expr", "r");
    assert(fp != NULL);

    int result;
    fscanf(fp, "%d", &result);
    pclose(fp);

    printf("%u %s\n",result, buf);
  }
  return 0;
}
