#define REBAR_TEST_HARNESS
#include "RebarPage.c"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#undef NDEBUG
#include <assert.h>
static uint8_t*normalData,*hiddenData;static U normalLength,hiddenLength;static H fakeHandles[2]={(H)1,(H)2};static int duplicate,normalCalls,redirectCalls;
static U allocMock(uint32_t t,U n,void**out){assert(t==2);*out=malloc((size_t)n);return *out?0:ERROR(9);}
static U freeMock(void*p){free(p);return 0;}
static U listMock(Database*d,uint8_t type,G*g,U*n,H*out){(void)d;assert(type==2&&!g);if(!out){*n=sizeof(fakeHandles);return ERROR(5);}memcpy(out,fakeHandles,sizeof(fakeHandles));*n=sizeof(fakeHandles);return 0;}
static U exportMock(Database*d,H h,U*n,void*out){(void)d;uint8_t*b=h==(H)1||duplicate?normalData:hiddenData;U len=h==(H)1||duplicate?normalLength:hiddenLength;if(!out){*n=len;return ERROR(5);}assert(*n>=len);memcpy(out,b,(size_t)len);*n=len;return 0;}
static Database mockdb={.list=listMock,.exportp=exportMock};
static U originalMock(Browser*b,H*h,U n,G*g,uint16_t id,void*screen,U*action){(void)screen;assert(b->send==originalMock);if(id==0x159a){assert(n==1&&h[0]==(H)1&&same((uint8_t*)g,&advancedGuid,16));redirectCalls++;}else normalCalls++;if(action)*action=5;return 123;}
static uint8_t*readfile(char*name,U*size){FILE*f=fopen(name,"rb");assert(f);fseek(f,0,SEEK_END);*size=(U)ftell(f);rewind(f);uint8_t*b=malloc((size_t)*size);assert(fread(b,1,(size_t)*size,f)==*size);fclose(f);return b;}
int main(void){
 normalData=readfile("tests/fixtures/normal.bin",&normalLength);hiddenData=readfile("tests/fixtures/hidden.bin",&hiddenLength);
 assert(target_package(normalData,normalLength)==1);assert(target_package(hiddenData,hiddenLength)==0);puts("PASS selects synthetic normal Advanced, excludes hidden duplicate GUID");
 assert(target_package(normalData,normalLength-1)==-1);puts("PASS truncated package rejected");
 uint8_t*copy=malloc((size_t)normalLength);memcpy(copy,normalData,(size_t)normalLength);copy[16]=0;assert(target_package(copy,normalLength)==-1);free(copy);puts("PASS malformed package rejected");
 database=&mockdb;pool=allocMock;freepool=freeMock;assert(choose_handle()==(H)1);duplicate=1;assert(!choose_handle());duplicate=0;puts("PASS unique live handle required");
 ST st={0};systemTable=&st;Browser br={.send=redirected};browser=&br;original=originalMock;U action=0;assert(redirected(&br,0,0,0,0,0,&action)==123);assert(redirectCalls==1&&br.send==originalMock&&action==5);puts("PASS original restored before selected-page call, status/action preserved");
 duplicate=1;br.send=redirected;assert(redirected(&br,0,0,0,0,0,&action)==123);assert(normalCalls==1&&br.send==originalMock);puts("PASS ambiguous selection leaves original call unchanged");
 free(normalData);free(hiddenData);return 0;
}
