#include<stdio.h>
#include<inttypes.h>
#include<dirent.h>
#include<string.h>
#include<stdlib.h>
#include<stdbool.h>

#define MRead(name, n) \
  unsigned char name[n];\
  uint32_t s ## name;\
  fread(&name, n, 1, stream);\
  if (verbose){\
  printf("     %16s : ", #name);}\
  for(int a=0;a<6;a++){\
    if (verbose){\
    if (a<n){\
      printf("%02x ",name[a]);\
    } else {\
      printf("   ");\
    }}\
  }\
  if (verbose){\
  printf(" ");}\
  s ## name=0;\
  for(int a=0;a<6;a++){\
    if (a<n) {\
      s ## name =  s ## name * 16 * 16 + name[a];\
      if (verbose){\
      if (name[a]>=32){\
        printf(" %1c ",name[a]);\
      } else {\
        printf("⌞ ⌟");\
      }}\
    } else {\
      if (verbose){\
      printf("   ");}\
    }\
  }\
  if (verbose)\
  printf(" %10d\n",s ## name);\

typedef struct AIFF { 
    uint32_t sampleFrames;
    uint16_t *samples;
    char name[256];
} AIFF;

typedef struct STEM {
  uint32_t maxSampleFrames;
  uint32_t n;
  AIFF * aiffs;
} STEM;
  

void readAiff(FILE * stream, AIFF *aiff){
  bool verbose = false;
  MRead(ckID,4);
  MRead(fileSize,4);
  MRead(formType,4);
  MRead(commType,4);
  MRead(chunkSize,4);
  MRead(numChannels,2);
  MRead(sampleFrames,4);
  MRead(bitDepth,2);
  MRead(sampleRatePre,2);
  MRead(sampleRate,2);
  MRead(sampleRatePost,6);
  MRead(sndChunkID,4);
  MRead(sndChunkSize,4);
  MRead(sndOffset,4);
  MRead(sndBlockSize,4);
  
  aiff->sampleFrames=ssampleFrames;
  aiff->samples = malloc( sizeof(uint16_t) * aiff->sampleFrames * 2); 
  fread(aiff->samples, sizeof(uint16_t), aiff->sampleFrames*2, stream);
  printf("  sample frames: %d\n", aiff->sampleFrames);
  return;
}


void openmulti (char *dirname, STEM *stem)
{

    uint32_t maxS = 0;\
    DIR *d;
    FILE *stream;
    struct dirent *dir;
    int aiffCnt = 0;
    d = opendir(dirname);
      

    if (d) {
      while ((dir = readdir(d)) != NULL) {
        
        if (!strstr(dir->d_name, ".aiff"))
          continue;
        aiffCnt++;
      }
      closedir(d);
    }

    d = opendir(dirname);
    if (d) {

      stem->aiffs = malloc(sizeof(AIFF)*aiffCnt);
      stem->n=aiffCnt;
      int i=0;
      
      while ((dir = readdir(d)) != NULL) {
        
        if (!strstr(dir->d_name, ".aiff"))
          continue;

        stream = fopen (dir->d_name, "r");
        if (stream == NULL) 
            continue;
        strcpy(stem->aiffs[i].name,dir->d_name);
        readAiff(stream,&stem->aiffs[i]);
        if (stem->aiffs[i].sampleFrames>maxS){
          maxS=stem->aiffs[i].sampleFrames;
        }
        printf("  %24s   sample frames: %10d\n\n", stem->aiffs[i].name, stem->aiffs[i].sampleFrames);
        fclose(stream);
        i++;
      }
      closedir(d);
    }
    printf("  max sample frames: %10d\n",maxS );
    stem->maxSampleFrames = maxS;
    
}

void writeChars(char *s, size_t n, FILE *stream){
  //printf("writing %s, size:%d\n",s,(int)n );
  fwrite(s, sizeof(char), n, stream);
}

void writeUInt32(uint32_t x, FILE *stream){
  char a = ((x)&0xFF);
  char b = ((x>>8)&0xFF);
  char c = ((x>>16)&0xFF);
  char d = ((x>>24)&0xFF);
  char  fs[4] = {d,c,b,a};
  fwrite(fs, sizeof(char), 4, stream);
}


void write(STEM *stem, int loops){

  printf("aiffs: %d\n", (int)stem->n);

  FILE * stream;
  stream = fopen("out.aiff", "w");

  writeChars("FORM",4,stream);                            // FORM
  writeUInt32(stem->maxSampleFrames * 2 * 2 * loops + 46, stream);// filesize
  writeChars("AIFF",4, stream);                           // AIFF
  writeChars( "COMM",4, stream);                          // COMM
  writeChars("\x00\x00\x00\x12",4, stream);               // chunk size
  writeChars("\x00\x02",2, stream);                       // channels
  writeUInt32(stem->maxSampleFrames*loops, stream);             // sample frames
  writeChars("\x00\x10",2, stream);                       // bit depth
  writeChars("\x40\x0E",2, stream);                       // sample rate pre
  writeChars("\xbb\x80",2, stream);                       // sample rate
  writeChars("\0\0\0\0\0\0",6, stream);                   // sample rate post
  writeChars("SSND",4, stream);                           // SSND chunk ID
  writeUInt32(stem->maxSampleFrames * 2 * 2 *loops + 8, stream); // SSND chunk size
  writeUInt32(0, stream);                                 // SSND offset
  writeUInt32(0, stream);                                 // SSND block size

  uint32_t i = 0;
  uint32_t j = 0;
  uint16_t s = 0;
  int16_t left = 0;
  int16_t right = 0;
  int32_t left32 = 0;
  int32_t right32 = 0;
  printf("writing %d samplesframes :\n", stem->maxSampleFrames*loops);

  int32_t measureLeft;
  int32_t measureRight;
  int32_t max=0;
  int32_t min=0;
  for(i=0; i<stem->maxSampleFrames;i++){
    measureLeft=0;
    measureRight=0;
    for(j=0; j<stem->n;j++){
      uint16_t leftBE= stem->aiffs[j].samples[2*(i%stem->aiffs[j].sampleFrames)];
      uint16_t rightBE= stem->aiffs[j].samples[1+2*(i%stem->aiffs[j].sampleFrames)];

      int16_t lFull= (int16_t)((leftBE>>8) | (leftBE<<8));
      int16_t rFull= (int16_t)((rightBE>>8) | (rightBE<<8));

      measureLeft += lFull;
      measureRight += rFull;
    }
    if (measureLeft>max)
      max=measureLeft;
    if (measureRight>max)
      max=measureRight;
    if (measureLeft<min)
      min=measureLeft;
    if (measureRight<min)
      min=measureRight;
  }

  
  float scale = (float)INT16_MAX / (float)max;
  float scale2 = (float)INT16_MIN / (float)min;

  

  if (scale2<scale){
    scale = scale2;
  }

  if (scale>1){
    scale=1;
    
  } else{
    printf("  output will be normalized\n  signal will be multiplied by %f to prevent clipping.", scale);
  }

  for(i=0; i<stem->maxSampleFrames*loops;i++){
    left32=0;
    right32=0;
    for(j=0; j<stem->n;j++){
      uint16_t leftBE= stem->aiffs[j].samples[2*(i%stem->aiffs[j].sampleFrames)];
      uint16_t rightBE= stem->aiffs[j].samples[1+2*(i%stem->aiffs[j].sampleFrames)];

      int16_t lFull= (int16_t)((leftBE>>8) | (leftBE<<8));
      int16_t rFull= (int16_t)((rightBE>>8) | (rightBE<<8));

      if (scale<1){
        left32+=lFull*scale;
        right32+=rFull*scale;  
      } else{
        left32+=lFull;
        right32+=rFull;
      }
      
    }
    left=left32;
    right=right32;
    uint16_t leftLE = (uint16_t)left;
    leftLE = ((leftLE>>8) | (leftLE<<8));
    uint16_t rightLE = (uint16_t)right;
    rightLE = ((rightLE>>8) | (rightLE<<8));
    
    fwrite(&leftLE,sizeof(uint16_t),1,stream);                  //left
    fwrite(&rightLE,sizeof(uint16_t),1,stream);                 //right
  }

  
  fclose(stream);
}

int main(){
  
  STEM stem;
  openmulti(".", &stem);
  if (stem.maxSampleFrames == 0){
    printf("  Nothing to output.\n");
  } else{
    printf("  Writing to out.aiff\n");
    write(&stem,4);  
  }
  
  //printf("%s\n", stem.aiffs[0].name);
   
  return(0);
}
