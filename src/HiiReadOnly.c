/* Offline-built x64 UEFI diagnostic. Exports HII to its own drive only.
 * Does not call SetVariable, RouteConfig, SendForm, flash or SMM services.
 * SPDX-License-Identifier: MIT
 */
#include <stdint.h>
#include <stddef.h>
typedef uint64_t U; typedef uint16_t C; typedef void *H;
typedef struct {uint32_t a;uint16_t b,c;uint8_t d[8];} G;
typedef struct {uint64_t sig;uint32_t rev,size,crc,res;} Header;
typedef struct Text Text;
struct Text {void *reset;U (*out)(Text*,C*);};
typedef struct {Header hdr;void *v[44];} BS;
typedef struct {Header hdr;C *vendor;uint32_t rev;H inH;void *in;H outH;Text *out;H errH;Text *err;void *rt;BS *bs;} ST;
typedef struct File File;
struct File {U revision;U (*open)(File*,File**,C*,U,U);U (*close)(File*);void *del,*read;U (*write)(File*,U*,void*);};
typedef struct FS FS;struct FS {U rev;U (*root)(FS*,File**);};
typedef struct {uint32_t rev;H parent;ST *st;H device;} LI;
typedef struct DB DB;struct DB {void *newp,*remove,*update,*list;U (*exportp)(DB*,H,U*,void*);};
static G hg={0xef9fc172,0xa1b2,0x4693,{0xb3,0x27,0x6d,0x32,0xfc,0x41,0x60,0x42}};
static G lg={0x5b1b31a1,0x9562,0x11d2,{0x8e,0x3f,0x00,0xa0,0xc9,0x69,0x72,0x3b}};
static G fg={0x964e5b22,0x6459,0x11d2,{0x8e,0x39,0x00,0xa0,0xc9,0x69,0x72,0x3b}};
#define ERR(x) ((x)>>63)
static void say(ST*s,C*t){if(s->out)s->out->out(s->out,t);}
static void status(ST*s,U v){C h[19];h[0]='0';h[1]='x';for(int j=0;j<16;j++)h[2+j]=u"0123456789ABCDEF"[(v>>(60-4*j))&15];h[18]=0;say(s,h);say(s,u"\r\n");}
_Static_assert(offsetof(ST,bs)==96,"ST ABI");
_Static_assert(offsetof(LI,device)==24,"LI ABI");
_Static_assert(offsetof(DB,exportp)==32,"HII ABI");
U EfiMain(H image,ST*s){
 U (*locate)(G*,void*,void**)=(void*)s->bs->v[37];
 U (*handle)(H,G*,void**)=(void*)s->bs->v[16];
 U (*alloc)(uint32_t,U,void**)=(void*)s->bs->v[5];
 U (*freep)(void*)=(void*)s->bs->v[6];
 DB *db=0;LI *li=0;FS *fs=0;File *root=0,*file=0;void *buf=0;U n=0,r=0;
 say(s,u"HiiReadOnly: exports setup packages to USB. No settings writes.\r\n");
 r=locate(&hg,0,(void**)&db);if(ERR(r))goto done;
 r=db->exportp(db,0,&n,0);
 if(!n||n>64*1024*1024){say(s,u"No packages or export exceeds 64 MiB.\r\n");goto done;}
 r=alloc(2,n,&buf);if(ERR(r))goto done;
 r=db->exportp(db,0,&n,buf);if(ERR(r))goto done;
 r=handle(image,&lg,(void**)&li);if(ERR(r))goto done;
 r=handle(li->device,&fg,(void**)&fs);if(ERR(r))goto done;
 r=fs->root(fs,&root);if(ERR(r))goto done;
 /* Refuse to overwrite an existing export. */
 r=root->open(root,&file,u"HII-export.bin",1,0);
 if(!ERR(r)){say(s,u"HII-export.bin already exists. Rename it before another run.\r\n");r=0x800000000000000fULL;goto done;}
 if(r!=0x800000000000000eULL)goto done;
 r=root->open(root,&file,u"HII-export.bin",0x8000000000000003ULL,0);if(ERR(r))goto done;
 {U written=n;r=file->write(file,&written,buf);if(!ERR(r)&&written!=n)r=0x8000000000000007ULL;}
 if(!ERR(r))say(s,u"Export saved: HII-export.bin on this application's drive.\r\n");
done:
 if(file)file->close(file);if(root)root->close(root);if(buf)freep(buf);
 say(s,u"Status: ");status(s,r);return r;
}
