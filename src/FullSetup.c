/* MIT license. One launcher, normal setup or the full Advanced_Hidden tree.
 * All defaults from the proven RebarPage path are explicitly overridden below.
 */
static int setupChoice;
#define REBAR_TEST_HARNESS
#define TARGET_TITLE 0x1c92
#define TARGET_QUESTION 0x7cb
#define TARGET_FORM 1
#define TARGET_NOTICE u"Opening the full hidden Advanced menu. No settings have been saved.\r\n"
#define MENU_MODE_GATE (setupChoice==2||setupChoice==3)
#define MERGE_REQUESTED (setupChoice==3)
#define MERGE_HANDLER merged_browser
#include "RebarPage.c"
typedef struct Strings Strings;
struct Strings {void*newstring;U(*get)(Strings*,char*,H,uint16_t,C*,U*,void**);U(*set)(Strings*,H,uint16_t,char*,C*,void*);};
static G stringGuid={0x0fd96974,0x23aa,0x4cdc,{0xb9,0xcb,0x98,0xd1,0x77,0x50,0x32,0x2a}};
static C*backup_title(Strings*s,H h,uint16_t id){
 U n=0,r=s->get(s,"en-US",h,id,0,&n,0);if(r!=ERROR(5)||n<2||n>4096||n%2)return 0;
 C*b=0;if(BAD(pool(2,n,(void**)&b)))return 0;U cap=n;r=s->get(s,"en-US",h,id,b,&n,0);if(BAD(r)||n>cap){freepool(b);return 0;}return b;
}
static U merged_browser(Browser*this,H*handles,U count,G*guid,uint16_t form,void*screen,U*action,H selected){
 if(!handles||!count||count>1024)return original(this,handles,count,guid,form,screen,action);
 U(*locate)(G*,void*,void**)=(void*)systemTable->bs->v[37];Strings*strings=0;
 U r=locate(&stringGuid,0,(void**)&strings);if(BAD(r)||!strings)return original(this,handles,count,guid,form,screen,action);
 C*oldRoot=backup_title(strings,selected,0x1cbe);C*oldSet=backup_title(strings,selected,0x1c92);
 H*combined=0;int renamedRoot=0,renamedSet=0;U total=count;int present=0;
 if(!oldRoot||!oldSet)goto fallback;
 for(U j=0;j<count;j++)if(handles[j]==selected)present=1;
 if(BAD(pool(2,(count+1)*sizeof(H),(void**)&combined)))goto fallback;
 for(U j=0;j<count;j++)combined[j]=handles[j];if(!present)combined[total++]=selected;
 r=strings->set(strings,selected,0x1cbe,"en-US",u"Advanced (Extended)",0);if(BAD(r))goto fallback;renamedRoot=1;
 r=strings->set(strings,selected,0x1c92,"en-US",u"Advanced (Extended)",0);if(BAD(r))goto fallback;renamedSet=1;
 say(systemTable,u"Opening normal setup with Advanced (Extended) added. No settings have been saved.\r\n");
 r=original(this,combined,total,guid,form,screen,action);
 if(renamedSet)strings->set(strings,selected,0x1c92,"en-US",oldSet,0);
 if(renamedRoot)strings->set(strings,selected,0x1cbe,"en-US",oldRoot,0);
 freepool(combined);freepool(oldRoot);freepool(oldSet);return r;
fallback:
 if(renamedSet)strings->set(strings,selected,0x1c92,"en-US",oldSet,0);
 if(renamedRoot)strings->set(strings,selected,0x1cbe,"en-US",oldRoot,0);
 if(combined)freepool(combined);if(oldRoot)freepool(oldRoot);if(oldSet)freepool(oldSet);
 say(systemTable,u"Combined menu could not be prepared; opening normal setup unchanged.\r\n");
 return original(this,handles,count,guid,form,screen,action);
}
typedef struct {uint16_t scan,character;} Key;
typedef struct Input Input;struct Input {void*reset;U(*read)(Input*,Key*);H wait;};
static int key_choice(Key k){if(k.scan==0x17)return -1;if(k.character=='1')return 1;if(k.character=='2')return 2;if(k.character=='3')return 3;return 0;}
static int choose_menu(ST*s){
 Input*in=s->in;if(!in||!in->read||!in->wait)return -1;
 U(*wait)(U,H*,U*)=(void*)s->bs->v[9];
 say(s,u"\r\nFramework BIOS 3.20 - Normal + Hidden Setup\r\n\r\n1  Normal BIOS Setup\r\n2  Hidden Advanced only\r\n3  Combined: normal tabs + Advanced (Extended)\r\nEsc  Return to shell\r\n\r\nSelect 3 to test the combined interface. First visit: inspect only.\r\n");
 for(;;){Key k={0};U r=in->read(in,&k);if(!BAD(r)){int choice=key_choice(k);if(choice)return choice;continue;}if(r!=ERROR(6))return -1;U index=0;r=wait(1,&in->wait,&index);if(BAD(r))return -1;}
}
#ifndef FULL_SETUP_TEST_HARNESS
U EfiMain(H image,ST*s){
 setupChoice=choose_menu(s);if(setupChoice<0)return 0;
 U r=StockLaunchMain(image,s);restore_browser();
 if(setupChoice!=1){if(!called)say(s,u"The stock application did not call this browser interface. Hidden menu was not opened.\r\n");else if(!matched)say(s,u"Hidden Advanced did not match uniquely. Only normal setup was used.\r\n");}
 return r;
}
#endif
