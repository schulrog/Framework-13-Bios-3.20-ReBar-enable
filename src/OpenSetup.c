/* MIT license. Launch the installed stock SetupUtilityApp by firmware-file GUID.
 * No firmware writes, patches, variable writes, or security-policy bypasses.
 * The launched stock Setup application runs its normal initialization code.
 */
#include <stdint.h>
#include <stddef.h>
typedef uint64_t U;typedef uint16_t C;typedef void*H;
typedef struct {uint32_t a;uint16_t b,c;uint8_t d[8];} G;
typedef struct {U sig;uint32_t rev,size,crc,res;} Header;
typedef struct Text Text;struct Text {void*reset;U(*out)(Text*,C*);};
typedef struct {Header h;void*v[44];} BS;
typedef struct {Header h;C*vendor;uint32_t rev;H inH;void*in;H outH;Text*out;H errH;Text*err;void*rt;BS*bs;} ST;
typedef struct FV FV;struct FV {void*getattrs,*setattrs;U(*read)(FV*,G*,void**,U*,uint8_t*,uint32_t*,uint32_t*);};
#define ERROR(n) (0x8000000000000000ULL|(n))
#define BAD(x) ((x)>>63)
static G fvGuid={0x220e73b6,0x6bdb,0x4413,{0x84,5,0xb9,0x74,0xb1,8,0x61,0x9a}};
static G dpGuid={0x09576e91,0x6d3f,0x11d2,{0x8e,0x39,0,0xa0,0xc9,0x69,0x72,0x3b}};
#ifdef EMULATOR_SHELL
static G target={0x7c04a583,0x9e3e,0x4f1c,{0xad,0x65,0xe0,0x52,0x68,0xd0,0xb4,0xd1}};
#else
static G target={0x3935b0a1,0xa182,0x4887,{0xbc,0x56,0x67,0x55,0x28,0xe7,0x88,0x77}};
#endif
static void say(ST*s,C*t){if(s->out)s->out->out(s->out,t);}
static void number(ST*s,U r){C t[19];t[0]='0';t[1]='x';for(int j=0;j<16;j++)t[j+2]=u"0123456789ABCDEF"[(r>>(60-4*j))&15];t[18]=0;say(s,t);say(s,u"\r\n");}
/* Copy one complete device path, append a PIWG firmware-file node. */
static U makepath(const uint8_t*dp,uint8_t*out,U capacity){
 U z=0;
 while(z+4<=capacity){
  U n=dp[z+2]|(U)dp[z+3]<<8;
  if(n<4||n>capacity-z)return ERROR(4);
  if(dp[z]==0x7f){if(dp[z+1]!=0xff||n!=4)return ERROR(3);break;}
  if(n>capacity-z-4)return ERROR(4);
  for(U j=0;j<n;j++)out[z+j]=dp[z+j];z+=n;
 }
 if(z+24>capacity)return ERROR(4);
 out[z]=4;out[z+1]=6;out[z+2]=20;out[z+3]=0;
 for(U j=0;j<16;j++)out[z+4+j]=((uint8_t*)&target)[j];
 out[z+20]=0x7f;out[z+21]=0xff;out[z+22]=4;out[z+23]=0;return 0;
}
U EfiMain(H image,ST*s){
 U(*handles)(uint32_t,G*,void*,U*,H**)=(void*)s->bs->v[36];
 U(*handle)(H,G*,void**)=(void*)s->bs->v[16];U(*freep)(void*)=(void*)s->bs->v[6];
 U(*load)(uint8_t,H,void*,void*,U,H*)=(void*)s->bs->v[22];
 U(*start)(H,U*,C**)=(void*)s->bs->v[23];U(*unload)(H)=(void*)s->bs->v[25];
 H*volumes=0;U count=0,r=handles(2,&fvGuid,0,&count,&volumes);
 if(BAD(r)){say(s,u"Firmware-volume enumeration failed: ");number(s,r);return r;}
 H selected=0;U matches=0;
 for(U i=0;i<count;i++){
  FV*f=0;U size=0;uint8_t type=0;uint32_t attrs=0,auth=0;
  r=handle(volumes[i],&fvGuid,(void**)&f);if(BAD(r))continue;
  r=f->read(f,&target,0,&size,&type,&attrs,&auth);
  if(!BAD(r)&&type==9){selected=volumes[i];matches++;}
 }
 if(volumes)freep(volumes);
 if(matches!=1){say(s,u"Expected one stock Setup application; match count: ");number(s,matches);return matches?ERROR(3):ERROR(14);}
 uint8_t*dp=0;uint8_t path[1024];
 r=handle(selected,&dpGuid,(void**)&dp);if(BAD(r)){say(s,u"Firmware volume has no device path: ");number(s,r);return r;}
 r=makepath(dp,path,sizeof(path));if(BAD(r))return r;
 H app=0;r=load(0,image,path,0,0,&app);
 if(BAD(r)){say(s,u"Stock Setup LoadImage failed: ");number(s,r);if(app)unload(app);return r;}
 say(s,u"Opening the stock firmware application. Do not save settings.\r\n");
#ifdef BEFORE_START
 if(!BEFORE_START(s)){unload(app);return ERROR(3);}
#endif
 U exitSize=0;C*exitData=0;r=start(app,&exitSize,&exitData);
#ifdef AFTER_START
 AFTER_START();
#endif
 say(s,u"Stock application returned: ");number(s,r);
 if(exitData)freep(exitData);
 /* A normal application is unloaded by firmware when it returns. */
 return r;
}
_Static_assert(offsetof(ST,bs)==96,"system-table ABI");
_Static_assert(offsetof(FV,read)==16,"FV2 ABI");
