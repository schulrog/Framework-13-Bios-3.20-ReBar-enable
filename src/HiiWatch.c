/* MIT license. Temporary x64 UEFI boot-service driver.
 * Observes HII package additions; exports only after Advanced forms appear.
 * No SetVariable, RouteConfig, SendForm, flash, or SMM calls.
 */
#include <stdint.h>
#include <stddef.h>
typedef uint64_t U;typedef uint16_t C;typedef void *H;
typedef struct {uint32_t a;uint16_t b,c;uint8_t d[8];} G;
typedef struct {U sig;uint32_t rev,size,crc,res;} Header;
typedef struct Text Text;struct Text {void *reset;U(*out)(Text*,C*);};
typedef struct {Header h;void *v[44];} BS;
typedef struct {Header h;C *vendor;uint32_t rev;H inH;void *in;H outH;Text*out;H errH;Text*err;void*rt;BS*bs;} ST;
typedef struct File File;struct File {U rev;U(*open)(File*,File**,C*,U,U);U(*close)(File*);void*del,*read;U(*write)(File*,U*,void*);void*getpos,*setpos,*getinfo,*setinfo;U(*flush)(File*);};
typedef struct FS FS;struct FS {U rev;U(*root)(FS*,File**);};
typedef struct {uint32_t rev;H parent;ST*st;H device;} LI;
typedef U(*Notify)(uint8_t,G*,void*,H,U);
typedef struct DB DB;struct DB {void*newp,*remove,*update,*list;U(*exportp)(DB*,H,U*,void*);U(*reg)(DB*,uint8_t,G*,Notify,U,H*);U(*unreg)(DB*,H);};
#define ERROR(n) (0x8000000000000000ULL|(n))
#define BAD(x) ((x)>>63)
static G hg={0xef9fc172,0xa1b2,0x4693,{0xb3,0x27,0x6d,0x32,0xfc,0x41,0x60,0x42}};
static G lg={0x5b1b31a1,0x9562,0x11d2,{0x8e,0x3f,0,0xa0,0xc9,0x69,0x72,0x3b}};
static G fg={0x964e5b22,0x6459,0x11d2,{0x8e,0x39,0,0xa0,0xc9,0x69,0x72,0x3b}};
static G advanced={0xc6d4769e,0x7f48,0x4d2a,{0x98,0xe9,0x87,0xad,0xcc,0xf3,0x5c,0xcc}};
static DB*db;static FS*fs;static H timer,registration;static int busy,finished;static unsigned attempts;
static U(*allocate)(uint32_t,U,void**);static U(*release)(void*);static U(*settimer)(H,uint32_t,U);
static uint32_t u32(const uint8_t*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static int equal(const uint8_t*a,const uint8_t*b,U n){for(U i=0;i<n;i++)if(a[i]!=b[i])return 0;return 1;}
/* Validate every list/package/opcode; match a FormSet, not an incidental GUID. */
static int contains_advanced(const uint8_t*b,U n){
 U o=0;int found=0;
 while(o<n){if(n-o<20)return -1;U len=u32(b+o+16);if(len<20||len>n-o)return -1;
  U p=o+20,end=o+len;
  while(p<end){if(end-p<4)return -1;uint32_t h=u32(b+p);U size=h&0xffffff;
   if(size<4||size>end-p)return -1;
   if((h>>24)==2){U z=p+4;while(z<p+size){if(p+size-z<2)return -1;U s=b[z+1]&127;if(s<2||s>p+size-z)return -1;
    if(b[z]==0x0e&&s>=23&&equal(b+z+2,(const uint8_t*)&advanced,16))found=1;z+=s;}}
   p+=size;
  }o=end;
 }return found;
}
static U capture(void){
 U n=0,r;void*buffer=0;File*root=0,*file=0;
 r=db->exportp(db,0,&n,0);if(!BAD(r)&&n==0)return ERROR(14);
 if(r!=ERROR(5)&&r!=ERROR(9))return r;
 if(n<20||n>64*1024*1024)return ERROR(4);
 U capacity=n;r=allocate(2,n,&buffer);if(BAD(r))return r;
 r=db->exportp(db,0,&n,buffer);if(BAD(r))goto cleanup;
 if(n>capacity){r=ERROR(4);goto cleanup;}
 int valid=contains_advanced(buffer,n);if(valid!=1){r=valid<0?ERROR(33):ERROR(14);goto cleanup;}
 r=fs->root(fs,&root);if(BAD(r))goto cleanup;
 r=root->open(root,&file,u"HII-setup.bin",1,0);
 if(!BAD(r)){finished=1;r=ERROR(15);goto cleanup;}
 file=0;if(r!=ERROR(14))goto cleanup;
 r=root->open(root,&file,u"HII-setup.bin",0x8000000000000003ULL,0);if(BAD(r))goto cleanup;
 {U written=n;r=file->write(file,&written,buffer);if(!BAD(r)&&written!=n)r=ERROR(7);if(!BAD(r)&&file->flush)r=file->flush(file);}
 /* Do not retry after creating a file, including a partial write. */
 finished=1;
cleanup:
 if(file)file->close(file);if(root)root->close(root);if(buffer)release(buffer);return r;
}
static void tick(H event,void*context){(void)event;(void)context;if(busy||finished||attempts>=120)return;attempts++;busy=1;(void)capture();busy=0;}
static U changed(uint8_t type,G*guid,void*package,H handle,U why){
 (void)guid;(void)package;(void)handle;(void)why;
 if(type==2&&!finished)attempts=0; /* defer all work to the periodic callback */
 return 0;
}
U EfiMain(H image,ST*s){
 U r;LI*li=0;
 U(*locate)(G*,void*,void**)=(void*)s->bs->v[37];U(*handle)(H,G*,void**)=(void*)s->bs->v[16];
 U(*create)(uint32_t,U,void(*)(H,void*),void*,H*)=(void*)s->bs->v[7];U(*close)(H)=(void*)s->bs->v[11];
 allocate=(void*)s->bs->v[5];release=(void*)s->bs->v[6];settimer=(void*)s->bs->v[8];
 r=locate(&hg,0,(void**)&db);if(BAD(r))return r;
 r=handle(image,&lg,(void**)&li);if(BAD(r))return r;
 r=handle(li->device,&fg,(void**)&fs);if(BAD(r))return r;
 r=create(0x80000200,8,tick,0,&timer);if(BAD(r))return r;
 r=db->reg(db,2,0,changed,1|8,&registration);if(BAD(r)){close(timer);timer=0;return r;}
 /* Periodic polling also observes a form already present on driver load. */
 r=settimer(timer,1,10000000);if(BAD(r)){db->unreg(db,registration);close(timer);timer=0;return r;}
 if(s->out)s->out->out(s->out,u"HiiWatch loaded. Open BIOS Setup without rebooting; wait 5 seconds.\r\nNo settings writes. Export target: HII-setup.bin on this drive.\r\nReboot removes this temporary driver.\r\n");
 return 0;
}
_Static_assert(offsetof(ST,bs)==96,"ST ABI");
_Static_assert(offsetof(LI,device)==24,"loaded-image ABI");
_Static_assert(offsetof(DB,reg)==40,"HII ABI");
