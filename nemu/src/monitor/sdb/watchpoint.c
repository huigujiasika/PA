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

#include "sdb.h"
#define NR_WP 32

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

//head 使用中 free 空闲

void init_wp_pool() {   //初始化
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
    wp_pool[i].exp="";
    wp_pool[i].result=0;
  }

  head = NULL;
  free_ = wp_pool;
}


/* TODO: Implement the functionality of watchpoint */
void new_wp(char* exp){
  //exp此处为局部变量 不能直接赋值 会被回收 cao

  WP *findWP=free_;

  if (findWP==NULL){     
    printf("\n监视点不够\n");
    assert(0);
  }
  free_=free_->next;

  bool success;
  findWP->exp=(char *)malloc(strlen(exp)+1);
  strcpy(findWP->exp,exp);

  findWP->result=expr(exp,&success);
  
  if(head==NULL){
    head=findWP;
    head->next=NULL;
    
  }else{
    findWP->next=head;
    head=findWP;
  }

}


uint32_t gethead(){
  return head->result;
}

void printexp(){
  printf("%c\n",head->exp[0]);
}


void free_wp(WP *wp){
  WP* findWp=head;
  while(findWp->NO != wp->NO){      //可以改进 很有可能wp是一个 
    findWp=findWp->next;
  }
  if(findWp==NULL){
    printf("\n监视点NO错误\n");
    return ;
  }

  if(free_==NULL){
    free_=findWp;
    free_->next=NULL;
  }
  else{
    findWp->next=free_->next;
    free_=findWp;
    free_->exp="";
    free_->result=0;
  }
  printf("\n监视点:%d 释放成功\n",findWp->NO);

}


bool watch_changed(WP** wp){   //要返回指针
  WP* findWp=head;
  bool success=true;
  while(findWp){
    if(expr(findWp->exp,&success)){
      *wp=findWp;

      return true;
    }

    findWp=findWp->next;
  }

  return false;
}


void watch_display(){
    WP* findWp=head;

    printf("\nNum  exp       result");

    while(findWp){
      bool success=false;

      uint32_t  result=expr(findWp->exp,&success);
      findWp->result=result;

      //printf("%d\n",findWp->NO);
      printf("\n%-4d %s %x\n",findWp->NO,findWp->exp,findWp->result);

      findWp=findWp->next;

    }

}