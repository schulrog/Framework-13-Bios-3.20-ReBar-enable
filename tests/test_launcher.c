#include <stdio.h>
#include <string.h>
#undef NDEBUG
#include <assert.h>
#include "OpenSetup.c"
int main(void){
 uint8_t out[128]={0},end[]={0x7f,0xff,4,0};
 assert(makepath(end,out,sizeof(out))==0);assert(out[0]==4&&out[1]==6&&out[2]==20&&out[20]==0x7f);assert(!memcmp(out+4,&target,16));puts("PASS firmware-file node and GUID");
 assert(makepath(end,out,23)==ERROR(4));puts("PASS short output rejected");
 uint8_t multi[]={0x7f,1,4,0};assert(makepath(multi,out,sizeof(out))==ERROR(3));puts("PASS unsupported multi-instance path rejected");
 uint8_t bad[]={1,1,0,0};assert(makepath(bad,out,sizeof(out))==ERROR(4));puts("PASS malformed node rejected");
 uint8_t path[]={1,1,4,0,0x7f,0xff,4,0};assert(makepath(path,out,sizeof(out))==0);assert(!memcmp(path,out,4)&&out[4]==4&&out[5]==6);puts("PASS existing path preserved");return 0;
}
