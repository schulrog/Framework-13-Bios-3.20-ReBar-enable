#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#undef NDEBUG
#include <assert.h>
#include "HiiWatch.c"
static unsigned char data[128];static U length;static int writes,opens,frees,closes,exists,shortwrite,reenter,flushes,flusherror;
static U exp(DB*d,H h,U*n,void*b){(void)d;(void)h;if(!b){*n=length;return ERROR(5);}assert(*n>=length);memcpy(b,data,(size_t)length);*n=length;return 0;}
static U al(uint32_t t,U n,void**p){assert(t==2);*p=malloc((size_t)n);return *p?0:ERROR(9);}
static U fr(void*p){free(p);frees++;return 0;}
static File rootfile,outfile;
static U cl(File*f){(void)f;closes++;return 0;}
static U fl(File*f){assert(f==&outfile);flushes++;return flusherror?ERROR(7):0;}
static U wr(File*f,U*n,void*b){assert(f==&outfile);assert(*n==length);assert(!memcmp(data,b,(size_t)*n));writes++;if(reenter)tick(0,0);if(shortwrite)(*n)--;return 0;}
static U op(File*f,File**out,C*name,U mode,U attr){(void)name;assert(f==&rootfile);assert(attr==0);opens++;if(mode==1){if(exists){*out=&outfile;return 0;}return ERROR(14);}assert(mode==0x8000000000000003ULL);*out=&outfile;return 0;}
static U rt(FS*f,File**out){(void)f;*out=&rootfile;return 0;}
static DB testdb={.exportp=exp};static FS testfs={.root=rt};
static void setup(int target){
 memset(data,0,sizeof(data));length=20+4+23+4;uint32_t n=(uint32_t)length;memcpy(data+16,&n,4);n=0x0200001b;memcpy(data+20,&n,4);data[24]=0x0e;data[25]=23;
 if(target)memcpy(data+26,&advanced,16);n=0xdf000004;memcpy(data+47,&n,4);
 db=&testdb;fs=&testfs;allocate=al;release=fr;rootfile.open=op;rootfile.close=cl;outfile.close=cl;outfile.write=wr;outfile.flush=fl;
 writes=opens=frees=closes=exists=shortwrite=reenter=busy=finished=attempts=flushes=flusherror=0;
}
int main(void){
 setup(1);assert(contains_advanced(data,length)==1);assert(capture()==0);assert(writes==1&&frees==1&&closes==2&&finished&&flushes==1);puts("PASS capture and cleanup");
 setup(0);assert(capture()==ERROR(14));assert(!writes&&!opens&&frees==1);puts("PASS absent target makes no file");
 setup(1);exists=1;assert(capture()==ERROR(15));assert(writes==0&&finished&&closes==2);puts("PASS no overwrite");
 setup(1);shortwrite=1;assert(capture()==ERROR(7));assert(finished&&frees==1);puts("PASS partial write stops retry");
 setup(1);reenter=1;tick(0,0);assert(writes==1&&!busy&&finished);puts("PASS reentrancy guard");
 setup(1);data[25]=1;assert(contains_advanced(data,length)==-1);assert(capture()==ERROR(33));assert(!writes&&!opens);puts("PASS invalid IFR rejected");
 setup(1);for(U i=1;i<length;i++)assert(contains_advanced(data,i)==-1);puts("PASS all truncations rejected");
 setup(0);attempts=120;tick(0,0);assert(frees==0);changed(2,0,0,0,1);assert(attempts==0);tick(0,0);assert(frees==1);puts("PASS bounded polling and notification reset");
 setup(1);flusherror=1;assert(capture()==ERROR(7));assert(finished&&flushes==1&&closes==2&&frees==1);puts("PASS flush error and cleanup");
 return 0;
}
