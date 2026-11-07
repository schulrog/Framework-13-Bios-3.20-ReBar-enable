/* MIT license. Session-only initial-page redirection, no settings writes.
 * Temporarily replaces the public FormBrowser2 SendForm pointer. The wrapper
 * restores it BEFORE entering the original browser, and on StartImage return.
 */
static int prepare_browser(void*);
static void restore_browser(void);
#define BEFORE_START(s) prepare_browser(s)
#define AFTER_START() restore_browser()
#define EfiMain StockLaunchMain
#include "OpenSetup.c"
#undef EfiMain
typedef struct Browser Browser;
typedef U(*Send)(Browser*,H*,U,G*,uint16_t,void*,U*);
struct Browser {Send send;void*callback;};
typedef struct Database Database;
struct Database {void*newp,*remove,*update;U(*list)(Database*,uint8_t,G*,U*,H*);U(*exportp)(Database*,H,U*,void*);};
#ifdef MERGE_HANDLER
static U MERGE_HANDLER(Browser*,H*,U,G*,uint16_t,void*,U*,H);
#endif
static G browserGuid={0xb9d4c360,0xbcfb,0x4f9b,{0x92,0x98,0x53,0xc1,0x36,0x98,0x22,0x58}};
static G hiiGuid={0xef9fc172,0xa1b2,0x4693,{0xb3,0x27,0x6d,0x32,0xfc,0x41,0x60,0x42}};
static G advancedGuid={0xc6d4769e,0x7f48,0x4d2a,{0x98,0xe9,0x87,0xad,0xcc,0xf3,0x5c,0xcc}};
static G saGuid={0x72c5e28c,0x7783,0x43a1,{0x87,0x67,0xfa,0xd7,0x3f,0xcc,0xaf,0xa4}};
static ST*systemTable;static Browser*browser;static Send original;static Database*database;
static U(*pool)(uint32_t,U,void**);static U(*freepool)(void*);
static int matched,called;
#ifndef TARGET_TITLE
#define TARGET_TITLE 0x1cbe
#endif
#ifndef TARGET_QUESTION
#define TARGET_QUESTION 0x448
#endif
#ifndef TARGET_FORM
#define TARGET_FORM 0x159a
#endif
#ifndef TARGET_NOTICE
#define TARGET_NOTICE u"Opening PCI Express Configuration, form 159A. No settings have been saved.\r\n"
#endif
static uint16_t word(const uint8_t*p){return p[0]|(uint16_t)p[1]<<8;}
static uint32_t dword(const uint8_t*p){return word(p)|(uint32_t)word(p+2)<<16;}
static int same(const uint8_t*a,const void*b,U n){const uint8_t*p=b;for(U j=0;j<n;j++)if(a[j]!=p[j])return 0;return 1;}
/* Match the NORMAL Advanced package and exact ReBAR field, not a shared GUID
 * alone. Returns 1 for a match, 0 for non-match, -1 for malformed data.
 */
static int target_package(const uint8_t*b,U n){
#ifdef REBAR_TRACE
 if(n>500000){say(systemTable,u"Header length: ");number(systemTable,dword(b+16));}
#endif
 if(n<20||dword(b+16)!=n)return -1;int fs=0,var=0,question=0,root=0;U p=20;
 while(p<n){if(n-p<4)return -1;U size=dword(b+p)&0xffffff;unsigned type=b[p+3];if(size<4||size>n-p){
#ifdef REBAR_TRACE
 say(systemTable,u"Bad package at: ");number(systemTable,p);number(systemTable,size);
#endif
 return -1;}
  if(type==2){U z=p+4;uint16_t form=0;
   while(z<p+size){if(p+size-z<2)return -1;U s=b[z+1]&127;unsigned op=b[z];if(s<2||s>p+size-z){
#ifdef REBAR_TRACE
 say(systemTable,u"Bad opcode at: ");number(systemTable,z);number(systemTable,s);
#endif
 return -1;}
    if(op==0x0e){if(s<23)return -1;fs=same(b+z+2,&advancedGuid,16)&&word(b+z+18)==TARGET_TITLE;}
    if(op==1&&s>=6){form=word(b+z+2);if(fs&&form==TARGET_FORM)root=1;}
    if(op==0x26&&s>=34&&word(b+z+2)==2&&same(b+z+4,&saGuid,16)&&dword(b+z+20)==7&&word(b+z+24)==0x578&&same(b+z+26,"SaSetup\0",8))var=1;
    if(op==5&&s==17&&fs&&form==0x159a&&word(b+z+6)==TARGET_QUESTION&&word(b+z+8)==2&&word(b+z+10)==0x42b&&(b[z+13]&3)==0&&b[z+14]==0&&b[z+15]==2)question=1;
    z+=s;
   }
  }p+=size;
 }return fs&&var&&question&&root;
}
static H choose_handle(void){
 U bytes=0,r=database->list(database,2,0,&bytes,0);
#ifdef REBAR_TRACE
 say(systemTable,u"List status/bytes: ");number(systemTable,r);number(systemTable,bytes);
#endif
 if(r!=ERROR(5)||bytes==0||bytes>65536||bytes%sizeof(H))return 0;
 H*handles=0;if(BAD(pool(2,bytes,(void**)&handles)))return 0;U capacity=bytes;r=database->list(database,2,0,&bytes,handles);
 H chosen=0;unsigned matches=0;
 if(!BAD(r)&&bytes<=capacity){for(U j=0;j<bytes/sizeof(H);j++){
  U n=0;r=database->exportp(database,handles[j],&n,0);if((r!=ERROR(5)&&r!=ERROR(9))||n<20||n>8*1024*1024)continue;
  void*b=0;if(BAD(pool(2,n,&b)))continue;U cap=n;r=database->exportp(database,handles[j],&n,b);
  int match=(!BAD(r)&&n<=cap)?target_package(b,n):-2;
#ifdef REBAR_TRACE
  say(systemTable,u"Export status/length/match: ");number(systemTable,r);number(systemTable,n);number(systemTable,(U)match);
#endif
  if(match==1){chosen=handles[j];matches++;}freepool(b);
 }}freepool(handles);return matches==1?chosen:0;
}
static U redirected(Browser*this,H*handles,U count,G*guid,uint16_t form,void*screen,U*action){
 called=1;restore_browser();
 H selected=choose_handle();if(!selected){say(systemTable,u"Target setup package not uniquely matched; leaving the normal menu unchanged.\r\n");return original(this,handles,count,guid,form,screen,action);}
 matched=1;
#ifdef MERGE_HANDLER
 if(MERGE_REQUESTED)return MERGE_HANDLER(this,handles,count,guid,form,screen,action,selected);
#endif
 say(systemTable,TARGET_NOTICE);
 return original(this,&selected,1,&advancedGuid,TARGET_FORM,screen,action);
}
static void restore_browser(void){if(browser&&browser->send==redirected)browser->send=original;}
static int prepare_browser(void*p){
#ifdef MENU_MODE_GATE
 if(!(MENU_MODE_GATE))return 1;
#endif
 ST*s=p;systemTable=s;U(*locate)(G*,void*,void**)=(void*)s->bs->v[37];
 pool=(void*)s->bs->v[5];freepool=(void*)s->bs->v[6];
 U r=locate(&hiiGuid,0,(void**)&database);if(BAD(r))return 0;
 r=locate(&browserGuid,0,(void**)&browser);if(BAD(r)||!browser||!browser->send)return 0;
 original=browser->send;browser->send=redirected;return browser->send==redirected;
}
#ifndef REBAR_TEST_HARNESS
U EfiMain(H image,ST*s){
 say(s,u"RebarPage: session-only setup page selection. First test: view only, then exit without saving.\r\n");
 U r=StockLaunchMain(image,s);restore_browser();
 if(!called)say(s,u"Stock Setup did not call the selected browser interface. No page redirection occurred.\r\n");
 else if(!matched)say(s,u"The target page was not matched. No page redirection occurred.\r\n");
 return r;
}
#endif
