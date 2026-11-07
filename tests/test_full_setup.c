#define FULL_SETUP_TEST_HARNESS
#include "FullSetup.c"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#undef NDEBUG
#include <assert.h>
static uint8_t *normalData,*hiddenData;static U normalLength,hiddenLength;static int duplicate,normalCalls,hiddenCalls,mergedCalls,failSet,sets;
static H fakeHandles[2]={(H)1,(H)2};static C rootTitle[64]=u"Advanced",setTitle[64]=u"Advanced_Hidden";
static U allocMock(uint32_t t,U n,void**out){assert(t==2);*out=malloc((size_t)n);return *out?0:ERROR(9);}
static U freeMock(void*p){free(p);return 0;}
static U listMock(Database*d,uint8_t type,G*g,U*n,H*out){(void)d;assert(type==2&&!g);if(!out){*n=sizeof(fakeHandles);return ERROR(5);}memcpy(out,fakeHandles,sizeof(fakeHandles));*n=sizeof(fakeHandles);return 0;}
static U exportMock(Database*d,H h,U*n,void*out){(void)d;uint8_t*b=h==(H)2||duplicate?hiddenData:normalData;U len=h==(H)2||duplicate?hiddenLength:normalLength;if(!out){*n=len;return ERROR(5);}assert(*n>=len);memcpy(out,b,(size_t)len);*n=len;return 0;}
static Database mockdb={.list=listMock,.exportp=exportMock};
static U getMock(Strings*s,char*lang,H h,uint16_t id,C*out,U*n,void**font){(void)s;(void)font;assert(!strcmp(lang,"en-US")&&h==(H)2);C*p=id==0x1cbe?rootTitle:setTitle;U len=0;while(p[len])len++;len=(len+1)*2;if(!out||*n<len){*n=len;return ERROR(5);}memcpy(out,p,(size_t)len);*n=len;return 0;}
static U setMock(Strings*s,H h,uint16_t id,char*lang,C*text,void*font){(void)s;(void)font;assert(h==(H)2&&!strcmp(lang,"en-US"));sets++;if(failSet&&sets==2)return ERROR(7);C*p=id==0x1cbe?rootTitle:setTitle;U j=0;do{p[j]=text[j];}while(text[j++]);return 0;}
static Strings mockstrings={.get=getMock,.set=setMock};
static U locateMock(G*g,void*registration,void**out){assert(!registration&&same((uint8_t*)g,&stringGuid,16));*out=&mockstrings;return 0;}
static U originalMock(Browser*b,H*h,U n,G*g,uint16_t id,void*screen,U*action){(void)screen;assert(b->send==originalMock);
 if(setupChoice==2&&id==1&&n==1){assert(h[0]==(H)2&&same((uint8_t*)g,&advancedGuid,16));hiddenCalls++;}
 else if(setupChoice==3&&n==3){assert(h[0]==(H)9&&h[1]==(H)1&&h[2]==(H)2);assert(!g&&id==1);assert(same((uint8_t*)rootTitle,u"Advanced (Extended)",40));mergedCalls++;}
 else normalCalls++;
 if(action)*action=5;return 123;
}
static uint8_t*readfile(char*name,U*size){FILE*f=fopen(name,"rb");assert(f);fseek(f,0,SEEK_END);*size=(U)ftell(f);rewind(f);uint8_t*b=malloc((size_t)*size);assert(fread(b,1,(size_t)*size,f)==*size);fclose(f);return b;}
int main(void){
 normalData=readfile("tests/fixtures/normal.bin",&normalLength);hiddenData=readfile("tests/fixtures/hidden.bin",&hiddenLength);
 assert(target_package(normalData,normalLength)==0);assert(target_package(hiddenData,hiddenLength)==1);puts("PASS exact full hidden package, normal duplicate excluded");
 assert(target_package(hiddenData,hiddenLength-1)==-1);puts("PASS truncated input rejected");
 database=&mockdb;pool=allocMock;freepool=freeMock;assert(choose_handle()==(H)2);duplicate=1;assert(!choose_handle());duplicate=0;puts("PASS unique live handle required");
 for(int i=1;i<=3;i++)assert(key_choice((Key){0,(uint16_t)('0'+i)})==i);assert(key_choice((Key){0x17,0})==-1);puts("PASS all menu choices and cancel");
 setupChoice=1;assert(prepare_browser(0)==1);puts("PASS normal mode does not touch browser interface");
 ST st={0};BS bs={0};st.bs=&bs;bs.v[37]=(void*)locateMock;systemTable=&st;Browser br={.send=redirected};browser=&br;original=originalMock;U action=0;H ordinary[]={(H)9,(H)1};
 setupChoice=2;assert(redirected(&br,ordinary,2,0,1,0,&action)==123);assert(hiddenCalls==1&&br.send==originalMock&&action==5);puts("PASS hidden root dispatch, restoration and status/action propagation");
 setupChoice=3;br.send=redirected;assert(redirected(&br,ordinary,2,0,1,0,&action)==123);assert(mergedCalls==1&&br.send==originalMock&&action==5);assert(same((uint8_t*)rootTitle,u"Advanced",18));assert(same((uint8_t*)setTitle,u"Advanced_Hidden",32));puts("PASS combined preserves original handles/order/filter and appends hidden; titles restored");
 sets=0;failSet=1;br.send=redirected;assert(redirected(&br,ordinary,2,0,1,0,&action)==123);assert(normalCalls==1&&same((uint8_t*)rootTitle,u"Advanced",18));failSet=0;puts("PASS partial rename failure rolls back and uses normal menu");
 duplicate=1;br.send=redirected;assert(redirected(&br,ordinary,2,0,1,0,&action)==123);assert(normalCalls==2&&br.send==originalMock);puts("PASS ambiguous match preserves normal call");
 free(normalData);free(hiddenData);return 0;
}
