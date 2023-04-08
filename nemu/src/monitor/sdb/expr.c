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

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
#include <memory/paddr.h>


enum {
  TK_NOTYPE = 256, TK_EQ, TK_DEC, TK_HEX, TK_AND, TK_NEQ, TK_REG, TK_DEREF, TK_NEG
};

static struct rule {  //正则规则
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
  {"\\+", '+'},         // plus
  {"==", TK_EQ},        // equal
  {"!=",TK_NEQ},
  {"&&",TK_AND},
  {"-",'-'},
  {"\\*",'*'},
  {"/",'/'},
  {"\\(",'('},
  {"\\)",')'},
  {"[0-9]+", TK_DEC}, 
  {"0x[A-Fa-f0-9]+",TK_HEX},
  {"\\$[A-Za-z0-9]+",TK_REG},
};

#define NR_REGEX ARRLEN(rules) //define ARRLEN(arr) (int)(sizeof(arr) / sizeof(arr[0]))
//应该是总共几个规则

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[128];
} Token;

static Token tokens[64] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

// void print(){
//     int i=0;
//     for(int i=0;i<10;i++){
//       printf("%s\n",tokens[i].str);
//     }
//   }

static bool make_token(char *e) {  //制造token
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        //当前地方找到符合规则
        /*
        regexec 函数是 POSIX 正则表达式函数库的一部分，用于在字符串中搜索与指定的正则表达式匹配的子串。

        其第一个参数是一个已编译的正则表达式模式（&re[i]）；
        第二个参数是要在其中搜索匹配子串的字符串（e + position）；
        第三个参数是指定匹配的最大次数（1）；
        第四个参数是一个指向存储匹配信息的结构体的指针（&pmatch）；
        最后一个参数是传递给函数的选项（0）。   

        如果成功，函数会返回 0，否则返回一个非零值。在该代码中，如果 regexec 返回 0，
        代表在该位置找到了与某一个正则表达式匹配的子串，并且这个子串的开始位置是 0（即它开始的位置就是当前的 position）。
        */
       
       /*pmatch.rm_so 是 regexec 函数的返回值中的一个字段，表示匹配的字符串在原始字符串（e + position）中的起始位置。

        如果 pmatch.rm_so 的值为 0，这意味着在当前的位置（即 position）找到了与正则表达式匹配的字符串，并且这个字符串是从该位置开始的。

        因此， pmatch.rm_so == 0 的判断是用来确定是否找到了在当前位置开始的与正则表达式匹配的子串。
          
      * /*/
        //char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        // Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
        //     i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */


        assert(substr_len < 64);  //防止超过限制
        assert(nr_token < 64);

        tokens[nr_token].type=rules[i].token_type;   
        memcpy(tokens[nr_token].str,e+position,sizeof(char)*substr_len);
        tokens[nr_token].str[substr_len]='\0';
        
        position+=substr_len;

        switch (rules[i].token_type) {
          case TK_NOTYPE:
            nr_token--;
            break;

          //不懂
          case '-':
           if (nr_token == 0 || (tokens[nr_token-1].type != ')' && tokens[nr_token-1].type != TK_DEC 
              && tokens[nr_token-1].type != TK_HEX && tokens[nr_token-1].type != TK_REG)) 
              tokens[nr_token].type = TK_NEG;
            break;  //不懂

        }

        nr_token++;
        break;
      }
    }

    if (i == NR_REGEX) {  //找完了
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }
  //print();
  return true;
}

int checkParentheses(int start,int end){
  int par_level=0;
  Assert(start<=end,"start大于end");

  for(int i=start;i<=end;i++){
    if(tokens[i].type=="(")
      par_level++;
    else if(tokens[i].type==")"){
      par_level--;
      Assert(par_level>=0,"括号不匹配");
      if(par_level==0&&i!=end){            //式子开始一定是由括号包裹
        return false;
      }
    }
  }
  assert(par_level==0);
  if (tokens[start].type != '(' || tokens[end].type != ')') 
    return false;
  
  return true;

}


int findMainOp(int start,int end){

  int par_level=0,priority=120,main_operator=-1;

  for(int i=start;i<=end;i++){
    if (tokens[i].type == '(') {
      par_level++;
    } else if (tokens[i].type == ')') {
      par_level--;
    }
    if (par_level) continue;

    if ((tokens[i].type == TK_DEREF || tokens[i].type == TK_NEG) && priority > 100) {
      main_operator = i;
      priority = 100;
    } else if ((tokens[i].type == '*' || tokens[i].type == '/') && priority >= 80) {
      main_operator = i;
      priority = 80;
    } else if ((tokens[i].type == '+' || tokens[i].type == '-') && priority >= 50) {
      main_operator = i;
      priority = 50;
    } else if (tokens[i].type == TK_AND && priority >= 30) {
      main_operator = i;
      priority = 30;
    } else if ((tokens[i].type == TK_EQ || tokens[i].type == TK_NEQ) && priority >= 20) {
      main_operator = i;
      priority = 20;
    }
  }

  //print();
  assert(main_operator != -1);
  return main_operator;

}



// word_t realOp(word_t val1,word_t val2,int type){
//   switch (type){
  
//     case '+':
//         return val1 + val2;
//     case '-':
//       return val1 - val2;
//     case '*':
//       return val1 * val2;
//     case '/': 
//       if (val2 == 0) {
//         printf("Invalid Expression(div0 error)\n");
//         return false;
//       }
//       return val1 / val2;
//     case TK_AND:
//       return val1 && val2;
//     case TK_EQ:
//       return val1 == val2;
//     case TK_NEQ:
//       return val1 != val2;
//     default:
//       printf("Invalid Expression\n");
//       return false;
//   }
// }



// word_t evalExp(int start,int end){
//   if(start>end)
//     return 0;
//   else if(start==end){
//     word_t val;
//     if(tokens[start].type==TK_DEC)
//       sscanf(tokens[start].str,"%u",&val);
//     else if(tokens[start].type==TK_HEX)
//       sscanf(tokens[start].str+2,"%x",&val);
//     else if (tokens[start].type==TK_REG)
//     {
//       bool success=false;
//       val=isa_reg_str2val(tokens[start].str+1,&success);
//       if(!success){
//         printf("Invaild Expression(reg)\n");
//         return false;
//       }
      
//     }
//     return val;
//   }else if(checkParantheses(start,end)==true){     //被括号包裹且符合要求
//     return evalExp(start+1,end-1);
//   }else{
//     int main_operator=findmainOp(start,end);

//     word_t val2 = evalExp(main_operator + 1, end);  //不懂
//     switch (tokens[main_operator].type) {
//       case TK_DEREF:
//         return paddr_read(val2, 1);
//       case TK_NEG:
//         return -val2;
//     }

//     word_t val1 = evalExp(start, main_operator - 1);

//     return realOp(val1,val2,tokens[main_operator].type);

//   }

// }

word_t evalExp(int start, int end) {
  if (start > end) {
    return 0;
  } else if (start == end) {
    word_t val;
    if (tokens[start].type == TK_DEC) {
      sscanf(tokens[start].str, "%u", &val);
    } else if (tokens[start].type == TK_HEX) {
      sscanf(tokens[start].str + 2, "%x", &val);
    } else if (tokens[start].type == TK_REG) {
      bool success = false;
      val = isa_reg_str2val(tokens[start].str + 1, &success);
      if (!success) {
        printf("Invalid Expression\n");
        return false;
      }
    }
    return val;
  } else if (checkParentheses(start, end) == true) {
    return evalExp(start + 1, end - 1);
  } else {
    int main_operator = findMainOp(start, end);
    
    word_t val2 = evalExp(main_operator + 1, end);
    switch (tokens[main_operator].type) {
      case TK_DEREF:
        return paddr_read(val2, 1);
      case TK_NEG:
        return -val2;
    }

    word_t val1 = evalExp(start, main_operator - 1);
    switch (tokens[main_operator].type) {
      case '+':
        return val1 + val2;
      case '-':
        return val1 - val2;
      case '*':
        return val1 * val2;
      case '/': 
        if (val2 == 0) {
          printf("Invalid Expression\n");
          return false;
        }
        return val1 / val2;
      case TK_AND:
        return val1 && val2;
      case TK_EQ:
        return val1 == val2;
      case TK_NEQ:
        return val1 != val2;
      default:
        printf("Invalid Expression\n");
        return false;
    }
  }
}









word_t expr(char *e, bool *success) {

  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  for (int i = 0; i < nr_token; i ++) {
    if (tokens[i].type == '*' && (i == 0 || 
        (tokens[i-1].type != TK_DEC 
        && tokens[i-1].type != TK_HEX 
        && tokens[i-1].type != TK_REG
        && tokens[i-1].type != ')'  )    )
        ) 
      tokens[i].type = TK_DEREF;
    
  }
  *success=true;


  return evalExp(0,nr_token-1);
}
